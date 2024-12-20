//
// Created by erwan on 15/11/2024.
//
#include "utils.h"

char* format_board(int* board, int size) {
    if (board == NULL || size <= 0) {
        return NULL;
    }

    // Estimation de la taille maximale de la chaîne : "[ , , ,]" (5 * taille + 2 pour les crochets)
    int max_length = size * 5 + 2;
    char* result = (char*)malloc(max_length * sizeof(char));
    if (!result) {
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }

    // Commencer par le crochet ouvrant
    strcpy(result, "[");

    // Construire la chaîne
    for (int i = 0; i < size; i++) {
        char buffer[12]; // Assez pour stocker un entier
        if (board[i] == 0) {
            strcat(result, " "); // Ajouter un espace pour les zéros
        } else {
            snprintf(buffer, sizeof(buffer), "%d", board[i]);
            strcat(result, buffer); // Ajouter la valeur
        }

        // Ajouter une virgule si ce n'est pas le dernier élément
        if (i < size - 1) {
            strcat(result, ",");
        }
    }

    // Ajouter le crochet fermant
    strcat(result, "]\n");

    return result;
}

int ctoint(const char *cmd) {
    char* endptr;  // Pointeur pour stocker l'endroit où la conversion s'arrête

    // Convertir la chaîne en entier
    long int result = strtol(cmd, &endptr, 10);

    // Vérifier si la conversion a réussi
    if (*endptr != '\0') {
        // La chaîne contient des caractères non numériques après l'entier
        return -1; // Retourner une valeur spéciale pour indiquer une erreur
    }

    // Retourner le résultat de la conversion
    return (int)result;
}

int hash_cmd(const char* cmd){
    if(strcmp(cmd,"ready") == 0 || strcmp(cmd,"r") == 0)
        return READY;
    else if (strcmp(cmd,"unready") == 0 || strcmp(cmd,"u") == 0)
        return UNREADY;
    else if (strcmp(cmd,"start") == 0 || strcmp(cmd,"sg") == 0)
        return START;
    else if (strcmp(cmd,"stop") == 0)
        return STOP;
    else if (strcmp(cmd,"addrobot") == 0 || strcmp(cmd,"add robot") == 0)
        return ROBOT_ADD;
    else if (strcmp(cmd,"remove robot") == 0 || strcmp(cmd,"removerobot") == 0)
        return ROBOT_REMOVE;
    else if (strcmp(cmd,"quit") == 0 || strcmp(cmd,"q") == 0)
        return QUIT;
    else if (ctoint(cmd) != -1)
        return CARD;
    else return -1;
}
