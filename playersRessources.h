//
// Created by erwan on 13/11/2024.
//

#ifndef THEMIND_PLAYERSRESSOURCES_H
#define THEMIND_PLAYERSRESSOURCES_H

#include <pthread.h>
#include <string.h>

#define NEWP_BROADCAST "%s a rejoint la partie ! Joueur connecté %d\n"
#define LEAVEP_BROADCAST "%s a quitté! Joueurs connectés: %d\n"
#define READYP_BROADCAST "[%d/ %d] joueur prêt\n"

#define B_CONSOLE 1

typedef struct {
    int socket_fd;
    char name[50];
    int ready;
    int id;
}Player;

typedef struct {
    Player* players;
    int count;
    int max;
    pthread_rwlock_t mutexRW;
}PlayerList;

PlayerList* init_pl(int max_players);
void free_player_list(PlayerList* players);
Player *create_player(PlayerList *players,int socket_fd);
int remove_player(PlayerList* players, Player *p);
int update_ready_player(PlayerList *players, Player *p, int state);
int set_player_name(PlayerList *players, Player *p, char* name);

int broadcast_message(const char* msg, PlayerList *players, Player *exclude_player, int params);
void new_player_broadcast(PlayerList *players, Player *p);
void leave_broadcast(PlayerList *players, Player *p);
void ready_player_broadcast(PlayerList *players);

int is_full(PlayerList *playerList);
int get_ready_count(PlayerList *pl);


#endif //THEMIND_PLAYERSRESSOURCES_H
