
#include "upload_download_manager.h"
#include "piece_manager.h"

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
        DEBUG_PRINTF("[Download Manager] Read %d bytes from socket=%d\n",
            bytes_read, args->peer->socket);

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
//    write(args->write_fd, "f", sizeof(char));
    Torrent * torrent = piece_manager_get_torrent();
    char * filename = sha1_to_hexstr(torrent->piece_hashes[args->pieceIndex]);
    char filePath[256];
    memset(filePath, 0, sizeof(char) * 256);
    filePath[0] = '\0';
    strcat(filePath, "./.torrent_data/");
    strcat(filePath, torrent->hash_str);
    strcat(filePath, "/");
    strcat(filePath, filename);

    FILE * piece = fopen(filePath, "r");
    if(piece == NULL){
        DEBUG_PRINTF("Fail opening piece %s\n", filePath);
    }
    else{
        DEBUG_PRINTF("Success opening piece %s\n", filePath);
    }
    fseek(piece, 0L, SEEK_END);
    uint64_t fileSize = ftell(piece);
    rewind(piece);

    double remaining = (double) fileSize - (double)args->begin;

    //printf("argslen %d, remaing %f\n", args->len, remaining);

    if(args->len <= 16000 && remaining >= args->len){
        // Send piece: <len=0009+X><id=7><index><begin><block>
        uint32_t bufferLen = 9 + args->len;
        uint8_t ID = 7;
        uint32_t pieceIndex = args->pieceIndex;
        uint32_t begin = args->begin;

        bufferLen = htobe32(bufferLen);
        pieceIndex = htobe32(pieceIndex);
        begin = htobe32(begin);

        uint8_t * buffer = calloc(4 + 9 + args->len, sizeof(uint8_t));
        memcpy(buffer, &bufferLen, 4);
        memcpy(buffer + 4, &ID, 1);
        memcpy(buffer + 5, &pieceIndex, 4);
        memcpy(buffer + 9, &begin, 4);

        fseek(piece, args->begin, SEEK_SET);

        int amountRead = 0;
        while(amountRead < args->len ){
            int numGot = fread(buffer + amountRead + 4 + 9, 1, args->len - amountRead, piece);
            amountRead = amountRead + numGot;
        }
        
        if(send_n_bytes(buffer, 4 + 9 + args->len, args->peer->socket) == -1){
            write(args->write_fd, "f", sizeof(char));
        }
        else{
            write(args->write_fd, "s", sizeof(char));
        }

        free(buffer);
    }
    else{
        // Fail
        write(args->write_fd, "f", sizeof(char));
    } 
    fclose(piece);
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
