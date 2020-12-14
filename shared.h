
#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

#define DEBUG_PRINTF(f_, ...) { if(g_debug == 1) printf((f_), ##__VA_ARGS__); };

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "hash.h"

int g_debug;

int read_n_bytes(void* buffer, int bytes, int sock);
int send_n_bytes(void* buffer, int bytes, int sock);

/*
    Converts a SHA-1 hash (20 bytes) into a string
    The returned string must be free'd
*/
char* sha1_to_hexstr(uint8_t* hash);

void print_bitfield(uint8_t *bitfield, int length);

// Converts a string into a 20-byte hash
void hexstr_to_sha1(uint8_t* dst_hash, char* hex_str);

/*
    Converts a size in bytes to a string
    The returned string must be free'd
*/
char* calculateSize(uint64_t size);


// Set the bit at pieceIndex to 1
// pieceIndex range from 0 to (number of piece - 1)
// Leftover bits are set to 0
// EX: bitfield: 00000000 pieceIndex: 5
// Change bitfield to 00001000
void set_have_piece(uint8_t * bitfield, int pieceIndex);

// Check if the given bitfield have the given piece index
bool have_piece(uint8_t * bitfield, int pieceIndex);

// Check if all pieces had been downloaded
bool have_all_piece();

#endif
