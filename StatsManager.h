
//
// Created by erwan on 17/11/2024.
//

#ifndef THEMIND_STATSMANAGER_H
#define THEMIND_STATSMANAGER_H

#include <bits/types/time_t.h>
#include <bits/types/FILE.h>

typedef struct {
    int card_n; // Card number
    int delta_time; // Time delta
} CardInfo;

typedef struct {
    int round_lvl; // Round level
    time_t start_time; // Starting time of the round (timestamp UNIX)
    CardInfo *cards; // Array of plating cards
    int card_count; // Number of cards played
    int win; // Win or loose round
} RoundData;

typedef struct {
    int player_count; // Player's number
    FILE *file; // Pointer to the data file to stock datas
    RoundData *rounds; // Dynamic array for rounds
    int round_count; // Number of played rounds
} GameData;

GameData* init_game_data(int player_count, const char *filename);
void add_round(GameData * game_data, int round_level);
void end_round(GameData* gameData, int loosing_card);
void add_card(GameData* game_data, int card_number);
void end_game(GameData* game_data);
void free_game_data(GameData* game_data);





#endif //THEMIND_STATSMANAGER_H
