//
// Created by erwan on 05/12/2024.
//

#include <string.h>
#include <malloc.h>
#include "ANSI-color-codes.h"
#include "parser.h"

void remove_ansi_codes(char *msg) {
    char *src = msg, *dst = msg;
    while (*src) {
        if (*src == '\e') { // Début d'une séquence ANSI
            src++;          // Ignorer '\e'
            if (*src == '[') {
                src++;      // Ignorer '['
                while ((*src >= '0' && *src <= '9') || *src == ';') { // Ignorer chiffres et ';'
                    src++;
                }
                if (*src) src++; // Ignorer la lettre finale (par exemple 'm')
            }
        } else {
            *dst++ = *src++; // Copier les caractères normaux
        }
    }
    *dst = '\0'; // Terminer la chaîne
}

ServerMsg parse_stoc(const char* msg){
    ServerMsg sm;
    sm.code = NULL_MSG;
    int n = 0;

    if(sscanf(msg,"%49s a lancé le round (niveau :%d)",sm.param1,&sm.param2) == 2){
        sm.code = ROUND_START;
    } else if (sscanf(msg,"%49s -> %d",sm.param1,&sm.param2) == 2){
        sm.code = CARD_PLAY;
    } else if (sscanf(msg,"Bravo vous avez gagné la manche %d",&sm.param2) == 1){
        sm.code = WIN_ROUND;
    } else if (sscanf(msg,"La manche %d est perdu !",&sm.param2) == 1){
        sm.code = LOOSE_ROUND;
    } else if (sscanf(msg,"Carte : %d",&sm.param2) == 1){
        sm.code = CARD;
    } else if (sscanf(msg,"%49s a lancé la partie ! (joueurs : %d)", sm.param1,&sm.param2) == 2 ){
        sm.code = GAME_START;
    } else if (strcmp("La partie vas commencer dans : 3 2 1 Go !",msg) == 0){
        sm.code = GO;
    } else if (strcmp("Prêt pour une nouvelle partie ?",msg) == 0) {
        sm.code = ENDGAME;
    } else {
        return sm;
    }

//    printf("{%s} parsed to : {%d}\n",msg,sm.code);
    return sm;
}

