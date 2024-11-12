#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#define MAX_CLIENTS 2
#define SERVER_FULL_MSG "Le serveur est plein. Veuillez réessayer plus tard.\n"
#define CMD_READY "ready"
#define CMD_UNREADY "unready"

int *p_listen_fd;  // Pointeur global vers listen_fd pour SIGINT

/* Structure pour identifier un joueur*/
typedef struct {
    int socket_fd;
    char name[50];
    char ready;
} Player;

/* Structure pour gérer une liste de Joueur*/
typedef struct {
    Player players[MAX_CLIENTS];
    int players_count;
    int players_ready;
} PlayerList;

/* Liste des joueurs*/
PlayerList playerList;

/* Verrous pour l'accès aux joueurs*/
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;

/* Affiche un message d'érreur critique et ferme le programme */
void fatal_error(const char* msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * Envoie un message a tous les joueurs du lobby
 * @param msg
 * @param exclude_fd
 */
void broadcast_message(const char* msg, int exclude_fd){
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < playerList.players_count; ++i) {
        if(playerList.players[i].socket_fd != exclude_fd){
            send(playerList.players[i].socket_fd, msg, strlen(msg),0);
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

/**
 * Message de connexion d'un nouveau joueur, broadcast + message console server
 * @param p Le nouveau joueur
 */
void new_player_broadcast(Player *p){
    char message[128];
    snprintf(message,sizeof message,"%s a rejoint la partie ! Joueur connecté %d\n",p->name, playerList.players_count);
    broadcast_message(message,p->socket_fd);
    printf("%s", message);
}

/**
 * Message de dé-connexion d'un nouveau joueur, broadcast + message console server
 * @param p Le nouveau joueur
 */
void player_leave_broadcast(Player *p){
    char message[128];
    snprintf(message, sizeof message, "%s a quitté! Joueurs connectés: %d\n", p->name, playerList.players_count);
    broadcast_message(message, -1);
    printf("%s",message);
}

/**
 * Envoie le nombre de joueur prêt a tout les joueurs.
 */
void player_ready_broadcast(){
    char message[128];
    snprintf(message, sizeof message, "[%d/ %d] joueur prêt\n",playerList.players_ready,playerList.players_count);
    broadcast_message(message, -1);
    printf("%s",message);
}

/**
 * Modifie l'état pret ou non d'un joueur
 * @param player Le joueur a changer d'état
 * @param ready L'état a lui affecté
 */
void handle_ready_cmd(Player *player, uint8_t ready){

    if(player->ready == ready)
        return;

    pthread_mutex_lock(&client_mutex);
    if (ready){
        player->ready = 1;
        playerList.players_ready++;
    } else {
        player->ready = 0;
        playerList.players_ready--;
    }
    pthread_mutex_unlock(&client_mutex);
    player_ready_broadcast();
}

/**
 * Fonction des threads joueurs
 * @param client_sock le socket client
 * @return
 */
void *handle_client(void *client_sock) {
    int client_fd = *(int*)client_sock;
    char message[256];
    free(client_sock);

    pthread_mutex_lock(&client_mutex); // --LOCK client_m--
    Player *player = &playerList.players[playerList.players_count++]; // Créer un nouveau joueur
    player->socket_fd = client_fd;
    player->ready = 0;

    send(client_fd,"Entrez votre nom: ",18,0); //Demande au client de donner un nom, sera peut-être gérer par le programme client
    recv(client_fd,player->name,sizeof(player->name)-1,0);
    player->name[strcspn(player->name,"\n")] = '\0';

    snprintf(message, sizeof message, "Bienvenue %s sur TheMind!\nIl y a %d joueurs\nEnvoyé 'ready' si vous êtes prêt a commencé !\n", player->name, playerList.players_count);
    send(client_fd,message,256,0);
    pthread_mutex_unlock(&client_mutex); // --UNLOCK client_m

    new_player_broadcast(player); // Envoie un message pour annoncer une nouvelle connexion

    char buffer[BUFSIZ];
    while(1){
        ssize_t len = recv(client_fd,buffer,sizeof (buffer) -1, 0);
        if (len <= 0) break;
        buffer[len] = '\0';

        if(strncmp(buffer,CMD_READY, strlen(CMD_READY)) == 0){
            handle_ready_cmd(player,1);
        } else if (strncmp(buffer,CMD_UNREADY, strlen(CMD_UNREADY)) == 0){
            handle_ready_cmd(player,0);
        } else {
            printf("Message from %s: %s\n",player->name,buffer);
        }
    }

    /* Déconnexion du joueur */
    close(client_fd);
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < playerList.players_count; ++i) {
        if(playerList.players[i].socket_fd == client_fd){
            playerList.players[i].socket_fd = playerList.players[--playerList.players_count].socket_fd;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);

    player_leave_broadcast(player);

    return NULL;
}

/**
 * Ecoute et accepte de nouvelles connexion clientes.
 * @param listen_fd Le socket d'écoute
 * @return NULL
 */
void *handle_new_connection(int listen_fd){
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int* client_fd = malloc(sizeof (int));
    *client_fd = accept(listen_fd,(struct sockaddr*)&client_addr,&client_len);

    if(*client_fd == -1) {
        free(client_fd);
        perror("ERROR accepting connection\n");
        return NULL;
    }

    pthread_mutex_lock(&client_mutex); //--LOCK client_m--
    if(playerList.players_count >= MAX_CLIENTS){
        send(*client_fd,SERVER_FULL_MSG, strlen(SERVER_FULL_MSG),0);
        close(*client_fd);
        free(client_fd);
        pthread_mutex_unlock(&client_mutex);
        printf("Un client a tenté de se connecter mais le serveur est plein.\n");
        return NULL;
    }
    pthread_mutex_unlock(&client_mutex);

    pthread_t thread_id;
    if (pthread_create(&thread_id,NULL,handle_client,client_fd) != 0){
        perror("ERROR creating thread\n");
        free(client_fd);
        return NULL;
    }
    pthread_detach(thread_id);
    return NULL;
}

void *manage_games_routine(void *datas){

}

/**
 * Ferme le serveur proprement
 * @param listen_fd le socket d'écoute
 */
void shutdown_server(int listen_fd) {
    const char *shutdown_msg = "Le serveur va se fermer, vous allez être déconnecté.\n";

    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < playerList.players_count; ++i) {
        send(playerList.players[i].socket_fd, shutdown_msg, strlen(shutdown_msg), 0);
        close(playerList.players[i].socket_fd);  // Fermer chaque socket client
    }
    playerList.players_count = 0;
    pthread_mutex_unlock(&client_mutex);

    close(listen_fd);  // Fermer le socket d'écoute du serveur
    printf("Serveur fermé.\n");
}

/**
 * Fonction appelé lorsque l'on ferme le programme manuellement avec SIGINT
 * @param sig
 */
void handle_sigint(int sig) {
    if (p_listen_fd) shutdown_server(*p_listen_fd);  // Utiliser la valeur pointée
    exit(0);  // Quitter proprement
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
    signal(SIGINT,handle_sigint);

    int port = atoi(argv[1]);
    int backlog = atoi(argv[2]);
    int listen_fd = create_listening_socket(port,backlog);
    p_listen_fd = &listen_fd;

    //keepalive ?
    while(1){
        handle_new_connection(listen_fd);
    }

    close(listen_fd);
    return 0;
}
