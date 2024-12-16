//
// Created by erwan on 14/11/2024.
//

#ifndef THEMIND_GAME_H
#define THEMIND_GAME_H

#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "playersRessources.h"
#include "utils.h"
#include "queue.h"
#include "statsManager.h"
#include "ANSI-color-codes.h"

#define STAT_FILE_DL GRN"\nLe fichier de statistiques est disponible. \nNom du fichier : %s.pdf \nPour le récupérer, utiliser la commande : getfile %s.pdf sur le port du serveur + 1\n\n"CRESET
#define DEFAULT_ROUND 1
#define NO_CARD 1
#define WRONG_CARD 2
#define ROUND_WIN 3
#define LOBBY_STATE 0
#define GAME_STATE 1
#define PLAY_STATE 2

/**
 * @struct Game
 * @brief Represents the state of the game, including rounds, players, and game-specific data.
 *
 * This structure holds all the necessary information to manage the state of the game, including the current round,
 * the list of players, the game board, the number of played cards, the queue of cards to be played, and the current game state.
 * It also contains a read-write lock to ensure thread-safe access to the game state.
 */
typedef struct {
    int round; // Level of the actual round
    PlayerList *playerList; // List of Players
    int *board; // Int array, representing the cards played
    int played_cards_count; // Number of played card
    Queue *cards_queue; // Queue of the card to be played, sorted
    int state; // Actual state of the game (GAME,LOBBY or PLAY)
    GameData *gameData; // Structure to hold and generate stats
    time_t startingTime; // Timer representing le beginning of the round
    pthread_rwlock_t mutex; // Mutex to ensure reading et writting acces
} Game;

Game *create_game(PlayerList *pl);
void free_game(Game* g);

int start_game(Game* g,Player *p);
int start_round(Game *g, Player *p);
void end_round(Game *g, int win);
void end_game(Game *g, Player *p, bool hard_disco);

void distribute_card(Game *g);
int play_card(Game *g, Player *p,int card);

int set_ready_player(Game *g,Player *p,int state);

void send_stats(Game*g,Player *p);

void countdown(Game *g, int sleep_delta);

void print_lobbyState(Game* g);
void print_gameState(Game* g);
void print_playState(Game* g);
void print_classement(Game* g, Player* p);


#endif //THEMIND_GAME_H
