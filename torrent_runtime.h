
/*
    The Torrent structure contains constant information on a particular torrent
    It does NOT have any state date related to the current progress on downloading
    a torrent or otherwise
*/
struct Torrent {
    uint8_t info_hash[20];
};

/*
    A TorrentRuntime structure stores date related to the runtime or
    execution of a particular torrent.
*/
struct TorrentRuntime {
    struct Torrent* torrent;
};

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
struct TorrentRuntime* create_torrent_runtime(const char* torrent_path, const char* file_path);

/*
    A function called periodically where the Torrent Runtime can perform logic
*/
void torrent_runtime_periodic();
