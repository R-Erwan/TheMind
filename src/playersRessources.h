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

#define BUFFER_SIZE 1024
#define B_CONSOLE 1

typedef struct {
    int socket_fd;
    char name[50];
    int ready;
    int id;
    int* cards;
}Player;

typedef struct {
    Player **players;
    int count;
    int max;
    pthread_rwlock_t mutexRW;
}PlayerList;

Player *create_player(PlayerList *players,int socket_fd);
void free_player(Player *player);
void disconnect_allP(PlayerList* pl);

PlayerList* init_pl(int max_players);
void free_player_list(PlayerList* players);

void init_player_card(PlayerList *pl, int nb_cards);
void free_players_card(PlayerList *pl);

int remove_player(PlayerList* players, Player *p);
int update_ready_player(PlayerList *players, Player *p, int state);
void reset_ready_players(PlayerList *players);
int set_player_name(PlayerList *players, Player *p, char* name);

int broadcast_message(PlayerList* players, Player* exclude_player, int params, const char* format, ...);

int is_full(PlayerList *playerList);
int get_ready_count(PlayerList *pl);
void send_p(Player *player, const char* format, ...);


#endif //THEMIND_PLAYERSRESSOURCES_H
