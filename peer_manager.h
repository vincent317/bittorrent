#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "torrent_runtime.h"

struct Peer{
    int socket;
    uint16_t port; //Also in big endian format
    uint8_t address[4]; //The corresponding 64bits version is in big endian format. 
    uint8_t am_choking, am_interested, peer_choking, peer_interested;
    uint64_t download_rate; 
    uint8_t *bitfield; //a dynamically allocated array, 
    int handshaked; //whether this peer has been handshaked
    struct timeval last_received_message_time;
    struct timeval last_sent_message_time;
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

//Returns 0 if theres no such peer; 1 otherwise
int peer_manager_inform_disconnect(struct Peer *peer);

#endif
