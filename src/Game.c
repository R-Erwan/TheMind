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
 * @return A pointer to a newly created and initialized `Game` object, or `NULL` if memory allocation fails.
 *
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
 * @param g A pointer to the `Game` object to be freed.
 * @note The function checks if each resource is allocated before freeing it.
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
 *
 * @param g A pointer to the `Game` object to be started.
 * @return 0 if the game was successfully started, or -1 if the game cannot be started
 *         due to invalid conditions (e.g., not all players are ready or the game is already in progress).
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
 * Create gameData for the stats.
 * Update gameState.
 * Start first round.
 *
 * @param g A pointer to the `Game` object where the round will be started.
 * @return 0 if the round was successfully started, or -1 if the round cannot be started
 *         due to invalid conditions (e.g., not all players are ready or the game is already in progress).
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
 * Update Game state.
 * Free temporary ressources like players card and game board, reseting queue.
 *
 * @param g A pointer to the `Game` object where the round will be ended.
 * @param win An integer indicating the outcome of the round:
 *            - 1 if the round was won by the players.
 *            - 0 if the round was lost by the players.
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
 * Update Game State.
 * Generate stats. Write the game in rank_file for ranking. Sending rank to players.
 *
 * @param g A pointer to the `Game` object to be ended.
 * @note The function does not free any game resources, as the game may be resumed or reinitialized.
 *       It only resets the round and game state to their initial values.
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
 * @note The function modifies the players' decks by assigning the cards, and also modifies the game’s
 *       card queue by enqueuing each card as it is distributed. After the distribution, the card queue
 *       is sorted.
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
 * @param g A pointer to the `Game` object, representing the current game state.
 * @param p A pointer to the `Player` object, representing the player whose readiness state is to be updated.
 * @param state The new readiness state to set for the player. This is typically a binary value (e.g., 0 for not ready, 1 for ready).
 *
 * @return
 * - `-2` if the game is in the `PLAY_STATE`, meaning the player's readiness cannot be updated at this time.
 * - The result of the `update_ready_player` function if the state is successfully updated.
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
 * @param g A pointer to the `Game` object, representing the current game state.
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
 * @param g A pointer to the `Game` object where the countdown will be performed.
 * @param sleep_delta The time (in seconds) to wait between each countdown message.
 * @note The function uses `sleep` to introduce a delay between the messages, so the countdown is visible to the players.
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
