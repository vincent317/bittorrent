#include "shared.h"


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
