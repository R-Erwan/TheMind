//
// Created by erwan on 09/12/2024.
//

#ifndef THEMINDROBOTIA_GAMESTATE_H

#include <stdbool.h>
#include "queue.h"
#define THEMINDROBOTIA_GAMESTATE_H
typedef struct{
    int l_card;
    Queue *cards;
    int min_card;
    int diff;
    int round_lvl;
    int nb_p;
    bool play;
} GameState;

GameState *create_gameState();

void free_GameState(GameState *gs);

void add_card(GameState *gs, int card);

void play_card(GameState *gs);

void reset(GameState *gs);



#endif //THEMINDROBOTIA_GAMESTATE_H
