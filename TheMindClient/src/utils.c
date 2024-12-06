//
// Created by erwan on 05/12/2024.
//

#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"

#define RED "\033[31m"
#define RESET "\033[0m"
#define WHT "\e[0;37m"

void print_brain_art() {
    printf("                                                THE MIND CLIENT\n");
    printf("===============================================================================================================\n");
    printf(RED"                                               ""@@(@@@@@@@@@@@@@@@@(((                                           \n");
    printf(WHT"                                          @@@@@(@@@@@@((@@@@@@((@@@@@@@@                                        \n");
    printf(RED"                                       @@@@@@@@@@@(@@@@((((@@@@(((((@@@((((                                     \n");
    printf(WHT"                                    @@@@(@@@#(@@@@@@@@((@@@((@@@@@@@@@@@@@@@@                                   \n");
    printf(RED"                                    @@@@(@@@@@@(@@#@@@((@@@@@@(((((((@@@((@@@@                                  \n");
    printf(WHT"                                    @@@@@@(@@@@(@@@@@@@(((((((@@@@@@@@(@@@@@@@(                                 \n");
    printf(RED"                                  @@@(((((@@@@@@@@@@(((#@@@@((@@@@@@@@@@@@@@@(@@                                \n");
    printf(WHT"                                  @@@@(@@@@@(((((((((@@@@@@@@@@@@@@(@@@((((((@@@                                \n");
    printf(RED"                                   @@@@(@@@@((@@@@@@@(@@@@@(@@@@@@((@@@@@@@@@@@@@                               \n");
    printf(WHT"                                     @@@@@(((@@@@(#@@@@@@@(((((((@@@((((@@@(@@@@@                               \n");
    printf(RED"                                         (((@@@@@@((((((((@@@((@@@@@((@@@@@@(@@@                                \n");
    printf(WHT"                                            (@@@@@@@@@@(@@@@@@@(@@@@@@/#@@@@@@@                                 \n");
    printf(RED"                                                @@@@#     @@@@@@//(((//((((                                     \n");
    printf(WHT"                                                             ((//((((//(((                                      \n"RESET);
    printf("=============================================================================================================== \n");
}

void print_file(const char *filename) {
    FILE *file = fopen(filename, "rt");  // Ouvrir le fichier en mode lecture
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier\n");
        return;
    }

    int ch;
    // Lire caractère par caractère
    while ((ch = fgetc(file)) != EOF) {
        // Ignorer le caractère '\r'
        if (ch != '\r') {
            putchar(ch);
        }
    }
    fclose(file);  // Fermer le fichier
}
int parse_stoc(const char* msg){
    const char* start_phrase = "Le fichier de statistiques est disponible.";
    if (strncmp(msg, start_phrase, strlen(start_phrase)) == 0) {
        return PDF_FILE_MESSAGE;
    }
    return -1;
}

pdfFile parse_pdf(const char* msg){
    pdfFile pdf_fi;
    pdf_fi.filename = NULL;
    pdf_fi.port = 0;

    const char* filename_line = strstr(msg, "Nom du fichier : ");
    if (!filename_line) {
        printf("Nom du fichier introuvable\n");
        return pdf_fi;
    }

    filename_line += strlen("Nom du fichier : ");  // Sauter "Nom du fichier : "

    // Extraire le nom du fichier (jusqu'à ".pdf")
    char filename[256];
    if (sscanf(filename_line, "%255s", filename) != 1) {
        printf("Erreur d'extraction du nom du fichier\n");
        return pdf_fi;
    }

    pdf_fi.filename = malloc(strlen(filename) + 1);
    if(!pdf_fi.filename){
        perror("Erreur d'allocation mémoire pour le nom du fichier");
        return pdf_fi;
    }
    strcpy(pdf_fi.filename,filename);


    // Extraire le port
    const char* port_line = strstr(msg, "sur le port ");
    if (!port_line) {
        printf("Port introuvable\n");
        free(pdf_fi.filename);
        pdf_fi.filename = NULL;
        return pdf_fi;
    }

    if (sscanf(port_line, "sur le port %d", &pdf_fi.port) != 1) {
        printf("Erreur d'extraction du port\n");
        return pdf_fi;
    }

    return pdf_fi;
}

void install(){
    // Vérifie et crée le répertoire pdf
    printf("Check installation ...\n");
    struct stat st;
    if (stat("../pdf", &st) != 0) { // Si le répertoire n'existe pas
        if (mkdir("../pdf", 0755) != 0) { // Tente de le créer
            perror("Erreur lors de la création du répertoire pdf");
            exit(EXIT_FAILURE);
        }
        printf("Répertoire 'pdf' créé avec succès.\n");
    } else if (!S_ISDIR(st.st_mode)) { // Vérifie si ce n'est pas un répertoire
        fprintf(stderr, "'pdf' existe mais n'est pas un répertoire.\n");
        exit(EXIT_FAILURE);
    }

    // Vérifie le répertoire ressources
    if (stat("../ressources", &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Erreur : Le répertoire 'ressources' est introuvable.\n");
        exit(EXIT_FAILURE);
    }

    // Vérifie l'existence de rules.txt
    char rules_path[256] = "../ressources/rules.txt";
    if (access(rules_path, F_OK) != 0) { // Si le fichier n'existe pas
        fprintf(stderr, "Erreur : Le fichier '%s' est introuvable.\n", rules_path);
        exit(EXIT_FAILURE);
    }

    // Vérifie l'existence de help_command.txt
    char help_command_path[256] = "../ressources/help_command.txt";
    if (access(help_command_path, F_OK) != 0) { // Si le fichier n'existe pas
        fprintf(stderr, "Erreur : Le fichier '%s' est introuvable.\n", help_command_path);
        exit(EXIT_FAILURE);
    }
    printf("Installation ok\n");
}