#include "torrent_runtime.h"
#include "bencode.h"
#include "peer_manager.h"

Torrent* g_torrent = NULL;

void reset_torrent(Torrent* torrent) {
    torrent->hash_str = NULL;
    torrent->tracker_url = NULL;
    torrent->name = "unnamed_torrent";
    torrent->multiple_files = 0;
    torrent->length = 0;
    torrent->piece_length = 0;
    torrent->piece_hashes = NULL;
    torrent->files = NULL;
};

/*
    Parse a bencoded structure and convert it into a torrent

    - announce
    - info
        - files?
            [
             - length
             - path
                ["path", "to", "file"]
            ]
        - name?
        - piece length
        - pieces

*/
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

        if (strncmp(key, "announce", keylen) == 0) {
            bencode_string_value(&dict_entry, &val, &len);
            str = calloc(len + 1, sizeof(char));
            memcpy(str, val, sizeof(char) * len);
            torrent->tracker_url = str;
        };

        if (strncmp(key, "info", keylen) == 0) {
            /*
                Edited by Dazhi on 11/30/2020
                Problem: The hash code here didnt work before. It was giving me inconsistent hash and the info string is also out of bound.
                Fix: Use bencode_dict_get_start_and_len to get the correct info string, do NOT rely on the dict_entry.str & dict_entry.len.
                Tested using the class 1184-0.txt.torrent and matched the reult on the class tracker. 
            */
            int len;
            uint8_t* info_placeholder;
            bencode_dict_get_start_and_len(&dict_entry, (const char**) &info_placeholder, &len);
            uint8_t* ctx_dst = (uint8_t*) torrent->info_hash;
            struct sha1sum_ctx* ctx = sha1sum_create(NULL, 0);            
            sha1sum_finish(ctx, info_placeholder, len, ctx_dst);
            sha1sum_destroy(ctx);

            torrent->hash_str = sha1_to_hexstr(ctx_dst);
            while (bencode_dict_has_next(&dict_entry)) {
                const char* info_key;
                int info_keylen;
                bencode_t info_entry;

                if (!bencode_dict_get_next(&dict_entry, &info_entry, &info_key, &info_keylen))
                    return 1;

                if (strncmp(info_key, "name", info_keylen) == 0) {
                    bencode_string_value(&info_entry, &val, &len);
                    str = calloc(len + 1, sizeof(char));
                    memcpy(str, val, sizeof(char) * len);
                    torrent->name = str;
                };

                if (strncmp(info_key, "length", info_keylen) == 0) {
                    long int len_bytes;
                    bencode_int_value(&info_entry, &len_bytes);
                    torrent->length = (uint64_t) len_bytes;
                };

                if (strncmp(info_key, "files", info_keylen) == 0) {
                    TorrentFile* ptr = NULL;
                    bencode_t file_dict;

                    torrent->multiple_files = 1;

                    // parse each file
                    while (bencode_list_has_next(&info_entry)) {
                        bencode_list_get_next(&info_entry, &file_dict);

                        TorrentFile* file = malloc(sizeof(TorrentFile));
                        TorrentFilePath* last_path = NULL;
                        file->next_file = NULL;
                        file->path = NULL;
                        memset(&file->full_path[0], 0, 2048 * sizeof(char));

                        // parse each field associated with the "file" key
                        while (bencode_dict_has_next(&file_dict)) {
                            bencode_t file_benk;
                            const char* file_field_name;
                            int file_field_len;

                            bencode_dict_get_next(&file_dict, &file_benk, &file_field_name, &file_field_len);
                            
                            if (strncmp(file_field_name, "length", file_field_len) == 0) {
                                if (
                                    !bencode_int_value(&file_benk, (long int*) &file->file_len) ||
                                    file->file_len <= 0
                                ) {
                                    printf("error: .torrent file has corrupted file entry\n");
                                    return 1;
                                }

                                torrent->length += file->file_len;
                            }

                            if (strncmp(file_field_name, "path", file_field_len) == 0) {
                                bencode_t enc_path_entry;
                                char* full_path_ptr = &file->full_path[0];

                                // parse each entry in a file's "path" array
                                while (bencode_list_has_next(&file_benk)) {
                                    bencode_list_get_next(&file_benk, &enc_path_entry);

                                    TorrentFilePath* path_component = malloc(sizeof(TorrentFilePath));
                                    memset(&path_component->component[0], 0, 256 * sizeof(char));

                                    const char* path_str;
                                    int path_len;
                                    bencode_string_value(&enc_path_entry, &path_str, &path_len);

                                    memcpy(&path_component->component[0], path_str, sizeof(char) * path_len);

                                    if (full_path_ptr != &file->full_path[0]) {
                                        *(full_path_ptr++) = '\\';
                                    }

                                    memcpy(full_path_ptr, path_str, sizeof(char) * path_len);
                                    full_path_ptr += path_len;

                                    if (last_path == NULL)
                                        file->path = path_component;
                                    else
                                        last_path->next = (struct TorrentFilePath*) path_component;
                                    
                                    last_path = path_component;
                                }
                            }
                        };

                        if (file->path == NULL) {
                            printf("error: .torrent file entry is missing a path\n");
                            return 1;
                        }

                        if (ptr != NULL)
                            ptr->next_file = (struct TorrentFile*) file;
                        else
                            torrent->files = file;

                        ptr = file;
                    }
                };

                if (strncmp(info_key, "piece length", info_keylen) == 0) {
                    long int len_bytes;
                    bencode_int_value(&info_entry, &len_bytes);
                    torrent->piece_length = (uint64_t) len_bytes;
                };

                if (strncmp(info_key, "pieces", info_keylen) == 0) {
                    if (torrent->piece_length == 0 || torrent->length == 0) {
                        printf("error: .torrent file is missing length and/or piece length field(s)\n");
                        return 1;
                    }

                    int hashes_len;
                    const uint8_t* data;
                    bencode_string_value(&info_entry, (const char**) &data, &hashes_len);

                    torrent->num_pieces =
                        (uint64_t) ceil((double) torrent->length / (double) torrent->piece_length);

                    torrent->piece_hashes = malloc(sizeof(uint8_t*) * torrent->num_pieces);

                    if (hashes_len != (int) torrent->num_pieces * 20) {
                        printf("error: .torrent file is corrupted - invalid pieces hash array\n");
                        return 1;
                    }

                    for (uint64_t i = 0; torrent->num_pieces > i; i++) {
                        uint8_t* piece_hash = malloc(sizeof(uint8_t) * 20);
                        memcpy(piece_hash, data + 20*i, 20 * sizeof(uint8_t));
                        torrent->piece_hashes[i] = piece_hash;
                    }
                }
            };
        }; 
    };

    if (
        torrent->tracker_url == NULL ||
        torrent->piece_hashes == NULL
    ) {
        printf("error: .torrent file is missing required field(s)\n");
        return 1;
    }

    if (torrent->files == NULL) {
        TorrentFilePath* path = malloc(sizeof(TorrentFilePath));
        path->next = NULL;
        strcpy(path->component, torrent->name);

        TorrentFile* file = malloc(sizeof(TorrentFile));
        file->next_file = NULL;
        file->path = path;
        file->file_len = torrent->length;
        strcpy(file->full_path, torrent->name);

        torrent->files = file;
    }

    return 0;
};

//To be implemented
uint32_t torrent_hash_to_piece_index(uint8_t* hash){
    return 0;
}

TorrentRuntime* create_torrent_runtime(const char* torrent_path, const char* seed_path) {
    
    // TODO: Handle Seed Path

    if (g_torrent != NULL) {
        printf("error: already downloading torrent, cannot download another\n");
        return NULL;
    }

    // initialize data structures
    Torrent* torrent = (Torrent*) malloc(sizeof(Torrent));
    TorrentRuntime* runtime = (TorrentRuntime*) malloc(sizeof(TorrentRuntime));
    runtime->torrent = torrent;

    // Update global torrent pointer
    g_torrent = torrent;

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
    bencode_init(&torrent_bencode, (const char*) metainfo, (int) metainfo_size);
    
    if (parse_bencode(&torrent_bencode, torrent)) {
        printf("Invalid .torrent file\n");
        exit(1);
    }

    char* torrent_size = calculateSize(torrent->length);
    char* piece_size = calculateSize(torrent->piece_length);

    printf("Beginning download of torrent:\n");
    printf("- name: %s\n", torrent->name);
    printf("- size: %s\n", torrent_size);
    printf("- pieces: %d x %s\n", (int) torrent->num_pieces, piece_size);
    printf("- files:\n");

    TorrentFile* fp = torrent->files;

    while (fp != NULL) {
        char* file_size = calculateSize(fp->file_len);
        printf("  | %s (%s)\n", fp->full_path, file_size);
        free(file_size);
        fp = (TorrentFile*) fp->next_file;
    }

    start_peer_manager(torrent); // this function should consume all process time

    printf("Program execution complete. Terminating.\n");

    free(piece_size);
    free(torrent_size);
    fclose(metainfo_stream);
    free(metainfo);

    return runtime;
};

void torrent_runtime_periodic() {
    // TODO
};
