
#include "cli.h"
#include "torrent_runtime.h"
#include <arpa/inet.h>

int main(int argc, const char** argv) {
    if (argc != 3  && argc != 5) {
        printf("Invalid execution, use: ./bittorrent <torrent path> [debug, y/n]\n");
        return 0;
    }

    g_debug = 0;

    if (argc == 3 && strcmp(argv[2], "y") == 0) {
        printf("Running in debug mode.....\n");
        direct_connected = 0;
        g_debug = 1;
    }else if(argc ==5){
        direct_connected = 1;
        uint32_t temp;
        inet_pton(AF_INET, argv[2], &temp);
        memcpy(direct_connect_address, &temp, 4);
        direct_connect_port = htons(atoi(argv[3]));

        if(argc == 5){
        if (strcmp(argv[2], "y") == 0)
            g_debug = 1;
        }
    }

    // update the root working directory
    getcwd(&g_rootdir[0], sizeof(g_rootdir));
    DEBUG_PRINTF("Working Directory: %s\n", g_rootdir);

    // create a new torrent runtime
    const char* torrent_path = argv[1];
    create_torrent_runtime(torrent_path);

    return 0;
};

void cli_periodic() {
    // nothing needed here for now...
};
