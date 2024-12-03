//
// Created by erwan on 13/11/2024.
//


#include "playersRessources.h"

/**
 * @brief Creates a new player and adds them to the player list.
 *
 * This function checks if the player list has reached its maximum capacity
 * or if the player's name is too long. If conditions are met, it allocates
 * a new player, and point the array player to the player, initializes the player with a default name and
 * socket descriptor, and adds them to the list.
 *
 * @param players Pointer to the player list.
 * @param socket_fd Socket descriptor of the player.
 * @return A pointer to the new Player structure if successful,
 *         or NULL if the player limit is reached or an error occurs.
 */
Player* create_player(PlayerList* players, int socket_fd) {
    if (players->count >= players->max) {
        return NULL;  // Limite de joueurs atteinte ou nom trop long
    }
    pthread_rwlock_wrlock(&players->mutexRW);

    Player *player = malloc(sizeof(Player));
    player->socket_fd = socket_fd;
    player->ready = 0;
    player->id = players->count;
    player->cards = NULL;
    snprintf(player->name,sizeof(player->name),"Anonyme%d",player->id);

    players->players[players->count] = player;
    players->count++;

    pthread_rwlock_unlock(&players->mutexRW);
    return player;
}
/**
 * @brief Frees all dynamically allocated resources for a player and deletes the player structure.
 *
 * This function first frees the player's card array if it was allocated,
 * and then frees the memory associated with the `Player` structure itself.
 * It also ensures that pointers are set to `NULL` after being freed to avoid accidental use.
 *
 * @param player A pointer to the `Player` structure to be freed.
 *               If `player` is `NULL`, the function does nothing.
 *
 * @note This function is intended to be used when completely removing a player,
 *       for example, during disconnection.
 */
void free_player(Player* player) {
    if (player == NULL) {
        return;
    }

    if (player->cards != NULL) {
        free(player->cards);
        player->cards = NULL;
    }

    free(player);
}
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
            free_player(players->players[i]);
        }
        free(players->players);
        free(players);
    }
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
        if (players->players[i]->id == p->id) {
            player_found = 1;

            free_player(p);

            // Remplace le joueur supprimé par le dernier joueur de la liste
            if (i != players->count - 1) {
                players->players[i] = players->players[players->count - 1];
                players->players[i]->id = i;  // Met à jour l'ID
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
    broadcast_message(players,NULL,B_CONSOLE,"[%d/ %d] joueur prêt\n", get_ready_count(players),players->count);
    return 0;
}
/**
 * @brief Sets the name of a player in a thread-safe manner.
 *
 * This function validates the provided name to ensure it meets the required
 * length constraints (between 3 and 49 characters). If valid, it updates
 * the player's name while holding a write lock on the `PlayerList` to ensure
 * thread safety. The function also removes any trailing newline character
 * from the input name.
 *
 * @param players A pointer to the `PlayerList` structure containing the player.
 * @param p A pointer to the `Player` whose name is being updated.
 * @param name The new name to assign to the player. Must be between 3 and 49 characters.
 *
 * @return 0 on success, -1 if the name is invalid (too short or too long).
 *
 * @note This function modifies the player's name in-place and requires
 */
int set_player_name(PlayerList *players, Player *p, char* name){
    if(strlen(name) >= 50 || strlen(name) < 3)
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
int broadcast_message(PlayerList* players, Player* exclude_player, int params, const char* format, ...) {
    char buffer[BUFFER_SIZE];
    va_list args;

    // Formater le message
    va_start(args, format);
    int length = vsnprintf(buffer, BUFFER_SIZE, format, args);
    va_end(args);

    if (length < 0) {
        perror("Erreur de formatage du message");
        return -1;
    }

    // Bloquer en lecture le mutex
    pthread_rwlock_rdlock(&players->mutexRW);

    // Diffuser le message à tous les joueurs, sauf le joueur exclu
    for (int i = 0; i < players->count; ++i) {
        Player* current_player = players->players[i];
        if (exclude_player == NULL || current_player->id != exclude_player->id) {
            if (send(current_player->socket_fd, buffer, length, 0) == -1) {
                perror("Erreur lors de l'envoi du message à un joueur");
            }
        }
    }

    // Afficher le message dans la console si le paramètre est défini
    if (params == B_CONSOLE) {
        printf("[BROADCAST]: %s\n", buffer);
    }

    // Débloquer le mutex
    pthread_rwlock_unlock(&players->mutexRW);

    return 0;
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
        if(pl->players[i]->ready == 1)
            count++;
    }
    pthread_rwlock_unlock(&pl->mutexRW);

    return count;
}
/**
 * @brief Initializes the cards array for all players in the PlayerList.
 *
 * This function allocates memory for the `cards` array of each player in the
 * `PlayerList`. The size of the `cards` array is determined by the `nb_cards`
 * parameter.

 * @param pl A pointer to the `PlayerList` structure containing the players.
 * @param nb_cards The number of cards to allocate for each player. Each
 *                 player's `cards` array will be of size `nb_cards` and
 *                 zero-initialized.
 *
 * @note If a player's `cards` array was already allocated, this function does
 *       not free the previous allocation, potentially leading to a memory leak.
 *       Ensure proper memory management before calling this function.
 */
void init_player_card(PlayerList *pl, int nb_cards) {
    pthread_rwlock_wrlock(&pl->mutexRW);
    for (int i = 0; i < pl->count; ++i) {
        pl->players[i]->cards = calloc(nb_cards,sizeof (int));
    }
    pthread_rwlock_unlock(&pl->mutexRW);
}
/**
 * @brief Frees the memory allocated for the `cards` array of all players in the PlayerList.
 *
 * This function iterates through the `PlayerList` and releases the memory allocated
 * for each player's `cards` array.
 *
 * @param pl A pointer to the `PlayerList` structure containing the players..
 *
 * @note After calling this function, the `cards` pointer of each player will be set
 *       to `NULL` to prevent dangling pointers.
 */
void free_players_card(PlayerList *pl){
    if (pl == NULL) {
        fprintf(stderr, "Error: PlayerList is NULL\n");
        return;
    }
    pthread_rwlock_wrlock(&pl->mutexRW);
    for (int i = 0; i < pl->count; ++i) {
        if (pl->players[i] != NULL && pl->players[i]->cards != NULL) {
            free(pl->players[i]->cards);
            pl->players[i]->cards = NULL;
        }
    }
    pthread_rwlock_unlock(&pl->mutexRW);
}
/**
 * @brief Resets the "ready" status of all players in the player list.
 *
 * This function iterates through the list of players and sets their `ready`
 * status to `0`. After resetting, it broadcasts
 * the updated readiness status to all clients.
 *
 * @param players A pointer to the `PlayerList` structure containing the list of players.
 *
 */
void reset_ready_players(PlayerList *players) {
    if (players == NULL) {
        fprintf(stderr, "Error: PlayerList is NULL.\n");
        return;
    }
    pthread_rwlock_wrlock(&players->mutexRW);
    for (int i = 0; i < players->count; ++i) {
        if (players->players[i] != NULL) {
            players->players[i]->ready = 0;
        }
    }
    pthread_rwlock_unlock(&players->mutexRW);
    broadcast_message(players,NULL,B_CONSOLE,"[%d/ %d] joueur prêt", get_ready_count(players),players->count);
}
/**
 * @brief Send format message to one player.
 * @param player Player to send the message on his socket.
 * @param format Format message.
 * @param ... Parameters puts in the char format string.
 */
void send_p(Player *player, const char* format, ...) {
    char buffer[BUFFER_SIZE];
    va_list args;

    // Initialiser la liste des arguments variables
    va_start(args, format);

    // Générer la chaîne formatée
    int length = vsnprintf(buffer, BUFFER_SIZE, format, args);
    // Nettoyer la liste des arguments
    va_end(args);

    // Vérifier si le message a été correctement formaté
    if (length < 0) {
        perror("Erreur de formatage du message");
        return;
    }

    // Envoyer le message via le socket
    if (send(player->socket_fd, buffer, length, 0) == -1) {
        perror("Erreur lors de l'envoi du message");
    }
}
