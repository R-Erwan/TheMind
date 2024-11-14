//
// Created by erwan on 13/11/2024.
//

#include <unistd.h>
#include <malloc.h>
#include <sys/socket.h>
#include "playersRessources.h"

/**
 * @brief Initializes a new player list with a defined maximum capacity.
 *
 * Allocates the necessary memory for the PlayerList structure and the dynamic
 * array of players, then initializes the read/write lock.
 *
 * @param max_players The maximum number of players allowed in the list.
 * @return A pointer to the new PlayerList structure, or NULL if an allocation fails.
 */
PlayerList* init_pl(int max_players) {
    PlayerList* players = malloc(sizeof(PlayerList));
    if (players == NULL) return NULL;

    players->players = malloc(sizeof(Player) * max_players);
    if (players->players == NULL) {
        free(players);
        return NULL;
    }

    players->count = 0;
    players->max = max_players;
    pthread_rwlock_init(&players->mutexRW, NULL);

    return players;
}

/**
 * @brief Frees the memory allocated for the player list.
 *
 * Destroys the read/write lock, frees the player array, and
 * then frees the PlayerList structure itself.
 *
 * @param players Pointer to the player list to free.
 */
void free_player_list(PlayerList* players){
    if(players){
        pthread_rwlock_destroy(&players->mutexRW);
        for (int i = 0; i < players->count - 1; ++i) {
            free(players->players[i].cards);
        }
        free(players->players);
        free(players);
    }
}

/**
 * @brief Creates a new player and adds them to the player list.
 *
 * This function checks if the player list has reached its maximum capacity
 * or if the player's name is too long. If conditions are met, it allocates
 * a new slot in the player array, initializes the player with the name and
 * socket descriptor, and adds them to the list.
 *
 * @param players Pointer to the player list.
 * @param socket_fd Socket descriptor of the player.
 * @param name Name of the player (limited to 50 characters).
 * @return A pointer to the new Player structure if successful,
 *         or NULL if the player limit is reached or an error occurs.
 */
Player* create_player(PlayerList* players, int socket_fd) {
    if (players->count >= players->max) {
        return NULL;  // Limite de joueurs atteinte ou nom trop long
    }

    pthread_rwlock_wrlock(&players->mutexRW);
    Player* player = &players->players[players->count++];

    player->socket_fd = socket_fd;
    player->ready = 0;
    player->id = players->count - 1;
    player->cards = NULL;
    snprintf(player->name,sizeof(player->name),"Anonyme%d",player->id);

    pthread_rwlock_unlock(&players->mutexRW);
    return player;
}

/**
 * @brief Removes a specific player from the player list.
 *
 * This function searches for a player in the list by their ID and removes
 * them if found. If the player is not the last one in the array, the last
 * player is moved to fill the empty slot, and their ID is updated.
 *
 * @param players Pointer to the player list.
 * @param p Pointer to the player to remove.
 * @return 0 if the player was found and removed, -1 otherwise.
 */
int remove_player(PlayerList* players, Player* p) {
    pthread_rwlock_wrlock(&players->mutexRW);

    int player_found = 0;
    for (int i = 0; i < players->count; ++i) {
        if (players->players[i].id == p->id) {
            player_found = 1;

            // Remplace le joueur supprimé par le dernier joueur de la liste
            if (i != players->count - 1) {
                players->players[i] = players->players[players->count - 1];
                players->players[i].id = i;  // Met à jour l'ID
            }

            players->count--;
            break;
        }
    }

    pthread_rwlock_unlock(&players->mutexRW);
    return player_found;
}

/**
 * @brief Change ready state of a specific player.
 *
 * @param players Pointer to the player list.
 * @param p Pointer to the player to remove.
 * @param state 1 to set player ready, 0 for not ready
 * @return 0 if a change occurs, -1 if the player was not found, 1 if the player
 *         was already in this state.
 */
int update_ready_player(PlayerList *players, Player *p, int state){
    if(p == NULL)
        return -1;
    if(p->ready == state)
        return 1;

    pthread_rwlock_wrlock(&players->mutexRW);
    p->ready = !p->ready;
    pthread_rwlock_unlock(&players->mutexRW);
    return 0;
}

int set_player_name(PlayerList *players, Player *p, char* name){
    if(strlen(name) >= 50)
        return -1;

    pthread_rwlock_wrlock(&players->mutexRW);
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[strcspn(p->name, "\n")] = '\0';
    pthread_rwlock_unlock(&players->mutexRW);
    return 0;
}

/**
 * @brief Broadcast message to all client.
 * @param msg The message to send
 * @param players Pointer to the player list.
 * @param exclude_player Pointer to the player to exclude from broadcasting
 * @param params 1 to display message in local console.
 * @return 0
 */
int broadcast_message(const char* msg, PlayerList *players, Player *exclude_player, int params){
    pthread_rwlock_rdlock(&players->mutexRW);
    for (int i = 0; i < players->count; ++i) {
        if(exclude_player == NULL || players->players[i].id != exclude_player->id){
            send(players->players[i].socket_fd, msg, strlen(msg),0);
        }
    }
    if(params == B_CONSOLE){
        printf("[BROADCAST] : %s",msg);
    }
    pthread_rwlock_unlock(&players->mutexRW);
    return 0;
}

/**
 * @brief Broadcast message to all player that theres a new player.
 * @param players Pointer to the list of player.
 * @param p Pointer on the new player joining.
 */
void new_player_broadcast(PlayerList *players, Player *p){
    char msg[128];
    snprintf(msg, sizeof msg, NEWP_BROADCAST, p->name, players->count);
    broadcast_message(msg,players,p,B_CONSOLE);
}

/**
 * @brief Broadcast message to all, that a player as quit.
 * @param players Pointer to the list of player.
 * @param p Pointer to the player who quit.
 */
void leave_broadcast(PlayerList *players, Player *p){
    char message[128];
    snprintf(message, sizeof message, LEAVEP_BROADCAST, p->name, players->count);
    broadcast_message(message,players,NULL,B_CONSOLE);
}

/**
 * @brief Broadcast the ratio of ready player.
 * @param players Pointer to the the list of player.
 */
void ready_player_broadcast(PlayerList *players){
    char message[128];
    snprintf(message, sizeof message, READYP_BROADCAST, get_ready_count(players),players->count);
    broadcast_message(message, players,NULL,B_CONSOLE);
}

/**
 * @brief test is the list is full
 * @param playerList Pointer to the player list
 * @return 1 if the list is full, 0 if theres is still place
 */
int is_full(PlayerList *playerList){
    pthread_rwlock_rdlock(&playerList->mutexRW);
    if(playerList->count == playerList->max){
        pthread_rwlock_unlock(&playerList->mutexRW);
        return 0;
    }
    pthread_rwlock_unlock(&playerList->mutexRW);
    return 1;
}

/**
 * @brief Calc the number of player ready.
 * @param pl Pointer to the list of player.
 * @return Number of player's ready.
 */
int get_ready_count(PlayerList *pl){
    pthread_rwlock_rdlock(&pl->mutexRW);
    int count = 0;
    for (int i = 0; i < pl->count; ++i) {
        if(pl->players[i].ready == 1)
            count++;
    }
    pthread_rwlock_unlock(&pl->mutexRW);

    return count;
}

void init_player_card(PlayerList *pl, int nb_cards) {
    pthread_rwlock_wrlock(&pl->mutexRW);
    for (int i = 0; i < pl->count; ++i) {
        pl->players[i].cards = malloc(sizeof (int) * nb_cards);
    }
    pthread_rwlock_unlock(&pl->mutexRW);
}

void send_card_message(Player *p, int card) {
    char msg[128];
    snprintf(msg, sizeof msg, SEND_CARD, card);
    send(p->socket_fd,msg,strlen(msg),0);
}




