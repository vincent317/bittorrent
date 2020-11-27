#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H

#include <stdlib.h>
#include <stdint.h>

/*
    Questions:
    1. What is the period function referring in "Every 50ms, the peer manager will call the “periodic” functions for
     the CLI, Piece Manager, and Torrent Runtime"
    2. Does bitfield represent which pieces of file a node have? i.e. A file has five pieces, a node with
    the bitfiled 01000 would indicate the node only has the second piece?
    3. Should the peer datastructure be available for all components or should it only be visible to the peer manager? 
    4. What's the frequency of updating Peer Data Structure from peers (choke, interested, etc. )(10 secs?)?
    5. Do we only need to broadcast the have message once we got a piece of data? Do we need to broadcast other things
      like choke or interested? 
    6. Are the list of four peers unchoked used for both uploading and downloading?
*/

struct Peer{
    uint8_t port;
    uint8_t address[16];
    int am_choking, am_interested, peer_choking, peer_interested;
    uint8_t *bitfield; //a dynamically allocated array
};

//number of bytes in each piece (integer)
uint8_t get_piece_length();

//string consisting of the concatenation of all 20-byte SHA1 hash values, one per piece (byte string, i.e. not urlencoded)
uint8_t *get_pieces();

//get the name of the file we are downloading
uint8_t *get_file_name();

//length of the file in bytes (integer)
uint8_t get_file_length();

//check whether we choke a certain peer
int if_am_choked(uint8_t ipaddr, uint8_t port);

//check whether we are interested in a certain peer
int if_am_interested(uint8_t ipaddr, uint8_t port);

//check whether the peer choke us
int if_peer_choked(uint8_t ipaddr, uint8_t port);

//check whether the peer is interested in us
int if_peer_interested(uint8_t ipaddr, uint8_t port);

//get the list of four peers which we unchoked, used for uploading & downloading
struct Peer *get_am_unchoked();

#endif