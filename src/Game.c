//
// Created by erwan on 14/11/2024.
//


#include "Game.h"

/**
 * @brief Creates and initializes a new game.
 *
 * This function dynamically allocates memory for a `Game` object and initializes it with default values.
 * It also associates the provided player list with the game and creates a new card queue.
 * The game is initialized in the "lobby" state, ready to start the first round.
 *
 * @param pl A list of players who will participate in the game.
 *           This parameter is a reference to an existing list of players, which must be non-null and valid.
 *
 * @return A pointer to a newly created and initialized `Game` object, or `NULL` if memory allocation fails.
 *
 * @note The function uses a read-write lock to ensure thread safety when accessing the game object.
 */
Game *create_game(PlayerList *pl) {
    Game *game = malloc(sizeof (Game));
    if(game == NULL) return NULL;
    game->playerList = pl;
    game->round = DEFAULT_ROUND;
    game->cards_queue = create_queue();
    game->board = NULL;
    game->played_cards_count =0;
    game->state = LOBBY_STATE;
    game->gameData = NULL;
    pthread_rwlock_init(&game->mutex,NULL);

    return game;

}
/**
 * @brief Frees the memory allocated for a game and its associated resources.
 *
 * This function deallocates all memory used by a `Game` object, including the game board,
 * the card queue, and the memory for the game object itself. It also destroys the read-write lock
 * associated with the game to ensure proper resource cleanup.
 *
 * @param g A pointer to the `Game` object to be freed.
 *          This pointer must be valid and non-null.
 *
 * @note The function checks if each resource is allocated before freeing it. It ensures that
 *       no invalid memory access occurs by verifying if `g->board` and `g->cards_queue` are non-null.
 */
void free_game(Game *g) {
    if (g) {
        if (g->board)
            free(g->board);
        pthread_rwlock_destroy(&g->mutex);
        free(g->gameData);
        destroy_queue(g->cards_queue);
        free(g);
    }
}
/**
 * @brief Starts a new game if the conditions are met.
 *
 * This function checks if all players are ready and if the game is not already in a playing state
 * before transitioning the game to the "game" state. If the conditions are satisfied, it starts a new round.
 * The function ensures thread safety by acquiring a write lock on the game's mutex during the state change.
 *
 * @param g A pointer to the `Game` object to be started.
 *          This pointer must be valid and non-null.
 *
 * @return 0 if the game was successfully started, or -1 if the game cannot be started
 *         due to invalid conditions (e.g., not all players are ready or the game is already in progress).
 *
 * @note The function locks and unlocks the read-write mutex to ensure that state changes and
 *       checks on the game's state are done atomically and safely in a multithreaded environment.
 */
int start_game(Game *g,Player *p) {
    pthread_rwlock_wrlock(&g->mutex);
    if(get_ready_count(g->playerList) != g->playerList->count || g->state == PLAY_STATE || g->state == GAME_STATE){
        pthread_rwlock_unlock(&g->mutex);
        return -1;
    }
    g->gameData = create_gm(); // Create GameData stats
    g->gameData->player_count = g->playerList->count; // Set the player number

    g->state = GAME_STATE;
    broadcast_message(g->playerList,NULL,B_CONSOLE,GRN"\n%s a lancé la partie ! (joueurs : %d)\n\n"CRESET,p->name,g->playerList->count);
    pthread_rwlock_unlock(&g->mutex);
    start_round(g,p);
    return 0;
}
/**
 * @brief Starts a new round of the game if the conditions are met.
 *
 * This function checks if all players are ready and if the game is not already in progress
 * before transitioning the game to the "play" state. If the conditions are satisfied,
 * it allocates memory for the game board, sends a broadcast message to all players about
 * the start of the round, initializes the players' decks, and distributes cards.
 * The function ensures thread safety by acquiring a write lock on the game's mutex during state changes.
 *
 * @param g A pointer to the `Game` object where the round will be started.
 *          This pointer must be valid and non-null.
 *
 * @return 0 if the round was successfully started, or -1 if the round cannot be started
 *         due to invalid conditions (e.g., not all players are ready or the game is already in progress).
 *
 * @note The function locks and unlocks the read-write mutex to ensure that state changes and
 *       checks on the game's state are done atomically and safely in a multithreaded environment.
 */
int start_round(Game *g,Player *p){
    pthread_rwlock_wrlock(&g->mutex);
    if(get_ready_count(g->playerList) != g->playerList->count || g->state == PLAY_STATE){
        pthread_rwlock_unlock(&g->mutex);
        return -1;
    }

    g->board = calloc((g->playerList->count * g->round),sizeof (int));
    g->state = PLAY_STATE;

    broadcast_message(g->playerList,NULL,B_CONSOLE,GRN"\n%s a lancé le round (niveau :%d)\n\n"CRESET,p->name,g->round);

    init_player_card(g->playerList,g->round); // Malloc player's deck
    distribute_card(g);
    print_playState(g);
    countdown(g,1); // Countdown broadcast.
    g->startingTime = time(NULL); // Init current timer.
    pthread_rwlock_unlock(&g->mutex);
    return 0;
}
/**
 * @brief Ends the current round and handles the results based on the win status.
 *
 * This function processes the end of a round by broadcasting a message to all players,
 * indicating whether the round was won or lost. If the round was won, it checks if there are enough
 * cards for the next round and increments the round number if possible. After the round is finished,
 * it cleans up the resources, including resetting player cards and game state, and prepares for the next round.
 *
 * @param g A pointer to the `Game` object where the round will be ended.
 *          This pointer must be valid and non-null.
 * @param win An integer indicating the outcome of the round:
 *            - 1 if the round was won by the players.
 *            - 0 if the round was lost by the players.
 *
 * @note The function frees memory associated with player cards, resets ready players,
 *       and clears the game board. The state of the game is also reset to `GAME_STATE` after the round.
 *       The round counter is only incremented if there are enough cards for the next round.
 */
void end_round(Game *g, int win){
    if(win){
        broadcast_message(g->playerList,NULL,0,GRN"\nBravo vous avez gagné la manche %d\n\n"CRESET,g->round);
        add_round(g->gameData,g->round,1); // Add 1 winning round to GameData

        //Check if next manche is possible, if there's enough card for every player.
        if((g->round + 1) * (g->playerList->count) <= 99){
            g->round++;
        }

    } else {
        broadcast_message(g->playerList,NULL,0,GRN"\nLa manche %d est perdu !\n\n"CRESET,g->round);
        add_round(g->gameData,g->round,0); // Add 1 loosing round to GameData
        g->round = DEFAULT_ROUND;
    }

    free_players_card(g->playerList);
    free(g->board); g->board = NULL;
    g->played_cards_count = 0;
    reset_queue(g->cards_queue);
    g->state = GAME_STATE;
    print_gameState(g);
}
/**
 * @brief Ends the game and resets it to the lobby state.
 *
 * This function broadcasts a message to all players indicating that the game has ended and that they
 * are being returned to the lobby. It then resets the round counter to the default value and sets
 * the game state back to `LOBBY_STATE`, preparing the game for a new session.
 *
 * @param g A pointer to the `Game` object to be ended.
 *          This pointer must be valid and non-null.
 *
 * @note The function does not free any game resources, as the game may be resumed or reinitialized.
 *       It only resets the round and game state to their initial values.
 *
 * @warning The function destroy associate GameData structure, ensure you have nothing to do with it
 *      after calling this function.
 */
void end_game(Game *g, Player* p, bool hard_disco){
    if(g->state == LOBBY_STATE) return;
    g->state = LOBBY_STATE;
    if(hard_disco){
        broadcast_message(g->playerList,p,B_CONSOLE,GRN"\n%s a mis fin a la partie, retour au lobby\n\n"CRESET,p->name);
    } else {
        broadcast_message(g->playerList,NULL,B_CONSOLE,GRN"\n%s a mis fin a la partie, retour au lobby\n\n"CRESET,p->name);
        p = NULL;
    }

    send_stats(g,p);
    char** names= malloc(g->playerList->count * sizeof(char*));
    for (int i = 0; i < g->playerList->count; i++) {
        names[i] = strdup(g->playerList->players[i]->name);
    }

    write_game_rank(g->gameData,names);

    for (int i = 0; i < g->playerList->count; i++) {
        free(names[i]);
    }
    free(names);

    print_classement(g,p); //Envoie le classement

    free_gm(g->gameData); // Destroy GameData
    g->gameData = NULL;
    g->round = DEFAULT_ROUND;

    broadcast_message(g->playerList,p,0,GRN"\nPrêt pour une nouvelle partie ?\n\n"CRESET);
}
/**
 * @brief Distributes cards to the players for the current round.
 *
 * This function initializes a deck of cards (from 1 to 99), shuffles the deck using the Fisher-Yates algorithm,
 * and then distributes the cards to the players for the current round. Each player receives a number of cards
 * equal to the current round number. The function also adds the distributed cards to the game's card queue
 * and sends a message to each player with the card they received.
 *
 * @param g A pointer to the `Game` object in which the cards will be distributed.
 *          This pointer must be valid and non-null.
 *
 * @note The function modifies the players' decks by assigning the cards, and also modifies the game’s
 *       card queue by enqueuing each card as it is distributed. After the distribution, the card queue
 *       is sorted.
 *
 * @see sort_queue() for how the card queue is sorted after distribution.
 */
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

    // Distributed cards
    int card_index = 0;
    for (int i = 0; i <g->round; ++i) {
        for (int j = 0; j < pl->count; ++j) {
            pl->players[j]->cards[i] = deck[card_index]; // Add card to player deck
            enqueue(g->cards_queue,deck[card_index]); // Add card to game_cards
            send_p(pl->players[j],BLK"Carte : %d\n"CRESET,deck[card_index]); // Send message to player.
            card_index++;
        }
    }
    sort_queue(g->cards_queue); // Sort the cards_queue
}
/**
 * @brief Handles the action of a player playing a card during the game.
 *
 * This function processes the player's attempt to play a specific card. It checks if the player has the card
 * in their hand and whether the card is valid (i.e., within the range of 1 to 99). If the card is valid,
 * the function compares it to the top card of the game's card queue to determine whether the move is valid.
 * If the card matches the top card, it is added to the board. If the card is incorrect, the round is lost.
 * The function also manages the state of the game and broadcasts relevant messages to all players.
 *
 * @param g A pointer to the `Game` object, representing the current game state.
 * @param p A pointer to the `Player` object, representing the player making the move.
 * @param card The card number the player wishes to play (between 1 and 99).
 *
 * @return
 * - `NO_CARD` if the player does not have the card or if the card is invalid.
 * - `WRONG_CARD` if the player plays a card that does not match the top card of the queue.
 * - `ROUND_WIN` if the round is won (i.e., all cards are played).
 * - `0` if the card is played successfully and no round ends.
 *
 * @note
 * The function uses a read-write lock to ensure thread safety while modifying the game state.
 * The game state is updated after each card is played, and messages are broadcasted to notify all players.
 * The function also updates the board and handles the game’s round status (win or loss).
 */
int play_card(Game *g, Player *p, int card){
    if(p->cards == NULL || card < 0 || card > 99) {
        return NO_CARD;
    }
    pthread_rwlock_wrlock(&g->mutex);

    //Check if player have this card
    int have_card = 0;
    for (int i = 0; i <= g->round; ++i) {
        if(p->cards[i] == card){
            p->cards[i] = 0; // Remove card from player
            have_card = 1;
            break;
        }
    }

    if(!have_card || card == 0){
        pthread_rwlock_unlock(&g->mutex);
        return NO_CARD;
    }

    broadcast_message(g->playerList,NULL,B_CONSOLE,GRN"\n%s -> %d\n\n"CRESET,p->name,card);

    if(card != peek(g->cards_queue)){
        //Branch when the card loose the round, refused
        time_t delta_time = time(NULL) - g->startingTime; // Calc delta time with the starting time of the round.
        add_loosing_card(g->gameData,card,delta_time);

        print_playState(g);

        end_round(g,0);
        pthread_rwlock_unlock(&g->mutex); // Cares to unlock mutex AFTER calling loose round
        return WRONG_CARD;
    } else {
        //Branch when the card is accepted

        time_t delta_time = time(NULL) - g->startingTime; // Calc delta time with the starting time of the round.
        add_card(g->gameData,card,delta_time);

        g->board[g->played_cards_count] = dequeue(g->cards_queue);
        g->played_cards_count++;

        print_playState(g);

        //If all cards played, win the round
        if(isEmpty(g->cards_queue)){
            end_round(g,1);
            pthread_rwlock_unlock(&g->mutex);
            return ROUND_WIN;
        }
    }
    pthread_rwlock_unlock(&g->mutex);
    return 0;
}
/**
 * @brief Sets the readiness state of a player in the game.
 *
 * This function updates the readiness state of a player, indicating whether they are ready to start the game or the round.
 * The function first checks if the game is in the `PLAY_STATE`, in which case it is not possible to modify the player's readiness
 * and returns an error. Otherwise, it updates the readiness state using the `update_ready_player` function and returns the result.
 *
 * @param g A pointer to the `Game` object, representing the current game state.
 * @param p A pointer to the `Player` object, representing the player whose readiness state is to be updated.
 * @param state The new readiness state to set for the player. This is typically a binary value (e.g., 0 for not ready, 1 for ready).
 *
 * @return
 * - `-2` if the game is in the `PLAY_STATE`, meaning the player's readiness cannot be updated at this time.
 * - The result of the `update_ready_player` function if the state is successfully updated.
 *
 * @note The function uses a read-write lock to ensure thread safety while checking and updating the game state.
 *       If the game is in the `PLAY_STATE`, no updates can be made to the player's readiness.
 */
int set_ready_player(Game *g, Player *p, int state) {
    pthread_rwlock_rdlock(&g->mutex);
    if(g->state == PLAY_STATE) {
        pthread_rwlock_unlock(&g->mutex);
        return -2;
    }
    int res = update_ready_player(g->playerList,p,state);
    if(g->state == LOBBY_STATE){
        print_lobbyState(g);
    } else if(g->state == GAME_STATE){
        print_gameState(g);
    }
    pthread_rwlock_unlock(&g->mutex);
    return res;
}
/**
 * @brief Call scripts for generating stats file.
 *
 * Write the datas of the game in a unique file.
 * Call diagramme generator script and pdf generator script
 * Add the partie in the rank file
 * Call script to get the top10 rank for the number of player in the game.
 * @param g
 */
void send_stats(Game*g,Player *p){
    if(g->gameData == NULL){
        return;
    }
    pthread_rwlock_wrlock(&g->mutex);

    create_uf(g->gameData);
    write_data_to_file(g->gameData);
    make_dg(g->gameData->data_fp);
    make_pdf(g->gameData->data_fp);

    char msg[256];
    char *filename = strrchr(g->gameData->data_fp,'/');
    if(filename){
        filename++;
    } else {
        filename = g->gameData->data_fp;
    }
    broadcast_message(g->playerList,p,B_CONSOLE,STAT_FILE_DL,filename,filename);

    pthread_rwlock_unlock(&g->mutex);
}
/**
 * @brief Performs a countdown for the players before starting the card play phase.
 *
 * This function broadcasts a countdown message to all players, starting from a general message indicating
 * the countdown has begun, followed by the numbers 3, 2, and 1, with a specified delay between each message.
 * After the countdown, a final message is broadcasted to indicate that players can start playing their cards.
 *
 * @param g A pointer to the `Game` object where the countdown will be performed.
 *          This pointer must be valid and non-null.
 * @param sleep_delta The time (in seconds) to wait between each countdown message.
 *
 * @note The function uses `sleep` to introduce a delay between the messages, so the countdown is visible to the players.
 *       The final message informs players that they can start playing their cards after the countdown ends.
 */

/*
 * ------------------------
 * Broadcast gobal function
 * ------------------------
 */
void countdown(Game *g,int sleep_delta){
    broadcast_message(g->playerList,NULL,B_CONSOLE,GRN"\nLa partie vas commencer dans : ");
    sleep(sleep_delta);
    broadcast_message(g->playerList,NULL,B_CONSOLE,"3 ");
    sleep(sleep_delta);
    broadcast_message(g->playerList,NULL,B_CONSOLE,"2 ");
    sleep(sleep_delta);
    broadcast_message(g->playerList,NULL,B_CONSOLE,"1 ");
    sleep(sleep_delta);
    broadcast_message(g->playerList,NULL,B_CONSOLE,"Go !\n\n"CRESET);
}
void print_lobbyState(Game* g){
    if (g->state != LOBBY_STATE) return;
    char msg[BUFSIZ] = "";
    strcat(msg, "------ LOBBY ------\n");

    // Ajout du nombre de joueurs
    char temp[256]; // Buffer temporaire pour les ajouts
    snprintf(temp, sizeof(temp), CYN "Nb Joueurs : %d\n" CRESET, g->playerList->count);
    strcat(msg, temp);
    // Ajout des joueurs
    strcat(msg, MAG "Joueurs : ");
    for (int i = 0; i < g->playerList->count; ++i) {
        if (g->playerList->players[i]->ready) {
            snprintf(temp, sizeof(temp), BMAG "%s " CRESET, g->playerList->players[i]->name);
        } else {
            snprintf(temp, sizeof(temp), MAG "%s " CRESET, g->playerList->players[i]->name);
        }
        strcat(msg, temp);
    }
    // Ajout d'un retour à la ligne
    strcat(msg, "\n");
    // Ajout du nombre de joueurs prêts
    snprintf(temp, sizeof(temp), YEL "Nb prêt : [%d/%d]\n" CRESET,
             get_ready_count(g->playerList), g->playerList->count);
    strcat(msg, temp);
    // Ajout de la fin du message
    strcat(msg, "-------------------\n");
    // Envoi du message
    broadcast_message(g->playerList, NULL, B_CONSOLE, msg);
}
void print_gameState(Game* g){
    if (g->state != GAME_STATE) return;
    char msg[BUFSIZ] = "";
    strcat(msg, "------ Partie en cours ------\n");

    // Ajout du nombre de joueurs
    char temp[256]; // Buffer temporaire pour les ajouts
    //Niveau de la prochaine manche
    snprintf(temp,sizeof (temp), BLU"Prochaine manche : %d\n"CRESET,g->round);
    strcat(msg,temp);
    //Nombre de joueurs
    snprintf(temp, sizeof(temp), CYN "Nb Joueurs : %d\n" CRESET, g->playerList->count);
    strcat(msg, temp);
    // Ajout des joueurs
    strcat(msg, MAG "Joueurs : ");
    for (int i = 0; i < g->playerList->count; ++i) {
        if (g->playerList->players[i]->ready) {
            snprintf(temp, sizeof(temp), BMAG "%s " CRESET, g->playerList->players[i]->name);
        } else {
            snprintf(temp, sizeof(temp), MAG "%s " CRESET, g->playerList->players[i]->name);
        }
        strcat(msg, temp);
    }
    // Ajout d'un retour à la ligne
    strcat(msg, "\n");
    // Ajout du nombre de joueurs prêts
    snprintf(temp, sizeof(temp), YEL "Nb prêt : [%d/%d]\n" CRESET,
             get_ready_count(g->playerList), g->playerList->count);
    strcat(msg, temp);
    //Statistiques :
    snprintf(temp,sizeof(temp),RED"Meilleur round : %d\n"CRESET,g->gameData->max_round_lvl);
    strcat(msg,temp);
    snprintf(temp,sizeof(temp),RED"Ratio rounds gagné / rounds : %d/%d\n"CRESET,g->gameData->win_rounds,g->gameData->rounds);
    strcat(msg,temp);
    // Ajout de la fin du message
    strcat(msg, "-----------------------------\n");
    // Envoi du message
    broadcast_message(g->playerList, NULL, B_CONSOLE, msg);
}
void print_playState(Game* g){
    if (g->state != PLAY_STATE) return;

    for (int i = 0; i < g->playerList->count; ++i) {
        Player *p = g->playerList->players[i];
        char msg[BUFSIZ] = "";
        strcat(msg, "------ Manche en cours ------\n");

        char temp[256]; // Buffer temporaire pour les ajouts
        snprintf(temp,sizeof (temp), BLU"Manche : %d\n"CRESET,g->round); //Niveau de la manche
        strcat(msg,temp);
        snprintf(temp,sizeof(temp),MAG"Cartes : "); //Carte du joueur p
        strcat(msg,temp);
        for (int j = 0; j < g->round; ++j) {
            if(p->cards[j] != 0){
                snprintf(temp,sizeof(temp),"%d ",p->cards[j]);
                strcat(msg,temp);
            }
        }
        strcat(msg,"\n"CRESET);
        char* board_msg = format_board(g->board,(g->round*g->playerList->count));
        snprintf(temp,sizeof(temp),YEL"Plateau : %s\n"CRESET,board_msg);
        strcat(msg,temp);
        free(board_msg);
        strcat(msg, "------------------------------\n"); //Fin du message
        send_p(p,msg);
    }

}
void print_classement(Game* g, Player* p){
    int line;
    char **result = get_top10(g->playerList->count,&line);
    broadcast_message(g->playerList,p,0,"----------------------------- Classement -----------------------------\n");
    broadcast_message(g->playerList,p,0,"Rang " CYN"nbJoueurs "MAG"MancheMax "BLU"Joueurs "YEL"Date \n"CRESET);
    for (int i = 0; i < line; ++i) {
        char *entry = result[i];
        char *nbJoueurs = strtok(entry,",");
        char *mancheMax = strtok(NULL, ",");
        char *joueurs = strtok(NULL, ",");
        char *date = strtok(NULL, ",");
        broadcast_message(g->playerList,p,0,"%-5d " CYN "%-10s " MAG "%-10s " BLU "%-20s " YEL "%-10s\n" CRESET,
                          i+1,nbJoueurs,mancheMax,joueurs,date);
        free(result[i]);
    }
    free(result);
    broadcast_message(g->playerList,p,0,"---------------------------------------------------------------------\n");
}
