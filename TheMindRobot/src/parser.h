//
// Created by erwan on 05/12/2024.
//

#ifndef THEMINDCLIENT_UTILS_H
#define THEMINDCLIENT_UTILS_H
#include <stdio.h>
#include <sys/stat.h>

#define GAME_START 100
#define ROUND_START 101
#define CARD_PLAY 102
#define WIN_ROUND 103
#define LOOSE_ROUND 104
#define CARD 105
#define GO 106
#define ENDGAME 107

#define NULL_MSG (-1)

typedef struct {
    int code;
    char param1[50];
    int param2;
} ServerMsg;

ServerMsg parse_stoc(const char* msg);
void remove_ansi_codes(char *msg);

#endif //THEMINDCLIENT_UTILS_H
