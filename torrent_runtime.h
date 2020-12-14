
#ifndef TORRENT_RUNTIME_H_INCLUDED
#define TORRENT_RUNTIME_H_INCLUDED

#include "shared.h"
#include "bencode.h"

// Given a hash, this function returns the index of the piece
int torrent_hash_to_piece_index(uint8_t* hash);

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
TorrentRuntime* create_torrent_runtime(const char* torrent_path);

/*
    A function called periodically where the Torrent Runtime can perform logic
*/
void torrent_runtime_periodic();

#endif
