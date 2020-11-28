
#include "torrent_runtime.h"
#include "bencode.h"

void reset_torrent(Torrent* torrent) {
    torrent->tracker_url = NULL;
    torrent->name = NULL;
    torrent->multiple_files = 0;
    torrent->length = 0;
    torrent->piece_length = 0;
    torrent->piece_hashes = NULL;
};

int parse_bencode(bencode_t* torrent_bencode, Torrent* torrent) {
    const char* val;
    char* str;
    int len;
    reset_torrent(torrent);

    while (bencode_dict_has_next(torrent_bencode)) {
        const char* key;
        int keylen;
        bencode_t dict_entry;
        
        if (!bencode_dict_get_next(torrent_bencode, &dict_entry, &key, &keylen))
            return 1;

        // get the tracker url
        if (strncmp(key, "announce", keylen) == 0) {
            bencode_string_value(&dict_entry, &val, &len);
            str = calloc(len + 1, sizeof(char));
            memcpy(str, val, sizeof(char) * len);
            torrent->tracker_url = str;
        };

        // get the info field
        if (strncmp(key, "info", keylen) == 0) {
            while (bencode_dict_has_next(&dict_entry)) {
                const char* info_key;
                int info_keylen;
                bencode_t info_entry;

                if (!bencode_dict_get_next(&dict_entry, &info_entry, &info_key, &info_keylen))
                    return 1;

                // read the name entry
                if (strncmp(info_key, "name", info_keylen) == 0) {
                    bencode_string_value(&info_entry, &val, &len);
                    str = calloc(len + 1, sizeof(char));
                    memcpy(str, val, sizeof(char) * len);
                    printf("Torrent Name: %s\n", str);
                };

                // read the length field
                if (strncmp(info_key, "length", info_keylen) == 0) {
                    long int len_bytes;
                    bencode_int_value(&info_entry, &len_bytes);
                    torrent->length = (uint64_t) len_bytes;
                    printf("Torrent Size: %ld bytes\n", len_bytes);
                };

                // read the piece length field
                if (strncmp(info_key, "piece length", info_keylen) == 0) {
                    long int len_bytes;
                    bencode_int_value(&info_entry, &len_bytes);
                    torrent->piece_length = (uint64_t) len_bytes;
                };

                // TODO: Read in the pieces hashes

                // TODO: Calculate hash of entire info field
            };
        }; 
    };

    if (
        torrent->tracker_url == NULL ||
        torrent->length == 0 ||
        torrent->piece_length == 0 ||
        torrent -> piece_hashes == NULL
    ) {
        printf("error: .torrent file is missing required field(s)\n");
        return 1;
    }

    return 0;
};

TorrentRuntime* create_torrent_runtime(const char* torrent_path, const char* seed_path) {
    // initialize data structures
    Torrent* torrent = (Torrent*) malloc(sizeof(Torrent));
    TorrentRuntime* runtime = (TorrentRuntime*) malloc(sizeof(TorrentRuntime));
    runtime->torrent = torrent;

    // read the .torrent file into memory
    FILE* metainfo_stream = fopen(torrent_path, "r");

    if (metainfo_stream == NULL) {
        printf("Unable to open .torrent file: %s\n", torrent_path);
        exit(1);
    }

    off_t metainfo_size = lseek(fileno(metainfo_stream), 0, SEEK_END);
    lseek(fileno(metainfo_stream), 0, SEEK_SET);
    uint8_t* metainfo = (uint8_t*) calloc(metainfo_size + 1, sizeof(uint8_t));
    read(fileno(metainfo_stream), metainfo, metainfo_size);

    // use bencoder library to parse .torrent file
    bencode_t torrent_bencode;
    printf("Parsing torrent file at: %s\n", torrent_path);
    bencode_init(&torrent_bencode, (const char*) metainfo, (int) metainfo_size);
    
    if (parse_bencode(&torrent_bencode, torrent)) {
        printf("Invalid .torrent file\n");
        exit(1);
    }

    // create peer manager
    // TODO

    // create piece manager
    // TODO

    return runtime;
};

