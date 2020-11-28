#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H

#include <stdlib.h>
#include <stdint.h>
#include "torrent_runtime.h"

/*
    Questions:
    1. What is the period function referring in "Every 50ms, the peer manager will call the “periodic” functions for
     the CLI, Piece Manager, and Torrent Runtime"
    5. Do we only need to broadcast the have message once we got a piece of data? Do we need to broadcast other things
      like choke or interested? (only the have and the request and choking and interested)
*/

//Linked list recommended
struct Peer{
    int socket;
    uint8_t port;
    uint8_t address[16];
    uint8_t am_choking, am_interested, peer_choking, peer_interested;
    uint64_t download_rate; 
    uint8_t *bitfield; //a dynamically allocated array, 
    struct Peer *next;
};

//returns the root peer of the peer linked list
struct Peer *get_root_peer();

//number of bytes in each piece (integer)
uint8_t get_piece_length();

//get the list of four peers which we unchoked, used for uploading & downloading (choking algorithm)
struct Peer *get_am_unchoked();

//update the download rate for the peer, the peer manager doesnt call it
int update_download_rate(struct Peer *peer, uint64_t download_rate);

//returns the peer from the given socket
struct Peer *get_peer_from_socket(int socket);

//starts the peer manager
int start_peer_manager(Torrent *torrent);

#endif
