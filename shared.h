
#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "hash.h"

/*
    Converts a SHA-1 hash (20 bytes) into a string
    The returned string must be free'd
*/
char* sha1_to_hexstr(uint8_t* hash);

/*
    Converts a size in bytes to a string
    The returned string must be free'd
*/
char* calculateSize(uint64_t size);

#endif
