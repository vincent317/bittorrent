
#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


/*
    Converts a SHA-1 hash (20 bytes) into a string
    The returned string must be free'd
*/
char* sha1_tostr(uint8_t* hash);


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
