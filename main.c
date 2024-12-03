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
#include "Game.h"

#define SERVER_FULL_MSG "Le serveur est plein. Veuillez réessayer plus tard.\n"
#define GAME_STARTED_MSG "Une partie est déja en cours. Veuillez réessayer plus tard.\n"
#define PDF_DIR "../pdf"
/**
 * @brief Structure containing arguments for a player management thread.
 *
 * This structure is used to pass the necessary information to a thread
 * responsible for managing a player.
 * It includes a pointer to the player and a reference to the current game.
 */
typedef struct {
    Player *p;
    Game  *game;
} ClientThreadArgs;

/**
 * @brief Structure containing arguments for a listener connection management thread.
 *
 * This structure is used to pass the necessary information to a thread
 * responsible for accepting new connection
 * It includes a pointer to the listening TCP socket and a reference to the current game.
 */
typedef struct{
    int listen_fd;
    Game *game;
} ListentThreadArgs;

volatile int keepalive = 1;
pthread_cond_t keepalive_cond;
pthread_mutex_t keepalive_mutex;

/**
 * @brief Handles a command sent by a player during the game.
 *
 * This function processes a command sent by a player and updates the game
 * state or player's status accordingly. It supports various commands based
 * on the current game state, including setting readiness or playing a card.
 *
 * @param cmd The command string received from the player.
 *            Possible values include "ready", "unready", or a card number.
 * @param g Pointer to the game structure.
 *          Contains the current game state and manages all players.
 * @param p Pointer to the player structure.
 *          Represents the player who sent the command.
 *
 * @note The function sends feedback messages to the player's socket for errors
 *       or invalid commands.
 */
void handle_command(const char* cmd, Game *g, Player *p){
    switch (hash_cmd(cmd)) {
        case READY :
            if(g->state == LOBBY_STATE || g->state == GAME_STATE) {
                if(set_ready_player(g,p,1) == -2)
                    send_p(p,"La partie est déjà en cours\n");

        } else send_p(p,"La partie est déjà en cours\n");
            break;
        case UNREADY :
            if(g->state == LOBBY_STATE || g->state == GAME_STATE) {
                if(set_ready_player(g,p,0) == -2)
                    send_p(p,"La partie est déjà en cours\n");

            } else send_p(p,"La partie est déjà en cours\n");
            break;
        case START:
            if(g->state == LOBBY_STATE) {
                if (start_game(g,p) == -1) // Start game
                    send_p(p, "Tous les joueurs ne sont pas prêt !\n");
            } else if(g->state == GAME_STATE){
                if(start_round(g,p) == -1) // Start round
                    send_p(p,"Tous les joueurs ne sont pas prêt !\n");
            } else if(g->state == GAME_STATE){
                send_p(p,"Vous êtes au milieu d'une manche !\n");
            }
            break;
        case CARD:
            if(g->state == PLAY_STATE && ctoint(cmd) != -1){
                int card = ctoint(cmd);
                if(play_card(g,p,card) == NO_CARD)
                    send_p(p,"Vous n'avez pas la carte %d\n",card);
            }
            break;
        case STOP:
            if(g->state == GAME_STATE) {
                end_game(g,p);
            } else if (g->state == PLAY_STATE){
                send_p(p,"Une manche est en cours !\n");
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
 * @param arg A pointer to a `ClientThreadArgs` structure containing:
 *            - The player (`Player *p`) being managed.
 *            - The game instance (`Game *game`) the player is participating in.
 *
 * @return Always returns `NULL` when the client thread ends.
 *
 * ### Behavior:
 * 1. **Initialization**:
 *    - Sends a welcome message to the client.
 *    - Receives the client's name and sets it in the player list.
 *    - Broadcasts the new player's arrival to all other players.
 *
 * 2. **Command Loop**:
 *    - Continuously listens for commands from the client using `recv`.
 *    - Processes each command using `handle_command`, which updates the game state.
 *    - Handles errors and ensures the command is correctly terminated.
 *
 * 3. **Cleanup**:
 *    - Closes the client's socket.
 *    - Removes the player from the player list.
 *    - If the game is in progress, ends the game or round as necessary.
 *
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
    char message[256] = {0}; // Buffer for inits messages.

    // First welcome message, ask for the name.
    snprintf(message,sizeof message,"Bienvenue sur TheMind ! \nEnvoyé votre nom \n");
    send(p->socket_fd,message,strlen(message),0);

    recv(p->socket_fd,name,sizeof (name) -1, 0);
    if(!set_player_name(pl,p,name)){
        //TODO Traiter le cas ou le nom du client ne vas pas.
    }

    // Second welcome message with player's name.
    snprintf(message, sizeof message, "Bienvenue %s\nIl y a %d joueurs\nEnvoyé 'ready' si vous êtes prêt a commencé !\n", p->name, pl->count);
    send(p->socket_fd,message, strlen(message),0);

    // Broadcast new player message to all player.
    broadcast_message(pl,p,B_CONSOLE,"%s a rejoint la partie ! Joueur connecté %d\n",p->name,pl->count);

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
    broadcast_message(pl,p,B_CONSOLE,"%s a quitté! Joueurs connectés: %d\n",p->name,pl->count -1);
    remove_player(pl,p);

    // End game if needed.
    if(game->state == GAME_STATE ) {
        end_game(game,p);
    }
    if(game->state == PLAY_STATE){
        end_round(game,0);
        end_game(game,p);
    }
    if(game->state == LOBBY_STATE){
        reset_ready_players(pl);
    }

    return NULL;
}
/**
 * @brief Handles a new client connection and manages the client thread creation.
 *
 * This function listens for incoming client connections on the specified
 * listening socket, accepts the connections, and creates a new thread
 * for each accepted client to handle their interactions. It ensures that
 * the server does not exceed the maximum number of players, and it sends
 * appropriate messages if the server is full or if the game has already started.
 *
 * The function runs in a loop, continuously accepting new client connections
 * and delegating the handling of each client to a separate thread.
 *
 * @param LTargs A pointer to a structure containing the listening socket file descriptor
 *               and the current game state. The structure is expected to be of type
 *               `ListentThreadArgs`, which includes:
 *               - `game`: A pointer to the current game instance.
 *               - `listen_fd`: The file descriptor for the listening socket.
 *
 * @return NULL This function does not return a value. It runs in an infinite loop
 *         until the `keepalive` condition is no longer true.
 *
 * @note The function dynamically allocates memory for `ClientThreadArgs`
 *       for each accepted client. This memory is freed after the client thread is created.
 *       If the game is already in progress or the server is full, a message is sent
 *       to the client and the connection is closed without creating a thread.
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

        if (client_fd == -1) {
            free(CTargs);
            perror("ERROR accepting connection");
            continue;
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

void *handle_downloads(void *args){
    int dl_fd = *(int*)args;
    free(args);

    printf("[DL] Server ready to handle new downloading request\n");
    while(keepalive){
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(dl_fd, (struct sockaddr *)&client_addr, &addr_len);
        if(dl_fd < 0) {
            perror("[DL] Error accept");
            continue;
        }

        char buffer[1024] = {0};
        read(client_fd,buffer,sizeof(buffer));
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
 *
 * This function is designed to handle the SIGINT signal (typically triggered by
 * pressing Ctrl+C in the terminal). When the signal is received, it sets the
 * global `keepalive` variable to 0, effectively signaling the server to stop.
 * Additionally, it signals a condition variable `keepalive_cond` to notify any
 * waiting threads to proceed and handle the termination process.
 *
 * @param sig The signal number (in this case, SIGINT). This parameter is
 *            provided by the signal handler system, but is not used directly
 *            in this function.
 *
 * @note The global variable `keepalive` is checked by main thread in
 *       the application to determine when to terminate. The `keepalive_cond`
 *       condition variable is used to synchronize this termination process
 *       with main thread.
 */
void handle_sigint(int sig) {
    keepalive = 0;
    pthread_cond_signal(&keepalive_cond);  // Signale au th
}
/**
 * @brief Creates and binds a listening socket for the server.
 *
 * This function creates a socket that will be used to listen for incoming
 * client connections on the specified port. It sets the socket option to
 * allow address reuse and binds the socket to the provided port. Finally,
 * it listens for incoming connections with the specified backlog size.
 *
 * @param port The port number to bind the server socket to.
 * @param backlog The maximum number of pending connections allowed in the
 *                socket's listen queue.
 *
 * @return The file descriptor of the listening socket.
 *
 * @note This function will exit the program with a failure status if any
 *       error occurs during socket creation, option setting, binding, or
 *       listening.
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
/**
 * @brief Entry point for the server program.
 *
 * This is the main function of the server. It initializes necessary resources,
 * such as mutexes and condition variables, and sets up signal handling. The
 * function then creates a listening socket and initializes the game and player
 * list. It starts a thread to handle incoming client connections, waits for
 * the server shutdown signal (SIGINT), and then shuts down the server, cleaning
 * up resources.
 *
 * @param argc The number of command-line arguments passed to the program.
 * @param argv An array of strings representing the command-line arguments.
 *
 * @return Returns 0 if the server is shut down successfully.
 *
 * @note The server listens for incoming connections, and when SIGINT (CTRL+C)
 *       is received, it shuts down the server, broadcasting a disconnection
 *       message to all clients before freeing all resources.
 */
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
    int backlog = atoi(argv[2]); // Max connection on waiting queue.
    int listen_fd = create_listening_socket(port,backlog); // Listening socket to handle connection

    PlayerList *pl = init_pl(4); // Create the player list
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
    broadcast_message(pl,NULL,B_CONSOLE,"Le serveur va se fermer, vous allez être déconnecté.\n");
    free_player_list(pl);
    free_game(g);
    close(listen_fd);

    printf("Serveur fermé\n");
    return 0;
}

