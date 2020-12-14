
#include "upload_download_manager.h"

/*
    NOTE: Do not use chdir since chdir is process wide!
*/

void create_download_manager(UploadDownloadManagerArgs* args) {
    DEBUG_PRINTF("[Download Manager] ==========================\n");
    /*  DEBUG_PRINTF("[Download Manager] Reading... piece=%d, begin=%d, len=%d\n",
        args->pieceIndex, args->begin, args->len); */

    /* DEBUG_PRINTF("[DL] reading %d bytes from socket=%d\n",
        args->len, args->peer->socket); */
    
    uint8_t* BUFFER = malloc(256000 * sizeof(uint8_t));
    int bytes_read = read_n_bytes(&BUFFER[0], args->len, args->peer->socket);

    if (bytes_read == -1) {
        // DEBUG_PRINTF("[DL] Error reading data from peer! (bytes_read=%d)\n", bytes_read);
        write(args->write_fd, "f", sizeof(char));
        close(args->write_fd);
        return;
    } else {
        // get the piece file
        char piece_temp_filename[1024] = {0};
        get_piece_filename(piece_temp_filename, args->pieceIndex, 1);

        // open the file
        FILE* piece_tmp = fopen(piece_temp_filename, (args->begin == 0) ? "w" : "a");

        if (piece_tmp == NULL) {
            DEBUG_PRINTF("[Download Manager] Error downloading piece!\n");
            return;
        } else {
            fseek(piece_tmp, 0, SEEK_END);
        }

        long int cursor_pos = ftell(piece_tmp);

        fwrite(&BUFFER[0], 1, (size_t) bytes_read, piece_tmp);
        fclose(piece_tmp);

        DEBUG_PRINTF("[Download Manager] Read & wrote piece data. %d bytes piece=%d -> temp file. Filesize=%d.\n",
            bytes_read, args->pieceIndex, (int) (cursor_pos + bytes_read));
        
        write(args->write_fd, "s", sizeof(char));
        close(args->write_fd);
        return;
    }
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
