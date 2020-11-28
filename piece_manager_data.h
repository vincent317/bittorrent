#ifndef PIECE_MANAGER_DATA_H
#define PIECE_MANAGER_DATA_H

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>

// Use as a list for holding download and upload pipe
struct intList{
    struct intList * next;
    struct intList * prev;

    int value;
};

struct pairList{
    struct intList * next;
    struct intList * prev;

    int sock;
    int pieceIndex;
};

void init_download_pipe();
void init_upload_pipe();
void init_requested_piece();

// add pipe to downloadList
void add_download_pipe(int sock);

// Remove pipe to downloadList
void remove_download_pipe(int sock);

// add pipe to uploadList
void add_upload_pipe(int sock);

// Remove pipe to downloadList
void remove_upload_pipe(int sock);

// add piece index to requested piece list
void add_requested_piece(int pieceIndex);

// Remove piece index to requested piece list
// Call when got the piece or issue with connection 
// so no longer can get the piece
void remove_requested_piece(int pieceIndex);

// Check if given piece index had a request send for
bool currently_requesting_piece(int pieceIndex);

#endif
