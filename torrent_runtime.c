
#include "torrent_runtime.h"

TorrentRuntime* create_torrent_runtime(const char* torrent_path, const char* seed_path) {
    // initialize data structures
    Torrent* torrent = (Torrent*) malloc(sizeof(Torrent));
    TorrentRuntime* runtime = (TorrentRuntime*) malloc(sizeof(TorrentRuntime));
    runtime->torrent = torrent;

    // use bencoder library to parse .torrent file
    bencode_t torrent_bencode;
    bencode_init(&torrent_bencode, )

    // create peer manager
    // TODO

    // create piece manager
    // TODO

    return runtime;
};

