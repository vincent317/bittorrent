
#include "shared.h"

const char* sha1_to_hexstr(uint8_t* sha1_hash_binary) {
    char* hash_hex = calloc(sizeof(char) * 41, sizeof(char));

    for (size_t i=0; 20 > i; i++) {
		sprintf(&hash_hex[2*i], "%02x", sha1_hash_binary[i]);
	}

    return (const char*) hash_hex;
};
