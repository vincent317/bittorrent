#include "piece_manager_data.h"


// list of pipe that is downloading
struct pairList * downloadintList;
// list of pipe that is uploading
struct pairList * uploadintList;
// list of piece currently send request to
struct pairList * requestedPieceList;



void init_download_pipe(){
    downloadintList = malloc(sizeof(struct pairList));
    downloadintList->sock = -1;
    downloadintList->pieceIndex = -1;
    downloadintList->next = downloadintList;
    downloadintList->prev = downloadintList;
}

void init_upload_pipe(){
    uploadintList = malloc(sizeof(struct pairList));
    uploadintList->sock = -1;
    uploadintList->pieceIndex = -1;
    uploadintList->next = uploadintList;
    uploadintList->prev = uploadintList;
}

void init_requested_piece(){
    requestedPieceList = malloc(sizeof(struct pairList));
    requestedPieceList->sock = -1;
    requestedPieceList->pieceIndex = -1;
    requestedPieceList->next = requestedPieceList;
    requestedPieceList->prev = requestedPieceList;
}


// add pipe to downloadList
void add_download_pipe(int sock, int pieceIndex){
    struct pairList * v = malloc(sizeof(struct pairList));
    v->sock = sock;
    v->pieceIndex = pieceIndex;

    struct pairList * pos = downloadintList->prev;
    v->next = pos->next;
    pos->next->prev = v;
    pos->next = v;
    v->prev = pos;
}

// Remove pipe to downloadList
void remove_download_pipe(int sock){
    struct pairList * t = downloadintList;
    while(t->next != downloadintList){
        if(t->sock == sock){
            t->prev->next = t->next;
            t->next->prev = t->prev;
            free(t);

            break;
        }
        t = t->next;
    }
}

// Return the list of download pipe
struct pairList * get_download_pipe(){
    return downloadintList;
}

// add pipe to uploadList
void add_upload_pipe(int sock, int pieceIndex){
    struct pairList * v = malloc(sizeof(struct pairList)); 
    v->sock = sock;
    v->pieceIndex = pieceIndex;
    
    struct pairList * pos = uploadintList->prev;
    v->next = pos->next;
    pos->next->prev = v;
    pos->next = v;
    v->prev = pos;
}

// Remove pipe to uploadList
void remove_upload_pipe(int sock){
    struct pairList * t = uploadintList;
    while(t->next != uploadintList){
        if(t->sock == sock){
            t->prev->next = t->next;
            t->next->prev = t->prev;
            free(t);
            break;
        }
        t = t->next;
    }
}

// Return the list of upload pipe
struct pairList * get_upload_pipe(){
    return uploadintList;
}

// add piece index to requested piece list
void add_requested_piece(int sock, int pieceIndex){
    struct pairList * v = malloc(sizeof(struct pairList));
    v->sock = sock;
    v->pieceIndex = pieceIndex;
    

    struct pairList * pos = requestedPieceList->prev;
    v->next = pos->next;
    pos->next->prev = v;
    pos->next = v;
    v->prev = pos;
}

// Remove piece index to requested piece list
// Call when got the piece or issue with connection 
// so no longer can get the piece
void remove_requested_piece(int pieceIndex){
    struct pairList * t = requestedPieceList;
    while(t->next != requestedPieceList){
        if(t->pieceIndex == pieceIndex){
            t->prev->next = t->next;
            t->next->prev = t->prev;
            free(t);
            break;
        }
        t = t->next;
    }
}

bool currently_requesting_piece(int pieceIndex){
    struct pairList * t = requestedPieceList;
    while(t->next != requestedPieceList){
        if(t->pieceIndex == pieceIndex){
            return true;
        }
        t = t->next;
    }
    return false;
}