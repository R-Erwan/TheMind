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


ServerMsg parse_stoc(const char* msg){
    ServerMsg sm;
    sm.code = NULL_MSG;


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
    } else if (strcmp("Go !",msg) == 0){
        sm.code = GO;
    } else {
        return sm;
    }

    printf("{%s} parsed to : {%d}\n",msg,sm.code);
    return sm;
}

