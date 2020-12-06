
#ifndef TORRENT_RUNTIME_H_INCLUDED
#define TORRENT_RUNTIME_H_INCLUDED

#include "shared.h"
#include "bencode.h"

/*
    The Torrent structure contains constant information on a particular torrent
    It does NOT have any state date related to the current progress on downloading
    a torrent or otherwise
*/
typedef struct {
    struct TorrentFilePath* next;
    char component[256];
} TorrentFilePath;

typedef struct {
    struct TorrentFile* next_file;
    TorrentFilePath* path;
    uint64_t file_len;
    char full_path[2048];
} TorrentFile;

typedef struct {
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

Torrent* g_torrent;

/*
    A TorrentRuntime structure stores date related to the runtime or
    execution of a particular torrent.
*/
typedef struct {
    Torrent* torrent;
} TorrentRuntime;

// Given a hash, this function returns the index of the piece
uint32_t torrent_hash_to_piece_index(uint8_t* hash);

/*
    Called by the CLI when the user wants to download a new torrent
    -   torrent_path = String of location to the .torrent file location
    -   seed_path = Path of location to the file/folder. NULL if not downloaded
        (only provided if the torrent will be seeded)

    This will create a new Torrent Runtime, and do the following:
    -   Intiailize new const / global Torrent Structure
    -   Create a Peer Manager (via create_peer_manager)
    -   Create a Piece Manager (via create_piece_manager)
*/
TorrentRuntime* create_torrent_runtime(const char* torrent_path, const char* seed_path);

/*
    A function called periodically where the Torrent Runtime can perform logic
*/
void torrent_runtime_periodic();

#endif
