#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "utils.h"
#define RULES_FILE "./ressources/rules.txt"
#define HELP_FILE "./ressources/help_command.txt"

bool keepalive = true;
char* s_ip;
int s_port;

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

void download_pdf(const char* filename,int port,const char* ip){
    int socket_fd = create_socket(ip,port);
    if(socket_fd < 0){
        perror("Erreur lors de la connection au serveur de téléchargement");
        return;
    }
    printf("Connection avec le serveur de téléchargement établie...\n");

    char msg[512];
    snprintf(msg,sizeof(msg),"getfile %s", filename);

    if (send(socket_fd,msg, strlen(msg),0) < 0){
        perror("Erreur lors de l'envoie de la requête");
        close(socket_fd);
        return;
    }

    char buffer[BUFSIZ];
    ssize_t b_received;
    char out_filename[256];
    snprintf(out_filename,sizeof(out_filename),"./pdf/%s",filename);

    FILE* file = fopen(out_filename,"wb");
    if(file == NULL){
        perror("Erreur lors de l'ouverture du fichier local");
        close(socket_fd);
        return;
    }

    printf("Téléchargement en cours vers %s ...\n",out_filename);
    while((b_received = recv(socket_fd,buffer,sizeof(buffer),0)) > 0) {
        if(fwrite(buffer,1,b_received,file) != (size_t)b_received) {
            perror("Erreur lors de l'écriture dans le fichier");
            fclose(file);
            close(socket_fd);
            return;
        }
    }

    if(b_received < 0){
        perror("Erreur lors de la réception des données");
    } else {
        printf("Téléchargement terminé avec succès.\n");
    }

    fclose(file);
    close(socket_fd);
}

void *handle_reader(void * args) {
    int socket_fd = *(int*)args;

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
        printf("%s",buffer);

        // Si message fichier pdf disponible
        if(parse_stoc(buffer) == PDF_FILE_MESSAGE){
            pdfFile pdfi = parse_pdf(buffer);
            if(pdfi.filename){
                download_pdf(pdfi.filename,s_port+1,s_ip);
                free(pdfi.filename); // Libérer la mémoire allouée
            } else {
                printf("Erreur lors de l'analyse du message PDF\n");
            }
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
        memset(buffer,0,sizeof(buffer));
        scanf("%s",buffer);

        if(strcmp("help",buffer) == 0 ){
            print_file(HELP_FILE);
        } else if(strcmp("quit",buffer) == 0){
            shutdown(socket_fd,SHUT_RD);
            break;
        } else {
            if(send(socket_fd,buffer,strlen(buffer),0) <= 0){
                perror("ERROR sending message");
                break;
            }
        }
    }
    keepalive = false;
    close(socket_fd);

    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 3){
        fprintf(stderr,"Usage : %s <port> <ipv4>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    char *ip = argv[2];
    // Set les variables global.
    s_ip = ip;
    s_port = port;
    install();

    print_brain_art();
    print_file(RULES_FILE);
    printf("Tentative de connection avec le serveur %s %d ...\n",ip,port);

    int socket_fd = create_socket(ip,port);
    int *socket = malloc(sizeof (int));
    *socket = socket_fd;

    printf("Connection avec le serveur établie !\n");


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

    free(socket);
    close(socket_fd);

    return 0;
}