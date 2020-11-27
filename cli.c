
#include "cli.h"
#include "torrent_runtime.h"

int main(int argc, const char** argv) {
    if (argc != 2 && argc != 3) {
        printf("Invalid execution, use: ./bittorrent <torrent path> [file/folder?]\n");
        return 0;
    }

    const char* torrent_path = argv[1];
    const char* seed_location = (argc == 2) ? argv[2] : NULL;

    create_torrent_runtime(torrent_path, seed_location);

    return 0;
};
