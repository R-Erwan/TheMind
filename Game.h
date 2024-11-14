//
// Created by erwan on 14/11/2024.
//

#ifndef THEMIND_GAME_H
#define THEMIND_GAME_H
#include "playersRessources.h"

typedef struct {
    int round;
    PlayerList *playerList;
    int *board;
}Game;

void start_game();
void distribute_card();
void play_card();
void win_round();
void loose_round();

#endif //THEMIND_GAME_H
