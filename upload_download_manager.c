
#include "upload_download_manager.h"

uint8_t BUFFER[32000];

void create_download_manager(UploadDownloadManagerArgs* args) {
    DEBUG_PRINTF("[Download Manager] ==========================\n");
    /* DEBUG_PRINTF("[DL] reading %d bytes from socket=%d\n",
        args->len, args->peer->socket); */
    
    int bytes_read = read_n_bytes(&BUFFER[0], args->len, args->peer->socket);

    if (bytes_read == -1) {
        // DEBUG_PRINTF("[DL] Error reading data from peer! (bytes_read=%d)\n", bytes_read);
        write(args->write_fd, "f", sizeof(char));
        return;
    } else {
        DEBUG_PRINTF("[Download Manager] Successfully read %d bytes from peer!\n", bytes_read);
    }

    write(args->write_fd, "s", sizeof(char));
};

void create_upload_manager(UploadDownloadManagerArgs* args) {
    DEBUG_PRINTF("Created upload manager!\n");
    write(args->write_fd, "f", sizeof(char));
};

void* begin_upload_download(void* vargp) {
    UploadDownloadManagerArgs* args = (UploadDownloadManagerArgs*) vargp;

    if (args->is_upload) {
        create_upload_manager(args);
    } else {
        create_download_manager(args);
    }

    free(vargp);
    return NULL;
};
