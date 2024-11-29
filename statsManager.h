//
// Created by erwan on 23/11/2024.
//

#include <bits/types/FILE.h>
#include "hpdf.h"

#ifndef TEST_STATMANAGERV2_H
#define TEST_STATMANAGERV2_H

#endif //TEST_STATMANAGERV2_H
#define DATA_DIR "../datas"
/**
 * @warning Before use this module ensure that the project have the correct file and directory structure,
 *          datas,pdf,ressources, and scripts folders.
 */

typedef struct {
    int player_count; // Player's number
    int rounds;
    int win_rounds;
    int max_round_lvl;
    int loosing_cards[100];
    double avg_reaction_time[100];
    int cards[100];
    int *round_list;
    char data_fp[30];
} GameData;

GameData* create_gm();
void free_gm(GameData* gm);
char *create_uf(GameData *gm);
void add_card(GameData* gm, int card, time_t reaction_time);
void add_round(GameData* gm, int round_lvl, int win);
void add_loosing_card(GameData *gm, int card, time_t reaction_time);
int write_data_to_file(GameData* gm);
int make_dg(const char* data_fp);
int make_pdf(const char* data_fp);
