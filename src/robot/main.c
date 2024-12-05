#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

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

int main(int argc, char **argv)
{
    const char *ip = "127.0.0.1";           // Adresse IP du serveur (ici localhost)
    int port = atoi(argv[1]);              // Port sur lequel le serveur écoute
    int actif = 1;
    char message[1024];

    // Créer le socket client et se connecter au serveur
    int client_socket = create_client_socket(ip, port);

    while(actif) {
        // Réception du message du serveur
        char buffer[1024];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1) {
            perror("ERROR receiving message");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        buffer[bytes_received] = '\0';  // Terminer la chaîne de caractères
        printf("%s", buffer);

        //Envoi de données
        scanf("%s", message);
        if(strcmp("stop", message) == 0)
            actif = 0;
        else if (send(client_socket, message, strlen(message), 0) == -1) {
            perror("ERROR sending message");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }

    // Fermer la socket
    close(client_socket);
    return 0;
}
