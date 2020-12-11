#include "file_assembler.h"

void file_assembler_begin(Torrent * torrent){
    TorrentFile * currentFile = torrent->files;

    uint64_t total = 0;
    uint64_t upperLimit = 0;
    uint64_t lowerLimit = 0;
    uint64_t pieceIndex = 0;
    uint32_t pieceSize = torrent->piece_length;

    while(currentFile != NULL){
        FILE * fp;
        FILE * piece;

        upperLimit = upperLimit + currentFile->file_len;

        TorrentFilePath * path = currentFile->path;
        char backPath[2048];
        chdir('./');
        while(path->next != NULL){
            mkdir(path->component, 0777);

            strcat(backPath, '../');
            chdir(path->component);
            path = path->next;
        }
        chdir(backPath);
        
        fp = fopen(currentFile->full_path, "a");

        while(total < upperLimit){
            const char * filename = sha1_to_hexstr(torrent->piece_hashes[pieceIndex]);
            char filePath[256];
            memset(filePath, 0, sizeof(char) * 256);
            filePath[0] = '\0';
            strcat(filePath, "./.torrent_data/");
            strcat(filePath, torrent->hash_str);
            strcat(filePath, "/");
            strcat(filePath, filename);
            piece = fopen(filePath, "r");

            fseek(piece, 0L, SEEK_END);
            uint64_t fileSize = ftell(piece);
            rewind(piece);

            int v = total + fileSize;
            
            if(pieceIndex * pieceSize < lowerLimit){
                // Add upper part of piece
                int offset = (lowerLimit - (pieceIndex * pieceSize));
                int amountGet = fileSize - offset;
                file_assembler_read_then_write(piece, fp, offset, amountGet);

                pieceIndex++;
                total += amountGet;
            }
            else if(v > upperLimit){
                // Add lower part of piece
                int amountGet = upperLimit - (pieceIndex * pieceSize);
                file_assembler_read_then_write(piece, fp, 0, amountGet);

                total += amountGet;
            }
            else if(lowerLimit < v && v <= upperLimit){
                // Add whole piece
                file_assembler_read_then_write(piece, fp, 0, fileSize);

                pieceIndex++;
                total += fileSize;
            }

            fclose(piece);
            free(filename);
        }

        lowerLimit = upperLimit;
        currentFile = currentFile->next_file;
        fclose(fp);
    }

}


void file_assembler_read_then_write(FILE * readFile, FILE * writeFile, uint64_t beginIndex, uint64_t amount){
    uint8_t buffer[2048];
    uint64_t amountLeft = amount;
    if(beginIndex > 0){
        // Begin reading from offset beginIndex
        fseek(readFile, beginIndex, SEEK_SET);
    }

    while(amountLeft > 0){
        int readAmount = 2064 < amountLeft ? 2064 : amountLeft;
        fread(buffer, 1, readAmount, readFile);
        fwrite(buffer, 1, readAmount, writeFile);
        amountLeft = amountLeft - readAmount;
    }
}
