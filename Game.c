//
// Created by erwan on 14/11/2024.
//

#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include "Game.h"

#define COUNT_DOWN_MSG "La partie vas commencer dans : 3\n"
#define PLAY_CARD_MSG "Go ! Vous pouvez jouez une carte\n"

Game *create_game(PlayerList *pl) {
    Game *game = malloc(sizeof (Game));
    if(game == NULL) return NULL;
    game->playerList = pl;
    game->round = 2;
    pthread_rwlock_init(&game->mutex,NULL);

    return game;
}

void free_game(Game *g) {
    if (g) {
        if (g->board)
            free(g->board);
        pthread_rwlock_destroy(&g->mutex);
        free(g);
    }
}

int start_game(Game *g) {
    if(get_ready_count(g->playerList) != g->playerList->count){
        return -1;
    }
    g->board = malloc(sizeof(int) * (g->playerList->count * g->round));

    init_player_card(g->playerList,g->round); // Malloc player's deck
    //Distribuer les cartes
    distribute_card(g);
    countdown(g,1);


    return 0;
}

void distribute_card(Game *g){
    PlayerList *pl = g->playerList;

    int deck[99];
    /* Remplit le deck avec les cartes de 1 à 99*/
    for (int i = 0; i < 99; ++i) {
        deck[i] = i + 1;
    }
    /* Mélange le deck (Fisher-Yates) */
    for (int i = 98 ; i > 0; i--) {
        int j = rand() % (i+1);

        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }

    int card_index = 0;
    for (int i = 0; i <g->round; ++i) {
        for (int j = 0; j < pl->count; ++j) {
            pl->players[j].cards[i] = deck[card_index];
            send_card_message(&pl->players[j],deck[card_index]);
            card_index++;
        }
    }
}

void countdown(Game *g,int sleep_delta){
    broadcast_message(COUNT_DOWN_MSG,g->playerList,NULL,B_CONSOLE);
    sleep(sleep_delta);
    broadcast_message("2\n",g->playerList,NULL,0);
    sleep(sleep_delta);
    broadcast_message("1\n",g->playerList,NULL,0);
    sleep(sleep_delta);
    broadcast_message(PLAY_CARD_MSG,g->playerList,NULL,0);
}
