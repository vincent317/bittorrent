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

// Struct to pass argument to download thread
struct downloadArg{
    int sock;
    int pieceIndex;
    int msgSize;
    int begin;
};

// Struct to pass argument to upload thread
struct uploadArg{
    int sock;
    int pieceIndex;
    int begin;
};

/////////////////////////////////////////////////////////////////////////
////////////////////// METHOD OTHER FILE MAY  CALL //////////////////////
/////////////////////////////////////////////////////////////////////////


// Piece manager look through the file system to see which 
// pieces the client already has.
void piece_manager_startup(Torrent * torrent);

// Return the client bitfield
uint8_t * piece_manager_get_my_bitfield();

// Return the client bitfield's size
int piece_manager_get_my_bitfield_size();

/*
// Give the sock and bitfield of the client that interested 
// in and can receive data from. The bitfield tell which piece
// the peer have
// Should call once when selecting a client
void add_bitfield(int sock, uint8_t * bitfield, int bitfieldSize);

// Peer manager call to update the bitfield of peer with the given socket
// Used when the peer manager receive a HAVE message.
void update_bitfield(int sock, int pieceIndex);

// Remove bitfield from list. Meaning client will no longer 
// try to request any piece from the peer with the given socket.
// Modify the list of peer to check
void remove_bitfield(int sock);
*/

// Peer manager call this func to tell piece manager to send
// the piece with the pieceHash to sock
// Will create the Upload Manager
// NOTE: will add pipe to list called uploadPipe
void piece_manager_send_piece(int sock, int pieceIndex, int begin);

// Peer manager call this func to tell piece manager to get the piece
// from the sock
// Will create the thread for the Download Manager
// NOTE: will add pipe to a list called downloadPipe
void piece_manager_create_download_manager(struct Peer * peer, int pieceIndex, int pieceSize, int begin);

// Call when first startup and have no pieces download yet.
// For the very first peer that reponse with the bitfield and is unchoking client,
// call this function and pass in the bitfield. Will return the pieceIndex of 
// the piece to request from that peer.
int piece_manager_first_download(uint8_t * bitfield);

// Peer manager call to find what piece to request next and from whom.
// request - should be initially point to NULL
// Method will allocate data
// Will follow rarest first 
// May give NULL for peer argument since no one have right condition
void piece_manager_initiate_download();

// Cancel the request
bool piece_manager_cancel_request(int pieceIndex);

// Periodic call by peer manager to listen to the pipe make
// from upload and download
// When a pipe is found to finish
//  - for download pipe, send download speed to peer
//          NEED: a function in peer manager to tell download speed
//                  also to tell peer manager that it can listen on current pipe again
//  - for upload pipe, let the peer manager know it can now listen to the upload socket
//          NEED: a function in peer manager
// Also need a function in peer manager to let the peer manager know a socket has disconnected
void piece_manager_check_upload_download();


/////////////////////////////////////////////////////////////////////////
//////////////////////////// HELPER /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


// Convert pieceHash to 0x folder name
void convert_hash_to_name(uint8_t * pieceHash, char * folderName);


// PieceName is a string of 40 character, where every 2 character represent a byte in hex value
// So convert to piece hash which has 20 byte
void convert_name_to_hash(uint8_t * pieceHash, char * pieceName, int pieceNameLen);

// Check if all pieces had been downloaded
bool have_all_piece();

// sendPiece will call threadSendPiece to make the thread for the upload manager
void * thread_send_piece(void *vargp);


// getPiece will call threadGetPiece to make the thread for the upload manager
void * thread_get_piece(void *vargp);


#endif
