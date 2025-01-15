//
// Created by Erwan on 23/11/2024.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "statsManager.h"

/**
 * @brief Creates and initializes a new GameData structure.
 *
 * @return Pointer to the newly allocated GameData structure, or NULL if memory allocation fails.
 */
GameData *create_gm() {
    GameData *gm = malloc(sizeof (GameData));
    if(!gm){
        fprintf(stderr,"Erreur : impossible d'allouer la mémoire pour GameData\n");
        return NULL;
    }
    gm->player_count = 0;
    gm->rounds = 0;
    gm->win_rounds = 0;
    gm->max_round_lvl = 0;
    for (int i = 0; i < 100; i++) {
        gm->loosing_cards[i] = 0;
        gm->avg_reaction_time[i] = 0.0;
        gm->cards[i] = 0;
    }
    gm->round_list = NULL;

    return gm;
}

/**
 * @brief Frees the memory allocated for a GameData structure.
 *
 * @param gm Pointer to the GameData structure to be freed.
 */
void free_gm(GameData *gm) {
    if(gm){
        free(gm->round_list);
        free(gm);
    }
}

/**
 * @brief Create FILE to store datas.
 * @param gm The Game Data struct associate.
 * @return A pointer to the data_fp.
 */
char *create_uf(GameData *gm) {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char date[20]; // YYYY-MM-DD + '\0'
    strftime(date, sizeof(date), "%Y-%m-%d-%H_%M_%S", local);
    char path[30]; // ../datas/ + date + '\0'
    snprintf(path, sizeof(path), DATA_DIR"/%s", date);

    FILE *file = fopen(path,"w");
    if (!file) {
        perror("Erreur lors de la création du fichier ");
        return NULL;
    }
    fclose(file);

    if(chmod(path,0777) !=0){
        perror("Erreur lors de la definition des permissions du fichier");
        return NULL;
    }

    strncpy(gm->data_fp,path,sizeof(path));
    return gm->data_fp;
}

/**
 * @brief Adds a card play to the GameData structure and updates the reaction time for the card.
 *
 * @param gm Pointer to the GameData structure.
 * @param card The card ID to be added.
 * @param reaction_time The reaction time in milliseconds.
 */
void add_card(GameData *gm, int card, time_t reaction_time) {
    // Incrémente le nombre de fois que la carte a été jouée
    gm->cards[card]++;
    int nbc = gm->cards[card]; // Nouveau nombre de cartes jouées
    double avgt = gm->avg_reaction_time[card]; // Ancien temps moyen

    // Recalcule le nouveau temps de réaction moyen
    gm->avg_reaction_time[card] = (nbc - 1) * avgt + (int)reaction_time;
    gm->avg_reaction_time[card] /= nbc;
}

/**
 * @brief Adds a new round to the GameData structure, updating relevant statistics.
 *
 * @param gm Pointer to the GameData structure.
 * @param round_lvl The level of the round played.
 * @param win Boolean indicating if the round was won (1 for win, 0 for loss).
 */
void add_round(GameData *gm, int round_lvl, int win) {
    if(!gm) return;
    int *new_list = realloc(gm->round_list, (gm->rounds + 1) * sizeof(int));
    if(!new_list) {
        fprintf(stderr,"Erreur impossible d'allouer la mémoire pour round_list\n");
        return;
    }
    gm->round_list = new_list;
    gm->round_list[gm->rounds] = round_lvl;

    gm->rounds++;
    if(win) {
        gm->win_rounds++;
        if(round_lvl>gm->max_round_lvl) gm->max_round_lvl = round_lvl;
    }

}

/**
 * @brief Records a losing card and updates its reaction time statistics.
 *
 * @param gm Pointer to the GameData structure.
 * @param card The ID of the losing card.
 * @param reaction_time The reaction time in milliseconds.
 */
void add_loosing_card(GameData *gm, int card, time_t reaction_time){
    gm->loosing_cards[card]++;
    add_card(gm,card,reaction_time);
}

int write_data_to_file(GameData *gm) {
    FILE *file = fopen(gm->data_fp,"w");

    if (!file) {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier de stats\n");
        return -1;
    }

    // Ligne pour le nombre de joueurs
    fprintf(file, "PLAYER %d\n", gm->player_count);

    // Ligne pour le nombre de rounds
    fprintf(file, "ROUNDS %d\n", gm->rounds);

    // Ligne pour le nombre de rounds gagnés
    fprintf(file, "WINROUNDS %d\n", gm->win_rounds);

    // Ligne pour le niveau maximum atteint
    fprintf(file, "MAXROUNDS %d\n", gm->max_round_lvl);

    // Ligne pour les cartes perdues
    fprintf(file, "LOOSINGCARD");
    for (int i = 0; i < 100; i++) {
        fprintf(file, " %d", gm->loosing_cards[i]);
    }
    fprintf(file, "\n");

    // Ligne pour le temps de réaction moyen global
    double total_reaction_time = 0;
    int total_cards_played = 0;
    for (int i = 0; i < 100; i++) {
        total_reaction_time += gm->avg_reaction_time[i] * gm->cards[i];
        total_cards_played += gm->cards[i];
    }
    double avg_reaction_time = total_cards_played > 0 ? total_reaction_time / total_cards_played : 0;
    fprintf(file, "REACTIONTIME %.2f\n", avg_reaction_time);

    // Ligne pour le temps de réaction moyen par carte
    fprintf(file, "REACTIONPERCARD");
    for (int i = 0; i < 100; i++) {
        fprintf(file, " %.2f", gm->avg_reaction_time[i]);
    }
    fprintf(file, "\n");

    // Ligne pour la liste des rounds joués
    fprintf(file, "ROUNDSLIST");
    for (int i = 0; i < gm->rounds; i++) {
        fprintf(file, " %d", gm->round_list[i]);
    }
    fprintf(file, "\n");

    // Ligne pour le nombre de fois où chaque carte a été jouée
    fprintf(file, "CARDSPLAYED");
    for (int i = 0; i < 100; i++) {
        fprintf(file, " %d", gm->cards[i]);
    }
    fprintf(file, "\n");

    fclose(file);
    return 0;
}

/**
 * @brief Executes a script to generate a data graph using the provided file path.
 *
 * @param datas_fp Path to the data file to be used by the script.
 * @return 0 on success, or -1 if an error occurs.
 */
int make_dg(const char* datas_fp){
    char cmd[512];
    int ret;
    snprintf(cmd,sizeof(cmd), "./scripts/make_dg.sh %s",datas_fp);
    ret = system(cmd);
    if (ret == -1) {
        perror("Erreur lors de l'exécution de la commande");
        return -1;  // Indique une erreur d'exécution
    }

    // Vérifier le code de retour du script
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        printf("Script exécuté avec succès.\n");
        return 0;  // Succès
    } else {
        printf("Le script a rencontré une erreur. Code de retour : %d\n", WEXITSTATUS(ret));
        return -1;  // Erreur
    }

}

/**
 * @brief Executes a script to generate a PDF using the provided data file path.
 *
 * @param data_fp Path to the data file to be used by the script.
 * @return 0 on success, or -1 if an error occurs.
 */
int make_pdf(const char *data_fp) {
    char cmd[512];
    char latex_f[] = "./ressources/main.tex";
    int ret;
    snprintf(cmd,sizeof(cmd),"./scripts/make_pdf.sh %s %s",data_fp,latex_f);
    ret = system(cmd);
    if(ret == -1) {
        perror("Erreur lors de l'éxécution de la commande");
        return -1;
    }

    // Vérifier le code de retour du script
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        printf("Script exécuté avec succès.\n");
        return 0;  // Succès
    } else {
        printf("Le script a rencontré une erreur. Code de retour : %d\n", WEXITSTATUS(ret));
        return -1;  // Erreur
    }
}

/**
 * @brief add a game in the rank.dat file
 * @param gm The game data
 * @param p_names All the players name in this game
 *
 * @warning this function need the script add_rank.sh.
 * @return 0 if it was succes, -1 if errrors occurs
 */
int write_game_rank(GameData* gm, char **p_names){

    // Format players name
    size_t pnt = 1;
    for (int i = 0; i < gm->player_count; i++) {
        pnt += strlen(p_names[i]) + 1;
    }

    char *format_pn = malloc(pnt);
    if(format_pn == NULL){
        perror("ERROR : Memory allocation");
        return -1;
    }

    format_pn[0] = '\0';
    for (int i = 0; i < gm->player_count; i++) {
        strcat(format_pn,p_names[i]);
        if(i < gm->player_count - 1){
            strcat(format_pn, " ");
        }
    }

    char cmd[BUFSIZ];
    int ret;
    snprintf(cmd,sizeof(cmd),"./scripts/add_rank.sh %d %d %s",gm->player_count,gm->max_round_lvl,format_pn);
    ret = system(cmd);

    free(format_pn);

    if(ret == -1) {
        perror("Erreur lors de l'éxécution de la commande");
        return -1;
    }

    // Vérifier le code de retour du script
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        printf("Script exécuté avec succès.\n");
        return 0;  // Succès
    } else {
        printf("Le script a rencontré une erreur. Code de retour : %d\n", WEXITSTATUS(ret));
        return -1;  // Erreur
    }

}

/**
 * @brief return array with a top10 rank, base on the player count
 * @param nb_p Number of player.
 * @param line_count A pointer to an int, used to know how many lines where returns.
 * @warning The script top10.sh need to be accessible.
 * @return A string list of game's rank.
 */
char** get_top10(int nb_p, int *line_count) {
    // Commande à exécuter
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./scripts/top10.sh %d", nb_p);

    // Ouvrir le processus pour lire la sortie du script
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Erreur lors de l'exécution du script");
        return NULL;
    }

    // Stocker chaque ligne dans un tableau dynamique
    char **lines = NULL;
    char buffer[256];
    *line_count = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        lines = realloc(lines, (*line_count + 1) * sizeof(char*));
        if (lines == NULL) {
            perror("Erreur lors du réallouage de mémoire");
            pclose(fp);
            return NULL;
        }
        buffer[strcspn(buffer, "\n")] = '\0'; // Retirer le '\n'
        lines[*line_count] = strdup(buffer);
        if (lines[*line_count] == NULL) {
            perror("Erreur lors de la duplication de la ligne");
            pclose(fp);
            return NULL;
        }
        (*line_count)++;
    }

    // Fermer le flux
    if (pclose(fp) == -1) {
        perror("Erreur lors de la fermeture du processus");
        for (int i = 0; i < *line_count; i++) {
            free(lines[i]);
        }
        free(lines);
        return NULL;
    }

    return lines;
}



