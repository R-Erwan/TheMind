#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "parser.h"
#include "GameState.h"

#define WAIT_DELTA 4

bool keepalive = true;
GameState *gs;
pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_cond = PTHREAD_COND_INITIALIZER;
int waiting = 0;  // Flag pour vérifier si le thread est en attente


int create_socket(const char* ip, int port){
    int socket_fd;
    struct sockaddr_in server_addr;

    socket_fd = socket(PF_INET,SOCK_STREAM,0);
    if(socket_fd == -1){
        perror("ERROR creating socket\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if(inet_pton(AF_INET,ip,&server_addr.sin_addr) <= 0){
        perror("ERROR invalid ip address\n");
        exit(EXIT_FAILURE);
    }

    if(connect(socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
        perror("ERROR connecting to server\n");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

void wake_up_thread() {
    pthread_mutex_lock(&wait_mutex);
    if (waiting) {
        pthread_cond_signal(&wait_cond);  // Réveille le thread en attente
    }
    pthread_mutex_unlock(&wait_mutex);
}

void *handle_reader(void * args) {
    int socket_fd = *(int*)args;
    char partial_msg[BUFSIZ] = "";
    size_t partial_len = 0;

    char buffer[BUFSIZ];
    while(keepalive){
        memset(buffer,0,sizeof(buffer));
        ssize_t len = recv(socket_fd,buffer,sizeof(buffer) - 1, 0);
        if(len <= 0){
            if(len == 0){
                printf("Connection fermé par le serveur.\n");
            } else {
                perror("ERROR receiving message");
            }
            break;
        }
        // Ensure that the commands end with \0. If chains contains \n replace this by \0.
        buffer[len] = '\0';
        strncat(partial_msg,buffer,sizeof(partial_msg) - partial_len -1);
        partial_len = strlen(partial_msg);
        char *start = partial_msg;
        char *newline = strchr(start,'\n');

        while (newline){
            *newline = '\0';
            ServerMsg serveur_msg = parse_stoc(start);
            switch (serveur_msg.code) {
                case GAME_START:
                    gs->nb_p = serveur_msg.param2;
                    break;
                case ROUND_START:
                    gs->round_lvl = serveur_msg.param2;
                    gs->diff = gs->min_card;
                    break;
                case CARD:
                    add_card(gs,serveur_msg.param2);
                    break;
                case GO:
                    gs->play = true;
                    wake_up_thread();
                    break;
                case CARD_PLAY:
                    gs->l_card = serveur_msg.param2;
                    gs->diff = gs->min_card - gs->l_card;
                    if(!isEmpty(gs->cards)){
                        wake_up_thread();
                    }
                    break;
                case LOOSE_ROUND:
                    reset(gs);
                    break;
                case WIN_ROUND:
                    reset(gs);
                    break;
            }

            start = newline + 1;
            newline = strchr(start,'\n');

        }

        if(*start != '\0') {
            memmove(partial_msg,start,strlen(start) + 1);
        } else {
            partial_msg[0] = '\0';
        }


    }
    keepalive = false;
    close(socket_fd);
    return NULL;
}

void *handle_sender(void * args) {
    int socket_fd = *(int*)args;
    char buffer[BUFSIZ];

    while(keepalive){
        if(gs->play == false || isEmpty(gs->cards) == true){
            pthread_mutex_lock(&wait_mutex);
            waiting = 1;
            pthread_cond_wait(&wait_cond,&wait_mutex);
        }
        waiting = 0;
        pthread_mutex_unlock(&wait_mutex);

        memset(buffer,0,sizeof(buffer));

        int diff_p = 99 / (gs->round_lvl * gs->nb_p);
        if(gs->diff < diff_p){

            sleep(WAIT_DELTA);
            snprintf(buffer,sizeof(buffer),"%d",gs->min_card);
            if(send(socket_fd,buffer,strlen(buffer),0) <= 0){
                perror("ERROR sending message");
                break;
            }
            play_card(gs);

        } else {
            int wait_time = ( gs->diff / diff_p ) * WAIT_DELTA;

            pthread_mutex_lock(&wait_mutex);
            waiting = 1;

            struct timespec ts;
            clock_gettime(CLOCK_REALTIME,&ts);
            ts.tv_sec += wait_time;

            int ret = pthread_cond_timedwait(&wait_cond,&wait_mutex, &ts);
            if(ret == 0) { // Thread réveillé par un autre thread
                waiting = 0;
                pthread_mutex_unlock(&wait_mutex);

            } else {
                waiting = 0;
                pthread_mutex_unlock(&wait_mutex);

                snprintf(buffer,sizeof(buffer),"%d",gs->min_card); // Joué la carte
                if(send(socket_fd,buffer,strlen(buffer),0) <= 0){
                    perror("ERROR sending message");
                    break;
                }
                play_card(gs);
            }

        }

    }
    keepalive = false;
    close(socket_fd);

    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 4){
        fprintf(stderr,"Usage : %s <port> <ipv4> <robotName>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    char *ip = argv[2];
    char *name = argv[3];
    // Set les variables global.

    printf("Tentative de connection avec le serveur %s %d ...\n",ip,port);

    int socket_fd = create_socket(ip,port);
    int *socket = malloc(sizeof (int));
    *socket = socket_fd;

    printf("Connection avec le serveur établie !\n");
    gs = create_gameState();

    send(socket_fd,name,5,0);


    // Création des threads
    pthread_t pidReader;
    if(pthread_create(&pidReader,NULL,handle_reader,socket) !=0){
        perror("ERROR : reader thread creation");
        exit(EXIT_FAILURE);
    }

    pthread_t pidWriter;
    if(pthread_create(&pidWriter,NULL,handle_sender,socket) !=0){
        perror("ERROR : sender thread creation");
        exit(EXIT_FAILURE);
    }

    // Attendre la fin des threads
    if (pthread_join(pidReader, NULL) != 0) {
        perror("ERROR : reader thread join");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(pidWriter, NULL) != 0) {
        perror("ERROR : writer thread join");
        exit(EXIT_FAILURE);
    }

    free_GameState(gs);
    free(socket);
    close(socket_fd);

    return 0;
}