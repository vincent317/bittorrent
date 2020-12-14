
#include "cli.h"
#include "torrent_runtime.h"

int main(int argc, const char** argv) {
    if (argc != 2 && argc != 3) {
        printf("Invalid execution, use: ./bittorrent <torrent path> [debug, y/n]\n");
        return 0;
    }

    g_debug = 0;

    if (argc == 3 && strcmp(argv[2], "y") == 0) {
        printf("Running in debug mode.....\n");
        g_debug = 1;
    }

    // update the root working directory
    getcwd(&g_rootdir[0], sizeof(g_rootdir));

    // create a new torrent runtime
    const char* torrent_path = argv[1];
    create_torrent_runtime(torrent_path);

    return 0;
};

void cli_periodic() {
    // nothing needed here for now...
};
