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
    pthread_rwlock_t mutex;
}Game;

Game *create_game(PlayerList *pl);
void free_game(Game* g);

int start_game(Game* g);
void distribute_card(Game *g);
int play_card(Game *g, Player *p,int card);
void win_round(Game *g);
void loose_round(Game *g);
void play_next_round(Game *g);
void stop_game(Game *g);
void countdown(Game *g, int sleep_delta);


#endif //THEMIND_GAME_H
