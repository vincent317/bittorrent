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
struct pairList{
    struct pairList * next;
    struct pairList * prev;

    int sock;
    int pieceIndex;
};

void init_download_pipe();
void init_upload_pipe();
void init_requested_piece();

// add pipe to downloadList
void add_download_pipe(int sock, int pieceIndex);

// Remove pipe to downloadList
void remove_download_pipe(int sock);

// Return the list of download pipe
struct pairList * get_download_pipe();

// add pipe to uploadList
void add_upload_pipe(int sock, int pieceIndex);

// Remove pipe to downloadList
void remove_upload_pipe(int sock);

// Return the list of upload pipe
struct pairList * get_upload_pipe();


// add piece index to requested piece list
void add_requested_piece(int sock, int pieceIndex);

// Remove piece index to requested piece list
// Call when got the piece or issue with connection 
// so no longer can get the piece
void remove_requested_piece(int pieceIndex);

// Check if given piece index had a request send for
bool currently_requesting_piece(int pieceIndex);

int get_peer_socket_from_piece_index(int pieceIndex);

#endif
