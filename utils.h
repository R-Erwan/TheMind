//
// Created by erwan on 15/11/2024.
//

#ifndef THEMIND_UTILS_H
#define THEMIND_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QUIT 0
#define READY 1
#define UNREADY 2
#define START 3
#define STOP 4
#define ROBOT_ADD 51
#define ROBOT_REMOVE 52
#define CARD 6

char* format_board(int* board, int size);
int ctoint(const char* cmd);
int hash_cmd(const char* cmd);
#endif //THEMIND_UTILS_H
