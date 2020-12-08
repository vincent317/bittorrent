#ifndef PIECE_MANAGER_H
#define PIECE_MANAGER_H

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/select.h>
#include "shared.h"
#include "torrent_runtime.h"
#include "peer_manager.h"
#include "upload_download_manager.h"

struct requestPiece{
    int index;              // Request piece's index
    int socket;             // Peer's socket that own that piece
};

struct peerPiece{
    struct peerPiece * next;
    struct peerPiece * prev;
    uint8_t * bitfield;
    int socket;                 // Socket of peer that have the bitfield
};

struct downloadArg{
    int sock;
    uint32_t pieceIndex;
    int msgSize;
    int begin;
};

// Struct to pass argument to upload thread
struct uploadArg{
    int sock;
    uint32_t pieceIndex;
    int begin;
};

struct OpenPeer{
    struct Peer * peer;
};

/////////////////////////////////////////////////////////////////////////

// A function called every 500ms for perioid tasks
void piece_manager_periodic();

// Piece manager look through the file system to see which 
// pieces the client already has.
void piece_manager_startup(Torrent * torrent);

// Check if client is interested in peer
int piece_manager_am_interested(struct Peer * peer);

// Return the client bitfield
uint8_t * piece_manager_get_my_bitfield();

// Return the client bitfield's size
int piece_manager_get_my_bitfield_size();

/*
    The Peer Manger calls this function to begin reading a piece from a peer's socket.
    This occurs after a peer sends a "piece" message.
*/
void piece_manager_create_download_manager(struct Peer * peer, uint32_t pieceIndex, int pieceSize, int begin);

// Peer manager call to find what piece to request next and from whom.
// request - should be initially point to NULL
// Method will allocate data
// Will follow rarest first 
// May give NULL for peer argument since no one have right condition
void piece_manager_initiate_download();

// Cancel the request
bool piece_manager_cancel_request(uint32_t pieceIndex);

// Periodic call by peer manager to listen to the pipe make
// from upload and download
// When a pipe is found to finish
//  - for download pipe, send download speed to peer
//          NEED: a function in peer manager to tell download speed
//                  also to tell peer manager that it can listen on current pipe again
//  - for upload pipe, let the peer manager know it can now listen to the upload socket
//          NEED: a function in peer manager
// Also need a function in peer manager to let the peer manager know a socket has disconnected
void piece_manager_periodic();

// Check if all pieces had been downloaded
bool have_all_piece();

#endif
