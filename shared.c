#include <string.h>
#include "shared.h"
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void print_bitfield(uint8_t *bitfield, int length){
    for(int i= 0; i < length; i++){
        for (int j=0; j< 8;j++){
            uint8_t a = (bitfield[i] << j);
            a >>= 7;
            printf("%d", a );
        }
        printf(" ");
    }
}

char* sha1_to_hexstr(uint8_t* sha1_hash_binary) {
    char* hash_hex = calloc(sizeof(char) * 41, sizeof(char));

    for (size_t i=0; 20 > i; i++) {
		sprintf(&hash_hex[2*i], "%02x", sha1_hash_binary[i]);
	}

    return (char*) hash_hex;
};

void get_piece_filename(char* dst, int piece_index, int temp) {
    uint8_t* piece_hash = g_torrent->piece_hashes[piece_index];
    char* piece_hash_str = sha1_to_hexstr(piece_hash);
    
    // TEMP: ./torrent_data/<torrent id>/temp/<piece hash>_<unique thread id>
    // FINAL: ./torrent_data/<torrent id>/<piece hash>
    strcat(dst, g_rootdir);
    strcat(dst, "/.torrent_data/");
    strcat(dst, g_torrent->hash_str);
    strcat(dst, "/");

    if (temp == 1) {
        strcat(dst, "temp/");
        strcat(dst, piece_hash_str);
    } else {
        strcat(dst, piece_hash_str);
    }
};

// Converts a 40-char string into 20-byte hash
void hexstr_to_sha1(uint8_t* dst_hash, char* hex_str){
    int pos;
    uint8_t val;
    int i = 0;
    for(pos = 0; pos < 40; pos += 2){
        char p[3];
        memcpy(p, hex_str + pos, 2);
        p[2] = '\0';
        char * endPtr;
        val = (uint8_t) strtoul(p, &endPtr, 16);
        dst_hash[i] = val;
        i++;
    }
};

int read_n_bytes(void *buffer, int bytes_expected, int read_socket) {
    int bytes_received = 0;
    int bytes_read = 0;

    while(bytes_received < bytes_expected) {
        bytes_read = recv(read_socket, buffer + bytes_received, bytes_expected-bytes_received, 0);

        if(bytes_read <= 0) {
            /* DEV_PRINTF("[Shared] Error: Reading %d bytes from socket %d failed (code=%d)\n",
                // bytes_expected, read_socket, bytes_read); */
            
            return -1;
        }

        bytes_received += bytes_read;
    }

	return bytes_received;
};

int send_n_bytes(void *buffer, int bytes_expected, int send_socket){
    int bytes_sent = 0;
    int temp = 0;

    while(bytes_sent < bytes_expected){
        temp = send(send_socket, buffer + bytes_sent, bytes_expected-bytes_sent, 0);
        if(temp == -1){
            
            return -1;
        }
        bytes_sent += temp;
    }

	return bytes_sent;
};

uint8_t* sha1_file(char* file_path) {
    FILE* fp = fopen(file_path, "r");

    if (!fp) {
        // printf("error! could not open file '%s'\n", file_path);
        return NULL;
    }

    // get file size
    fseek(fp, 0L, SEEK_END);
    long int sz = ftell(fp);
    rewind(fp);

    // read in the data
    uint8_t* file_data = malloc(sizeof(uint8_t) * sz);
    fread(file_data, sz, 1, fp);
    fclose(fp);

    // hash the data
    uint8_t* file_hash = malloc(sizeof(uint8_t) * 20);
    struct sha1sum_ctx* ctx = sha1sum_create(NULL, 0);            
    sha1sum_finish(ctx, file_data, sizeof(uint8_t) * sz, file_hash);
    sha1sum_destroy(ctx);

    free(file_data);
    return file_hash;
};

/*
    Source:
    https://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c
*/

int cp(const char *to, const char *from) {
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT, 0666);
    if (fd_to < 0) {
        close(fd_from);
        if (fd_to >= 0)
            close(fd_to);

        return -1;
    }

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                close(fd_from);
                if (fd_to >= 0)
                    close(fd_to);

                return -1;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            close(fd_from);
            if (fd_to >= 0)
                close(fd_to);

            return -1;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

    return -1;
};

/*
    Source:
    https://stackoverflow.com/questions/3898840/converting-a-number-of-bytes-into-a-file-size-in-c
*/

#define DIM(x) (sizeof(x)/sizeof(*(x)))

static const char     *sizes[]   = { "EiB", "PiB", "TiB", "GiB", "MiB", "KiB", "B" };
static const uint64_t  exbibytes = 1024ULL * 1024ULL * 1024ULL *
                                   1024ULL * 1024ULL * 1024ULL;

char* calculateSize(uint64_t size) {   
    char     *result = (char *) malloc(sizeof(char) * 20);
    uint64_t  multiplier = exbibytes;
    int i;

    for (i = 0; i < (int) DIM(sizes); i++, multiplier /= 1024)
    {   
        if (size < multiplier)
            continue;
        if (size % multiplier == 0)
            sprintf(result, "%lu %s", size / multiplier, sizes[i]);
        else
            sprintf(result, "%.1f %s", (float) size / multiplier, sizes[i]);
        return result;
    }

    strcpy(result, "0");
    return result;
};

void set_have_piece(uint8_t * bitfield, int pieceIndex){
    int posByte = pieceIndex / 8;
    int pos = 7 - (pieceIndex % 8);
    int m = 1 << pos;
    bitfield[posByte] = bitfield[posByte] | m;
}

// Check if the given bitfield have the given piece index
bool have_piece(uint8_t * bitfield, int pieceIndex){
    int posByte = pieceIndex / 8;
    int pos = 7 - (pieceIndex % 8);
    int m = 1 << pos;
    uint8_t temp = bitfield[posByte]; 
    return (temp & m) > 0;
}


