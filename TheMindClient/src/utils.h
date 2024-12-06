//
// Created by erwan on 05/12/2024.
//

#ifndef THEMINDCLIENT_UTILS_H
#define THEMINDCLIENT_UTILS_H
#include <stdio.h>
#include <sys/stat.h>

#define PDF_FILE_MESSAGE 1
typedef struct {
    int port;
    char* filename;
} pdfFile;


void print_brain_art();
void print_file(const char *filename);
int parse_stoc(const char* msg);
pdfFile parse_pdf(const char* msg);
void install();

#endif //THEMINDCLIENT_UTILS_H
