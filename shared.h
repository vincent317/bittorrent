
#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

#define MAX_PIECE_SIZE 256000
#define PIECE_DOWNLOAD_SIZE 15000.0
#define DEBUG_PRINTF(f_, ...) { if(g_debug == 1) printf((f_), ##__VA_ARGS__); };
#define DEBUG_PRINT_NET(f_, ...) { if(false) printf((f_), ##__VA_ARGS__); };
#define UNUSED(x) (void)(x)


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "hash.h"

/*
    The Torrent structure contains constant information on a particular torrent
    It does NOT have any state date related to the current progress on downloading
    a torrent or otherwise
*/
typedef struct TorrentFilePath {
    struct TorrentFilePath* next;
    char component[256];
} TorrentFilePath;

typedef struct TorrentFile {
    struct TorrentFile* next_file;
    TorrentFilePath* path;
    uint64_t file_len;
    char full_path[2048];
} TorrentFile;

typedef struct Torrent {
    uint8_t info_hash[20];
    const char* hash_str;
    const char* tracker_url;
    const char* name;
    uint8_t multiple_files;
    uint64_t length;
    uint64_t num_pieces;
    uint64_t piece_length;
    uint8_t** piece_hashes;
    TorrentFile* files;
} Torrent;

/*
    A TorrentRuntime structure stores date related to the runtime or
    execution of a particular torrent.
*/
typedef struct {
    Torrent* torrent;
} TorrentRuntime;

Torrent* g_torrent;
char g_rootdir[1024];
int g_debug;

int read_n_bytes(void* buffer, int bytes, int sock);
int send_n_bytes(void* buffer, int bytes, int sock);

void get_piece_filename(char* dst, int piece_index, int temp);
int cp(const char *to, const char *from);
uint8_t* sha1_file(char* file_path);

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
