//
// Created by erwan on 13/11/2024.
//

#ifndef THEMIND_PLAYERSRESSOURCES_H
#define THEMIND_PLAYERSRESSOURCES_H

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/socket.h>
#include <stdarg.h>

#define B_CONSOLE 1

typedef struct {
    int socket_fd; // Socket associate for the player
    char name[50];
    int ready; // boolean 1 is ready, 0 not ready
    int id; // Unique id
    int* cards; // Decks of cards,
}Player;

typedef struct {
    Player **players; // Players list
    int count; // Number of players
    int max; // Max players allowed
    pthread_rwlock_t mutexRW; // Mutex to ensure write and read operation.
}PlayerList;

/*
 * Creation and frees function on PLAYER
 */
Player *create_player(PlayerList *players,int socket_fd);
void free_player(Player *player);

/*
 * Creation and frees function on PLAYER LIST
 */
PlayerList* init_pl(int max_players);
void free_player_list(PlayerList* players);
void disconnect_allP(PlayerList* pl);

/*
 * Creation and frees function on PLAYER's CARDS
 */
void init_player_card(PlayerList *pl, int nb_cards);
void free_players_card(PlayerList *pl);

/*
 * Operations on PLAYER LIST
 */
int remove_player(PlayerList* players, Player *p);
int update_ready_player(PlayerList *players, Player *p, int state);
int set_player_name(PlayerList *players, Player *p, char* name);

/*
 * Message sending functions
 */
int broadcast_message(PlayerList* players, Player* exclude_player, int params, const char* format, ...);
void send_p(Player *player, const char* format, ...);

/*
 * Information getting function
 */
int is_full(PlayerList *playerList);
int get_ready_count(PlayerList *pl);


#endif //THEMIND_PLAYERSRESSOURCES_H
