#include <string.h>
#include "shared.h"
#include <netinet/tcp.h>


const char* sha1_to_hexstr(uint8_t* sha1_hash_binary) {
    char* hash_hex = calloc(sizeof(char) * 41, sizeof(char));

    for (size_t i=0; 20 > i; i++) {
		sprintf(&hash_hex[2*i], "%02x", sha1_hash_binary[i]);
	}

    return (const char*) hash_hex;
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

int read_n_bytes(void *buffer, int bytes_expected, int read_socket){
    int bytes_received = 0;
    int temp = 0;

    while(bytes_received < bytes_expected){
        temp = recv(read_socket, buffer + bytes_received, bytes_expected-bytes_received, 0);
        if(temp == -1){
            fprintf(stderr, "read failed\n");
            return -1;
        }
        bytes_received += temp;
    }

	return bytes_received;
};

int send_n_bytes(void *buffer, int bytes_expected, int send_socket){
    int bytes_sent = 0;
    int temp = 0;

    while(bytes_sent < bytes_expected){
        temp = send(send_socket, buffer + bytes_sent, bytes_expected-bytes_sent, 0);
        if(temp == -1){
            fprintf(stderr, "send failed\n");
            return -1;
        }
        bytes_sent += temp;
    }

	return bytes_sent;
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


