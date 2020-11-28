#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>

// Use as a list for holding download and upload pipe
struct intList{
    struct intList * next;
    struct intList * prev;

    int value;
};

void initDownloadPipe();
void initUploadPipe();
void initRequestedPiece();

// add pipe to downloadList
void addDownloadPipe(int sock);

// Remove pipe to downloadList
void removeDownloadPipe(int sock);

// add pipe to uploadList
void addUploadPipe(int sock);

// Remove pipe to downloadList
void removeUploadPipe(int sock);

// add piece index to requested piece list
void addRequestedPiece(int sock);

// Remove piece index to requested piece list
// Call when got the piece or issue with connection 
// so no longer can get the piece
void removeRequestedPiece(int sock);