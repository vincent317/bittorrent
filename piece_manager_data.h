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
    int peerSock;
};

void init_download_pipe();
void init_upload_pipe();
void init_requested_piece();

void record_upload_download_pipe(int is_upload, int sock, int pieceIndex, int peerSock);
void remove_upload_download_pipe(int is_upload, int sock);

// Return the list of download pipe
struct pairList * get_download_pipe();

// Return true if currently downloading the pieceIndex
bool is_currently_downloading_piece(int pieceIndex);

// Return the list of upload pipe
struct pairList * get_upload_pipe();

// add piece index to requested piece list
void add_requested_piece(int sock, int pieceIndex);

// Remove piece index to requested piece list
// Call when got the piece or issue with connection 
// so no longer can get the piece
void remove_requested_piece(int pieceIndex);

// Remove the request to the peer with the given sock
void remove_request_from_peer(int sock);

// Check if given piece index had a request send for
bool currently_requesting_piece(int pieceIndex);

// Check if a ongoing request had been send to the given socket
bool currently_requesting_piece_from(int sock);

// find the peer socket that is used to request the given pieceIndex
int get_peer_socket_from_piece(int pieceIndex);

#endif
