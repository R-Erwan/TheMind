#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "playersRessources.h"

#define MAX_CLIENTS 2
#define SERVER_FULL_MSG "Le serveur est plein. Veuillez réessayer plus tard.\n"
#define CMD_READY "ready"
#define CMD_UNREADY "unready"

typedef struct {
    Player *p;
    PlayerList  *pl;
} ClientThreadArgs;

typedef struct{
    int listen_fd;
    PlayerList *pl;
} ListentThreadArgs;

volatile int keepalive = 1;
pthread_cond_t keepalive_cond;
pthread_mutex_t keepalive_mutex;

/* Affiche un message d'érreur critique et ferme le programme */
void fatal_error(const char* msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * Fonction des threads joueurs
 * @param client_sock le socket client
 * @return
 */
void *handle_client(void *arg) {
    ClientThreadArgs *args = (ClientThreadArgs *)arg;
    Player *p = args->p;
    PlayerList *pl = args->pl;

    char name[50];
    char message[256];

    snprintf(message,sizeof message,"Bienvenue sur TheMind ! \nEnvoyé votre nom \n");
    send(p->socket_fd,message,sizeof message,0);

    recv(p->socket_fd,name,sizeof (name) -1, 0);
    if(!set_player_name(pl,p,name)){
        //TODO Traiter le cas ou le nom du client ne vas pas.
    }

    snprintf(message, sizeof message, "Bienvenue %s\nIl y a %d joueurs\nEnvoyé 'ready' si vous êtes prêt a commencé !\n", p->name, pl->count);
    send(p->socket_fd,message,sizeof message,0);

    new_player_broadcast(pl,p); // Envoie un message pour annoncer une nouvelle connexion

    char buffer[BUFSIZ];
    while(1){
        ssize_t len = recv(p->socket_fd,buffer,sizeof (buffer) -1, 0);
        if (len <= 0) break;

        buffer[len] = '\0';
        if(strncmp(buffer,CMD_READY, strlen(CMD_READY)) == 0){
            update_ready_player(pl,p,1);
            ready_player_broadcast(pl);
        } else if (strncmp(buffer,CMD_UNREADY, strlen(CMD_UNREADY)) == 0){
            update_ready_player(pl,p,0);
            ready_player_broadcast(pl);
        } else {
            printf("Message from %s: %s\n",p->name,buffer);
        }
    }

    remove_player(pl,p);
    leave_broadcast(pl,p);

    close(p->socket_fd);
    free(args);
    return NULL;
}

/**
 * Ecoute et accepte de nouvelles connexion clientes.
 * @param listen_fd Le socket d'écoute
 * @return NULL
 */
void *handle_new_connection(void *argsT){
    while(keepalive){

        ListentThreadArgs *argT = (ListentThreadArgs *)argsT;
        PlayerList *pl = argT->pl;
        int listen_fd = argT->listen_fd;

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        ClientThreadArgs *args = malloc(sizeof(ClientThreadArgs));
        if(args == NULL) {
            free(args);
            perror("ERROR allocation memory for thread args\n");
            continue;
        }

        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd == -1) {
            free(args);
            perror("ERROR accepting connection");
            continue;
        }

        if(!is_full(pl)){
            send(client_fd,SERVER_FULL_MSG, strlen(SERVER_FULL_MSG),0);
            close(client_fd);
            free(args);
            printf("A client tried to connect, but the server is full.\n");
            continue;
        }

        args->pl = pl;
        args->p = create_player(pl,client_fd);

        pthread_t thread_id;
        if (pthread_create(&thread_id,NULL,handle_client,args) != 0){
            perror("ERROR creating thread\n");
            remove_player(args->pl,args->p); // Think to remove player from the pl
            close(client_fd);
            free(args);
            continue;
        }

        pthread_detach(thread_id);
    }
    return NULL;
}

/**
 * Fonction appelé lorsque l'on ferme le programme manuellement avec SIGINT
 * @param sig
 */
void handle_sigint(int sig) {
    keepalive = 0;
    pthread_cond_signal(&keepalive_cond);  // Signale au th
}

/**
 * Initialise et créer le socket d'écoute pour les connection clientes
 * @param port Le port d'écoute
 * @param backlog Le nombre max de connection en attente dans la file de connection
 * @return le file descriptor du socket
 */
int create_listening_socket(int port, int backlog){
    int listen_fd; //socket
    struct sockaddr_in addr; //IPV4
    int opt = 1; // Option pour le socket

    listen_fd = socket(PF_INET,SOCK_STREAM,0);
    if(listen_fd == -1) fatal_error("ERROR listen socket");
    if(setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof (int)) == -1) fatal_error("ERROR opt listen socket");

    memset(&addr,0,sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(listen_fd,(struct sockaddr*)&addr,sizeof addr) == -1) fatal_error("Error binding listen socket");
    if( listen(listen_fd,backlog) == -1) fatal_error("ERROR listening on listen socket");

    printf("Server listening on port %d\n",port);

    return listen_fd;
}

int main(int argc, char* argv[]) {
    if(argc !=3) {
        fprintf(stderr,"Usage : %s <port> <backlog>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    signal(SIGINT,handle_sigint); // Catch signal SIGINT (CTRL +C).

    pthread_mutex_init(&keepalive_mutex,NULL); //Init mutex keepalive for stopping serveur.
    pthread_cond_init(&keepalive_cond,NULL); //Init condition.

    int port = atoi(argv[1]); // Listening port.
    int backlog = atoi(argv[2]); // Max connection on waiting queue.
    int listen_fd = create_listening_socket(port,backlog);

    PlayerList *pl = init_pl(4); // Create the player list

    ListentThreadArgs *args = malloc(sizeof(ClientThreadArgs)); // Listening Thread args
    args->pl = pl;
    args->listen_fd = listen_fd;

    pthread_t tid;
    if (pthread_create(&tid,NULL,handle_new_connection,args) != 0){ // Create listening thread
        perror("ERROR creating thread\n");
        free(args);
        exit(EXIT_FAILURE);
    }

    pthread_cond_wait(&keepalive_cond, &keepalive_mutex);  // wait for the sigint signal

    /* Shutdown server and free ressources*/
    broadcast_message("Le serveur va se fermer, vous allez être déconnecté.\n",pl,NULL,0);
    free_player_list(pl);
    close(listen_fd);

    printf("Serveur fermé\n");
    return 0;
}
