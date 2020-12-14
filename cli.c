
#include "cli.h"
#include "torrent_runtime.h"

int main(int argc, const char** argv) {
    if (argc != 2 && argc != 3) {
        printf("Invalid execution, use: ./bittorrent <torrent path> [debug, y/n]\n");
        return 0;
    }

    const char* torrent_path = argv[1];
    create_torrent_runtime(torrent_path);

    return 0;
};

void cli_periodic() {
    // TODO
};
