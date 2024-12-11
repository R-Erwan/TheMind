//
// Created by erwan on 09/12/2024.
//

#include <malloc.h>
#include "GameState.h"

GameState *create_gameState() {
    GameState *gs = malloc(sizeof(GameState));
    if(gs == NULL) return NULL;
    gs->l_card = 0;
    gs->diff = -1;
    gs->nb_p = -1;
    gs->round_lvl = -1;
    gs->min_card = 100;
    gs->cards = create_queue();
    gs->play = false;

    return gs;
}

void free_GameState(GameState *gs) {
    if(gs){
        destroy_queue(gs->cards);
        free(gs);
    }

}

void add_card(GameState *gs, int card) {
    enqueue(gs->cards,card);
    sort_queue(gs->cards);
    if(card < gs->min_card){
        gs->min_card = card;
        gs->diff = gs->min_card;
    }
}

int play_card(GameState *gs) {
    if (dequeue(gs->cards) == -1){
        return -1;
    }

    if(isEmpty(gs->cards)){
        gs->min_card = 100;
    } else {
        gs->min_card = peek(gs->cards);
    }
    return 0;
}

void reset(GameState *gs) {
    reset_queue(gs->cards);
    gs->min_card = 100;
    gs->diff = 100;
    gs->play = false;
    gs->l_card = 0;
}
