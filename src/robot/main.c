#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>

bool keepalive = true;
bool PartieEnCours = false;
int nb_cards = 0;

typedef struct{
    int socket_fd;
    int *cards;
}robotParams;

int create_client_socket(const char *ip, int port) {
    int socketfd;
    struct sockaddr_in server_addr;

    // Créer la socket
    socketfd = socket(PF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("ERROR creating socket");
        exit(EXIT_FAILURE);
    }

    // Préparer l'adresse du serveur
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("ERROR invalid address");
        exit(EXIT_FAILURE);
    }

    // Se connecter au serveur
    if (connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("ERROR connecting to server");
        exit(EXIT_FAILURE);
    }
    return socketfd;
}

void *GameHandler(void *args)
{
    int i = 0;
    char *str;
    robotParams *rp = (robotParams*)args;
    while(PartieEnCours && i < 5 && rp->cards[i] != 0){
        sleep(rp->cards[i] /5);
        if(PartieEnCours){
            str = (char*)malloc(12);
            if (str == NULL) {
                perror("Erreur d'allocation mémoire");
                exit(EXIT_FAILURE);
            }
            sprintf(str, "%d\n", rp->cards[i]);
            if (send(rp->socket_fd,str, sizeof(str),0) == -1)
            {
                perror("ERROR sending message");
                free(rp);
                free(str);
                close(rp->socket_fd);
                exit(EXIT_FAILURE);
            }
            free(str);
        }
        i++;
    }
    free(rp);
}

void parse_robot_messages(int socket_fd, char *buffer, int *card_list)
{
    pthread_t gameHandler;
    robotParams *rp;
    if(strcmp(buffer, "Envoyé votre nom\0") == 0){
        if (send(socket_fd,"Robot", 5,0) == -1)
        {
            perror("ERROR sending message");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }
    } else if(strcmp(buffer, "Envoyé 'ready' si vous êtes prêt a commencé !\0") == 0){
        if (send(socket_fd,"ready", 5,0) == -1)
        {
            perror("ERROR sending message");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }
    } else if(strcmp(buffer, "Distribution des cartes...\0") == 0){
        for (int i = 0; i < 5; i++) {
            card_list[i] = 0;
        }
        nb_cards = 0;
    } else if (strncmp(buffer, "Carte", 5) == 0){
        card_list[nb_cards] = atoi(buffer + 8);
        printf("ici: %d\n", card_list[nb_cards]);
        nb_cards++;
    } else if (strncmp(buffer, "Go", 2) == 0){
        rp = malloc(sizeof(robotParams)); // Allocation dynamique
        if (rp == NULL) {
            perror("ERROR: Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        rp->socket_fd = socket_fd;
        rp->cards = card_list;
        PartieEnCours = true;
        if(pthread_create(&gameHandler,NULL,GameHandler,rp) !=0){
            perror("ERROR : reader thread creation");
            exit(EXIT_FAILURE);
        }
    }else if (strncmp(buffer, "Mince!\0",5) == 0) {
        PartieEnCours = false;
    }
}

void *handle_reader(void * args) {
    int socket_fd = *(int*)args;
    int *card_list = (int *)calloc(5, sizeof(int));

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
        buffer[len -1] = '\0';
        printf("%s\n", buffer);
        parse_robot_messages(socket_fd, buffer, card_list);
    }
    keepalive = false;
    free(card_list);
    close(socket_fd);
    return NULL;
}

int main(int argc, char **argv)
{
    const char *ip = "127.0.0.1";           // Adresse IP du serveur (ici localhost)
    int port = atoi(argv[1]);              // Port sur lequel le serveur écoute
    int actif = 1;
    char message[1024];

    // Créer le socket client et se connecter au serveur
    int robot_socket = create_client_socket(ip, port);

    pthread_t pidReader;
    if(pthread_create(&pidReader,NULL,handle_reader,&robot_socket) !=0){
        perror("ERROR : reader thread creation");
        exit(EXIT_FAILURE);
    }

    // Attendre la fin des threads
    if (pthread_join(pidReader, NULL) != 0) {
        perror("ERROR : reader thread join");
        exit(EXIT_FAILURE);
    }

    // Fermer la socket
    close(robot_socket);
    return 0;
}
