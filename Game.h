//
// Created by erwan on 14/11/2024.
//

#ifndef THEMIND_GAME_H
#define THEMIND_GAME_H
#include "playersRessources.h"
#include "utils.h"
#include "queue.h"


#define COUNT_DOWN_MSG "La partie vas commencer dans :\n"
#define PLAY_CARD_MSG "Go ! Vous pouvez jouez une carte\n"
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
    /**
     * @brief The current round of the game.
     *
     * This field represents the current round number in the game. The round is incremented after each completed round.
     */
    int round;

    /**
     * @brief A pointer to the list of players participating in the game.
     *
     * This field holds a pointer to a `PlayerList` structure, which manages the players, their cards, and readiness status.
     */
    PlayerList *playerList;

    /**
     * @brief The game board, representing the cards played during the game.
     *
     * This is an array that stores the cards that have been played so far in the current round. The size of the board is
     * determined by the number of rounds and players.
     */
    int *board;

    /**
     * @brief The number of cards that have been played in the current round.
     *
     * This field tracks the total number of cards played so far in the current round.
     */
    int played_cards_count;

    /**
     * @brief A pointer to the queue of cards that are available to be played in the game.
     *
     * This queue holds the remaining cards in the game, and it is used to track which cards are yet to be played.
     */
    Queue *cards_queue;

    /**
     * @brief The current state of the game.
     *
     * This field represents the current state of the game, such as whether it is in the lobby, in play, or finished.
     * It is an integer that corresponds to various predefined game states.
     */
    int state;

    /**
     * @brief A read-write lock used to ensure thread-safe access to the game state.
     *
     * This lock is used to synchronize access to the game data, ensuring that multiple threads can safely read or
     * modify the game state without conflicts.
     */
    pthread_rwlock_t mutex;
} Game;

Game *create_game(PlayerList *pl);
void free_game(Game* g);

int start_game(Game* g);
int start_round(Game *g);
void end_round(Game *g, int win);
void end_game(Game *g);

void distribute_card(Game *g);
int play_card(Game *g, Player *p,int card);

void countdown(Game *g, int sleep_delta);
void broadcast_board(Game *g);
int set_ready_player(Game *g,Player *p,int state);


#endif //THEMIND_GAME_H
