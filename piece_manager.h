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
    uint8_t * pieceHash;
};

// Struct to pass argument to upload thread
struct uploadArg{
    int sock;
    int msgSize;
};

/////////////////////////////////////////////////////////////////////////
////////////////////// METHOD OTHER FILE WILL CALL //////////////////////
/////////////////////////////////////////////////////////////////////////


// Piece manager look through the file system to see which 
// pieces the client already has.
void startup();

// Give the sock and bitfield of the client that interested 
// in and can receive data from. The bitfield tell which piece
// the peer have
// Should call once when selecting a client
void addBitfield(int sock, uint8_t * bitfield, int bitfieldSize);

// Peer manager call to update the bitfield of peer with the given socket
// Used when the peer manager receive a HAVE message.
void updateBitfield(int sock, int pieceIndex);

// Remove bitfield from list. Meaning client will no longer 
// try to request any piece from the peer with the given socket.
// Modify the list of peer to check
void removeBitfield(int sock);


// Peer manager call this func to tell piece manager to send
// the piece with the pieceHash to sock
// Will create the Upload Manager
// NOTE: will add pipe to list called uploadPipe
void sendPiece(int sock, uint8_t * pieceHash);

// Peer manager call this func to tell piece manager to get the piece
// from the sock
// Will create the thread for the Download Manager
// NOTE: will add pipe to a list called downloadPipe
void getPiece(int sock, int size);


// Peer manager call to find what piece to request next and from whom.
// request - should be initially point to NULL
// Method will allocate data
// Will follow rarest first 
void requestPiece(struct requestPiece * request);


// Periodic call by peer manager to listen to the pipe make
// from upload and download
// When a pipe is found to finish
//  - for download pipe, send download speed to peer
//          NEED: a function in peer manager to tell download speed
//                  also to tell peer manager that it can listen on current pipe again
//  - for upload pipe, let the peer manager know it can now listen to the upload socket
//          NEED: a function in peer manager
// Also need a function in peer manager to let the peer manager know a socket has disconnected
void checkUpLoadDownLoad();





/////////////////////////////////////////////////////////////////////////
//////////////////////////// HELPER /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

// PieceName is a string of 40 character, where every 2 character represent a byte in hex value
// So convert to piece hash which has 20 byte
void convert(uint8_t * pieceHash, char * pieceName, int pieceNameLen);


// Set the bit at pieceIndex to 1
// pieceIndex range from 0 to (number of piece - 1)
// Leftover bits are set to 0
void setHavePiece(uint8_t * bitfield, int pieceIndex);


// Check if the given bitfield have the given piece index
bool havePiece(uint8_t * bitfield, int pieceIndex);

// Check if all pieces had been downloaded
bool haveAllPiece();

// sendPiece will call threadSendPiece to make the thread for the upload manager
void * threadSendPiece(void *vargp);


// getPiece will call threadGetPiece to make the thread for the upload manager
void * threadGetPiece(void *vargp);


