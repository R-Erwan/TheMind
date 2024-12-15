#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "playersRessources.h"
#include "ANSI-color-codes.h"
#include "Game.h"

#define MAX_PLAYERS 4
#define SERVER_FULL_MSG "Le serveur est plein. Veuillez réessayer plus tard.\n"
#define GAME_STARTED_MSG "Une partie est déja en cours. Veuillez réessayer plus tard.\n"
#define PDF_DIR "../pdf"
#define ROBOTIA_dir "../TheMindRobot/build-robot/TheMindRobotIA"
/**
 * @brief Structure containing arguments for a player management thread.
 */
typedef struct {
    Player *p;
    Game  *game;
} ClientThreadArgs;
/**
 * @brief Structure containing arguments for a listener connection management thread.
 */
typedef struct{
    int listen_fd;
    Game *game;
} ListentThreadArgs;

volatile bool keepalive = true; // Boolean for managing listening et downloading thread.
pthread_cond_t keepalive_cond;
pthread_mutex_t keepalive_mutex;
int s_port; // Global variable listening port

/**
 * @brief Start robot program, the robot quit after the end of the game.
 * @param robot_name Robot's name.
 */
void start_robot(char* robot_name){
    pid_t pid = fork();
    if(pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char port_str[6];
        snprintf(port_str,sizeof(port_str),"%d",s_port);
        execl(ROBOTIA_dir,ROBOTIA_dir,port_str,"127.0.0.1",robot_name,"0",NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }
}
/**
 * @brief Handles a command sent by a player.
 *
 * @param cmd The command string received from the player.
 * @param g Pointer to the game structure.
 * @param p Pointer to the player who send the command.
 */
void handle_command(const char* cmd, Game *g, Player *p){
//    printf("%s : %s\n",p->name,cmd);
    switch (hash_cmd(cmd)) {
        case READY :
            if(g->state == LOBBY_STATE || g->state == GAME_STATE) {
                if(set_ready_player(g,p,1) == -2)
                    send_p(p,RED"La partie est déjà en cours\n"CRESET);
        } else send_p(p,RED"La partie est déjà en cours\n"CRESET);
            break;
        case UNREADY :
            if(g->state == LOBBY_STATE || g->state == GAME_STATE) {
                if(set_ready_player(g,p,0) == -2)
                    send_p(p,RED"La partie est déjà en cours\n"CRESET);

            } else send_p(p,RED"La partie est déjà en cours\n"CRESET);
            break;
        case START:
            if(g->state == LOBBY_STATE) {
                if (start_game(g,p) == -1) // Start game
                    send_p(p, RED"Tous les joueurs ne sont pas prêt !\n"CRESET);
            } else if(g->state == GAME_STATE){
                if(start_round(g,p) == -1) // Start round
                    send_p(p,RED"Tous les joueurs ne sont pas prêt !\n"CRESET);
            } else if(g->state == PLAY_STATE){
                send_p(p,RED"Vous êtes au milieu d'une manche !\n"CRESET);
            }
            break;
        case CARD:
            if(g->state == PLAY_STATE && ctoint(cmd) != -1){
                int card = ctoint(cmd);
                if(play_card(g,p,card) == NO_CARD)
                    send_p(p,RED"Vous n'avez pas la carte %d\n"CRESET,card);
            }
            break;
        case STOP:
            if(g->state == GAME_STATE) {
                end_game(g,p,false);
            } else if (g->state == PLAY_STATE){
                send_p(p,RED"Une manche est en cours !\n"CRESET);
            }
            break;
        case ROBOT_ADD:
            if(g->state == LOBBY_STATE){
                if(g->playerList->count < g->playerList->max){
                    char name[50];
                    snprintf(name, sizeof(name),"Robot%d",g->playerList->count);
                    start_robot(name);
                } else {
                    send_p(p,RED"Le lobby est déja plein !\n"CRESET);
                }
            } else {
                send_p(p,RED"Vous ne pouvez ajouter un robot uniquement dans le lobby\n"CRESET);
            }
            break;
        default:
            printf("%s a envoyé : %s\n",p->name,cmd);
    }
}
/**
 * @brief Handles a client connection in a separate thread.
 *
 * This function manages the communication with a single client, including
 * initializing the player, handling commands, broadcasting messages to other
 * players, and cleaning up when the client disconnects.
 *
 * @param arg A pointer to a `ClientThreadArgs` structure.
 * @return Always returns `NULL` when the client thread ends.
 * @note This function frees the memory allocated for the `ClientThreadArgs` structure.
 * @warning This function must be called in a separate thread for each client.
 */
void *handle_client(void *arg) {
    // Extracted variable from arg.
    ClientThreadArgs *args = (ClientThreadArgs *)arg;
    Player *p = args->p;
    Game *game = args->game;
    PlayerList *pl = game->playerList;
    free(args);

    char name[50] = {0}; // Buffer for player's name.

    // First welcome message, ask for the name.
    send_p(p,"Bienvenue sur TheMind ! \nEnvoyé votre nom\n");

    recv(p->socket_fd,name,sizeof (name) -1, 0);
    set_player_name(pl,p,name);

    broadcast_message(pl,NULL,B_CONSOLE,GRN"\n%s a rejoint !\n\n"CRESET,p->name);
    print_lobbyState(game); // Send lobby message broadcast

    // Loop on client commands
    char buffer[BUFSIZ];
    while(1){
        memset(buffer,0,sizeof(buffer));
        ssize_t len = recv(p->socket_fd,buffer,sizeof (buffer) -1, 0);
        if (len <= 0) break;

        // Ensure that the commands end with \0. If chains contains \n replace this by \0.
        buffer[len] = '\0';
        char* end = strchr(buffer, '\n');
        if (end) *end = '\0';

        // Call the command handler
        handle_command(buffer,game,p);
    }

    // Cleanup player. @warning the order is important here.
    close(p->socket_fd);
    broadcast_message(pl,p,B_CONSOLE,GRN"\n%s a quitté!\n\n"CRESET,p->name);

    // End game if needed.
    if(game->state == GAME_STATE ) {
        end_game(game,p,true);
    }
    if(game->state == PLAY_STATE){
        end_round(game,0);
        end_game(game,p,true);
    }

    remove_player(pl,p);
    print_lobbyState(game);
    return NULL;
}
/**
 * @brief Handles a new client connection and manages the client thread creation.
 *
 * This function listens for incoming client connections on the specified
 * listening socket, accepts the connections, and creates a new thread.
 *
 * @param LTargs A pointer to a `ListentThreadArgs` structure.
 * @return NULL This function does not return a value. It runs in an infinite loop
 *         until the `keepalive` condition is no longer true.
 * @note The function dynamically allocates memory for `ClientThreadArgs`
 *       for each accepted client. This memory is freed after the client thread is created.
 */
void *handle_new_connection(void *LTargs){
    ListentThreadArgs *arg_in = (ListentThreadArgs *)LTargs;

    Game *game = arg_in->game;
    int listen_fd = arg_in->listen_fd;

    free(LTargs);

    while(keepalive){
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        ClientThreadArgs *CTargs = malloc(sizeof(ClientThreadArgs));
        if(CTargs == NULL) {
            free(CTargs);
            perror("ERROR allocation memory for thread args\n");
            continue;
        }

        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if(client_fd < 0){
            if (errno == EBADF || errno == EINTR) { // EBADF : socket fermée
                free(CTargs);
                printf("[AC] Socket fermée, arrêt du thread.\n");
                break;
            } else {
                free(CTargs);
                perror("ERROR accepting connection");
                continue;
            }
        }


        if(!is_full(game->playerList)){
            send(client_fd,SERVER_FULL_MSG, strlen(SERVER_FULL_MSG),0);
            close(client_fd);
            free(CTargs);
            printf("A client tried to connect, but the server is full.\n");
            continue;
        }

        if(game->state == GAME_STATE || game->state == PLAY_STATE){
            send(client_fd,GAME_STARTED_MSG, strlen(GAME_STARTED_MSG),0);
            close(client_fd);
            free(CTargs);
            printf("A client tried to connect, but the game is started.\n");
            continue;
        }

        CTargs->game = game;
        CTargs->p = create_player(game->playerList,client_fd);
        if(CTargs->p == NULL){
            send(client_fd,SERVER_FULL_MSG, strlen(SERVER_FULL_MSG),0);
            close(client_fd);
            free(CTargs);
            printf("A client tried to connect, but the server is full.\n");
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id,NULL,handle_client,CTargs) != 0){
            perror("ERROR creating thread\n");
            remove_player(CTargs->game->playerList,CTargs->p); // Think to remove player from the pl
            close(client_fd);
            free(CTargs);
            continue;
        }

        pthread_detach(thread_id);
    }
    return NULL;
}
/**
 * @brief Handle requests for pdf stats file download
 * @param args Pointer to the listening socket
 * @return NULL This function does not return a value. It runs in an infinite loop
 *         until the `keepalive` condition is no longer true.
 */
void *handle_downloads(void *args){
    int dl_fd = *(int*)args;
    free(args);

    printf("[DL] Server ready to handle new downloading request\n");
    while(keepalive){
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(dl_fd, (struct sockaddr *)&client_addr, &addr_len);
        if(dl_fd < 0) {
            if (errno == EBADF || errno == EINTR) { // EBADF : socket fermée
                printf("[DL] Socket fermée, arrêt du thread.\n");
                break;
            } else {
                perror("[DL] Error accept");
                continue;
            }
        }

        char buffer[1024] = {0};
        ssize_t read_size =  read(client_fd,buffer,sizeof(buffer) -1 );
        if(read_size <= 0){
            perror("[DL] reading error");
            close(client_fd);
            continue;
        }
        printf("[DL] Requête reçue : %s\n",buffer);

        // send file if valide request
        if(strncmp(buffer, "getfile",7) == 0){
            char *filename = buffer + 8;
            filename[strcspn(filename,"\n")] = '\0';
            char filepath[256];
            snprintf(filepath,sizeof(filepath),PDF_DIR"/%s",filename);
            FILE *file = fopen(filepath,"rb");
            if (file){
                char file_buf[1024];
                size_t bytes_read;
                while ((bytes_read = fread(file_buf,1, sizeof(file_buf),file)) > 0 ){
                    send(client_fd,file_buf,bytes_read,0);
                }
                fclose(file);
                printf("[DL] Fichier %s envoyé avec succès\n",filename);
            } else {
                write(client_fd,"Erreur : fichier non trouvé\n",28);
            }
        } else {
            write(client_fd,"Commande invalide\n",19);
        }
        close(client_fd);
    }

    return NULL;
}
/**
 * @brief Handles the SIGINT (interrupt) signal by stopping the server.
 * @note The global variable `keepalive` is checked by main thread in
 *       the application to determine when to terminate.
 */
void handle_sigint(int sig) {
    keepalive = 0;
    pthread_cond_signal(&keepalive_cond);  // Signale au th
}
/**
 * @brief Creates and binds a listening socket for the server.
 *
 *
 * @param port The port number to bind the server socket to.
 * @param backlog The maximum number of pending connections allowed in the
 *                socket's listen queue.
 *
 * @return The file descriptor of the listening socket.
 * @note This function will exit the program with a failure status if any
 *       error occurs.
 */
int create_listening_socket(int port, int backlog){
    int listen_fd; //socket
    struct sockaddr_in addr; //IPV4
    int opt = 1; // Option pour le socket

    listen_fd = socket(PF_INET,SOCK_STREAM,0);
    if(listen_fd == -1){
        perror("ERROR listening socket");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof (int)) == -1){
        perror("ERROR opt listen socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr,0,sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(listen_fd,(struct sockaddr*)&addr,sizeof addr) == -1) {
        perror("Error binding listen socket");
        exit(EXIT_FAILURE);
    }

    if( listen(listen_fd,backlog) == -1){
        perror("ERROR listening on listen socket");
        exit(EXIT_FAILURE);
    }

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

    srand(time(NULL)); // Init random seed.

    int port = atoi(argv[1]); // Listening port.
    s_port = port;
    int backlog = atoi(argv[2]); // Max connection on waiting queue.
    int listen_fd = create_listening_socket(port,backlog); // Listening socket to handle connection

    PlayerList *pl = init_pl(MAX_PLAYERS); // Create the player list
    Game *g = create_game(pl); // Create the game management system.

    /**
     * Listening Thread.
     */
    ListentThreadArgs *LTargs = malloc(sizeof(ListentThreadArgs)); // Listening Thread args
    LTargs->game = g;
    LTargs->listen_fd = listen_fd;

    pthread_t tid;
    if (pthread_create(&tid,NULL,handle_new_connection,LTargs) != 0){ // Create listening thread
        perror("ERROR creating thread\n");
        free(LTargs);
        exit(EXIT_FAILURE);
    }

    /**
     *  Download Thread.
     */
    int port2 = atoi(argv[1]) + 1;
    int download_fd = create_listening_socket(port2,backlog);
    int *dl_arg = malloc(sizeof(int));
    *dl_arg = download_fd;

    pthread_t tid_dl;
    if(pthread_create(&tid_dl,NULL,handle_downloads,dl_arg) != 0){
        perror("ERROR creating downloading handler thread\n");
        free(dl_arg);
        exit(EXIT_FAILURE);
    }

    pthread_cond_wait(&keepalive_cond, &keepalive_mutex);  // wait for the sigint signal

    /* Shutdown server and free ressources*/
    broadcast_message(pl,NULL,B_CONSOLE,RED"\nLe serveur va se fermer, vous allez être déconnecté.\n\n"CRESET);
    disconnect_allP(pl); // Close all clients socket.

    shutdown(listen_fd,SHUT_RDWR);
    close(listen_fd); // Close listening socket.
    shutdown(download_fd,SHUT_RDWR);
    close(download_fd); // Close downloading socket.

    pthread_join(tid,NULL);
    pthread_join(tid_dl,NULL);

    free_player_list(pl);
    free_game(g);

    printf("Serveur fermé\n");
    return 0;
}