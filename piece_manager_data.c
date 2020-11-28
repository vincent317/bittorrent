#include "piece_manager_data.h"


// list of pipe that is downloading
struct intList * downloadintList;
// list of pipe that is uploading
struct intList * uploadintList;
// list of piece currently send request to
struct intList * requestedPieceList;



void init_download_pipe(){
    downloadintList = malloc(sizeof(struct intList));
    downloadintList->value = 0;
    downloadintList->next = downloadintList;
    downloadintList->prev = downloadintList;
}

void init_upload_pipe(){
    uploadintList = malloc(sizeof(struct intList));
    uploadintList->value = 0;
    uploadintList->next = uploadintList;
    uploadintList->prev = uploadintList;
}

void init_requested_piece(){
    requestedPieceList = malloc(sizeof(struct intList));
    requestedPieceList->value = 0;
    requestedPieceList->next = requestedPieceList;
    requestedPieceList->prev = requestedPieceList;
}


// add pipe to downloadList
void addDownloadPipe(int sock){
    struct intList * v = malloc(sizeof(struct intList));
    v->value = sock;

    struct intList * pos = downloadintList->prev;
    v->next = pos->next;
    pos->next->prev = v;
    pos->next = v;
    v->prev = pos;
}

// Remove pipe to downloadList
void removeDownloadPipe(int sock){
    struct intList * t = downloadintList;
    while(t->next != downloadintList){
        if(t->value == sock){
            t->prev->next = t->next;
            t->next->prev = t->prev;
            free(t);

            break;
        }
        t = t->next;
    }
}

// add pipe to uploadList
void addUploadPipe(int sock){
    struct intList * v = malloc(sizeof(struct intList));
    v->value = sock;

    struct intList * pos = uploadintList->prev;
    v->next = pos->next;
    pos->next->prev = v;
    pos->next = v;
    v->prev = pos;
}

// Remove pipe to uploadList
void removeUploadPipe(int sock){
    struct intList * t = uploadintList;
    while(t->next != uploadintList){
        if(t->value == sock){
            t->prev->next = t->next;
            t->next->prev = t->prev;
            free(t);
            break;
        }
        t = t->next;
    }
}

// add piece index to requested piece list
void addRequestedPiece(int pieceIndex){
    struct intList * v = malloc(sizeof(struct intList));
    v->value = pieceIndex;

    struct intList * pos = requestedPieceList->prev;
    v->next = pos->next;
    pos->next->prev = v;
    pos->next = v;
    v->prev = pos;
}

// Remove piece index to requested piece list
// Call when got the piece or issue with connection 
// so no longer can get the piece
void removeRequestedPiece(int pieceIndex){
    struct intList * t = requestedPieceList;
    while(t->next != requestedPieceList){
        if(t->value == pieceIndex){
            t->prev->next = t->next;
            t->next->prev = t->prev;
            free(t);
            break;
        }
        t = t->next;
    }
}

bool currently_requesting_piece(int pieceIndex){
    struct intList * t = requestedPieceList;
    while(t->next != requestedPieceList){
        if(t->value == pieceIndex){
            return true;
        }
        t = t->next;
    }
    return false;
}
