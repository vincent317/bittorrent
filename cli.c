
#include "shared.h"

int main(int argc, const char** argv) {
    if (argc != 2 && argc != 3) {
        printf("Invalid execution, use: ./bittorrent <torrent path> [file/folder?]\n");
        return 0;
    }

    const char* torrent_path = argv[1];
    const char* seed_location = (argc == 2) ? argv[2] : NULL;

    printf("%s\n", torrent_path);

    return 0;
};
