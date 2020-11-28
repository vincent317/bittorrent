
#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/*
    Converts a SHA-1 hash (20 bytes) into a string
    The returned string must be free'd
*/
char* sha1_tostr(uint8_t* hash);

#endif
