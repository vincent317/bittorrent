
#include "shared.h"

const char* sha1_to_hexstr(uint8_t* sha1_hash_binary) {
    char* hash_hex = calloc(sizeof(char) * 41, sizeof(char));

    for (size_t i=0; 20 > i; i++) {
		sprintf(&hash_hex[2*i], "%02x", sha1_hash_binary[i]);
	}

    return (const char*) hash_hex;
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

    for (i = 0; i < DIM(sizes); i++, multiplier /= 1024)
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
