#include "piece_manager.h"
#include "piece_manager_data.h"
#include "peer_manager.h"
#include "torrent_runtime.h"
#include "shared.h"

//struct peerPiece * peerBitfieldList;



uint8_t * myBitfield;
int maxNumPiece;

void startup(){
    // Set initial for list
  /*  peerBitfieldList = malloc(sizeof(struct peerPiece));
    peerBitfieldList->bitfield = NULL;
    peerBitfieldList->socket = 0;
    peerBitfieldList->next = peerBitfieldList;
    peerBitfieldList->prev = peerBitfieldList;
*/

    // Set initial for list
    init_download_pipe();
    init_upload_pipe();
    init_requested_piece();



    uint32_t fileLen = 100;        // Get the length field in the torrent file
    uint32_t pieceLen = 9;      // Get the piece length in the torrent file
    maxNumPiece = ceil((double) fileLen / pieceLen);
    myBitfield = malloc((int) ceil((double) maxNumPiece / 8));


    // Bitfield - 1 represent have, 0 represent don't have
    // Part of the Path to the folder that hold the pieces download so far
    
    // TODO: Call function to get torrent id
    char * torrentID = "1234";
    
    char folderPath[100];
    memset(folderPath, 0, sizeof(char) * 100);
    folderPath[0] = '\0';
    strcat(folderPath, "./.torrent_data/");
    strcat(folderPath, torrentID);
   // strcat(folderPath, "/");

    
    DIR *folder;
    struct dirent *file;
    if((folder = opendir(folderPath)) != NULL){
        while((file = readdir(folder)) != NULL){
            if(strlen(file->d_name) > 5){
                char pieceName[41];
                memset(pieceName, 0, sizeof(char) * 41);
                int i = (int) (strrchr(file->d_name, '.') - file->d_name);
                memcpy(pieceName, file->d_name, i);
                pieceName[i] = '\0';

                // Convert pieceHash to byte
                uint8_t pieceHash[20];
                convert(pieceHash, pieceName, strlen(pieceName));

                // Get piece index from hash and set bitfield
                int pieceIndex = torrent_hash_to_piece_index(pieceHash);
                set_have_piece(myBitfield, pieceIndex);
            }
        }
        closedir(folder);
    }
}


uint8_t * get_my_bitfield(){
    return myBitfield;
}





/*
void add_bitfield(int sock, uint8_t * bitfield, int bitfieldSize){
    struct peerPiece * v = malloc(sizeof(struct peerPiece));
    v->socket = sock;
    v->bitfield = malloc(bitfieldSize);
    memcpy(v->bitfield, bitfield, bitfieldSize);

    struct peerPiece * pos = peerBitfieldList->prev;
    v->next = pos->next;
    pos->next->prev = v;
    pos->next = v;
    v->prev = pos;
}

void update_bitfield(int sock, int pieceIndex){
    struct peerPiece * t = peerBitfieldList;
    while(t->next != peerBitfieldList){
        if(t->socket == sock){
            setHavePiece(t->bitfield, pieceIndex);
            break;
        }
        t = t->next;
    }
}

void remove_bitfield(int sock){
    struct peerPiece * t = peerBitfieldList;
    while(t->next != peerBitfieldList){
        if(t->socket == sock){
            t->prev->next = t->next;
            t->next->prev = t->prev;
            free(t->bitfield);
            free(t);
            break;
        }
        t = t->next;
    }
}
*/

void piece_manager_send_piece(int sock, int pieceIndex){
    struct downloadArg * data = malloc(sizeof(struct downloadArg));
    data->sock = sock;
    data->pieceIndex = pieceIndex;

    pthread_t tid1;
    pthread_create(&tid1, NULL, thread_send_piece, (void *)data); 
}

void piece_manager_get_piece(int sock, int size, int pieceIndex){
    struct uploadArg * data = malloc(sizeof(struct uploadArg));
    data->sock = sock;
    data->pieceIndex = pieceIndex;
    data->msgSize = size;

    pthread_t tid1;
    pthread_create(&tid1, NULL, thread_get_piece, (void *)data); 

}

// Current code can request multiple piece to same peer if that piece is among the rarest.
void piece_manager_request_piece(){
    struct Peer * smallest = NULL;      // The peer that have the minPiece that client will send request to
    int minOccur = INT_MAX;             // The number of peer that have the minPiece 
    int minPiece = -1;                  // The current rarest piece

    for(int i = 0; i < maxNumPiece; i++){
        // Check that client don't have piece and had not send a request for the piece
        if(!have_piece(myBitfield, i) && !currently_requesting_piece(i)){
            int chance = 6;
            int currentOccur = 0;
            int currentPiece = -1;
            struct Peer * root = get_root_peer();
            struct Peer * currentPeer = root->next;
            struct Peer * currentSmallest = NULL;

            while(currentPeer != NULL){
                // Client is interested and peer is not choking and 
                // have current look at piece that client don't have
                if(currentPeer->am_interested == 1 && currentPeer->peer_choking == 0 && have_piece(currentPeer->bitfield, i)){
                    currentOccur++;
                    if(chance >= 5){
                        currentSmallest = currentPeer;
                    }
                    chance = rand() % 10; 
                    currentPiece = i;
                }
                currentPeer = currentPeer->next;
            }
            if(minOccur < currentOccur){
                minOccur = currentOccur;
                smallest = currentSmallest;
                minPiece = currentPiece;
            }
        }
    }


    // TODO: call the peer_manager_begin_download to tell peer manager to send message to download piece
    if(minPiece != -1){


        // Track piece that have send request for
        add_requested_piece(minPiece, smallest->socket);
    }
}


void piece_manager_check_upload_download(){
    struct timeval waitingTime;
    fd_set downloadPipeSet;
    fd_set uploadPipeSet;
    struct pairList * downloadPipeList = get_download_pipe();
    struct pairList * uploadPipeList = get_upload_pipe();

    // Process download pipe
    struct pairList * currentElem = downloadPipeList->next;
    int maxPipe = 0;
    while(currentElem != downloadPipeList){
        FD_ZERO(&downloadPipeSet);
        FD_SET(currentElem->sock, &downloadPipeSet);
        maxPipe = maxPipe > currentElem->sock ? maxPipe : currentElem->sock;
        currentElem = currentElem->next;
    }

    waitingTime.tv_sec = 0;
    waitingTime.tv_usec = 100;
    int activity = select(maxPipe + 1, &downloadPipeSet, NULL, NULL, &waitingTime);
    if(activity < 0){
        perror("select() fail for download pipe");
        exit(EXIT_FAILURE);
    }


    currentElem = downloadPipeList->next;
    while(currentElem != downloadPipeList){
        if(FD_ISSET(currentElem->sock, &downloadPipeSet)){
            // TODO: Read from pipe and react as needed
        
        
        
        }
        currentElem = currentElem->next;
    }


    // Process upload pipe

    currentElem = uploadPipeList->next;
    maxPipe = 0;
    while(currentElem != uploadPipeList){
        FD_ZERO(&uploadPipeSet);
        FD_SET(currentElem->sock, &uploadPipeSet);
        maxPipe = maxPipe > currentElem->sock ? maxPipe : currentElem->sock;
        currentElem = currentElem->next;        
    }

    waitingTime.tv_sec = 0;
    waitingTime.tv_usec = 100;
    int activity = select(maxPipe + 1, &uploadPipeSet, NULL, NULL, &waitingTime);
    if(activity < 0){
        perror("select() fail for upload pipe");
        exit(EXIT_FAILURE);
    }

    currentElem = uploadPipeList->next;
    while(currentElem != uploadPipeList){
        if(FD_ISSET(currentElem->sock, &uploadPipeSet)){
            // TODO: Read from pipe and react as needed
        
        
        
        }
        currentElem = currentElem->next;
    }


    
}



void convert(uint8_t * pieceHash, char * pieceName, int pieceNameLen){
    int pos;
    uint8_t val;
    int i = 0;
    for(pos = 0; pos < pieceNameLen; pos += 2){
        char p[3];
        memcpy(p, pieceName + pos, 2);
        p[2] = '\0';
        char * endPtr;
        val = (uint8_t) strtoul(p, &endPtr, 16);
        pieceHash[i] = val;
        i++;
    }
}


// Check if all pieces had been downloaded
bool have_all_piece(){
    bool result = true;
    for(int i = 0; i < maxNumPiece; i++){
        bool a = have_piece(myBitfield, i);
        result = result && a;
    }

    return result;
}



void * thread_send_piece(void *vargp){
    struct downloadArg * data = (struct downloadArg *) vargp;
    int sock = data->sock;
    int pieceIndex = data->pieceIndex;

    pthread_t threadID = pthread_self();
    // fd[0] - read
    // fd[1] - write
    int fd[2];
    pipe(fd);
    add_upload_pipe(fd[1], pieceIndex);

    // TODO: Call the function to download from download manager

    
    free(vargp);
}


void * thread_get_piece(void *vargp){
    struct uploadArg * data = (struct uploadArg *) vargp;
    int sock = data->sock;
    int pieceIndex = data->pieceIndex;
    int msgSize = data->msgSize;

    pthread_t threadID = pthread_self();
    // fd[0] - read
    // fd[1] - write
    int fd[2];
    pipe(fd);

    add_download_pipe(fd[1], pieceIndex);

    // TODO: Call the function to download from download manager


    free(vargp);
}



/*
int main(int argc, char *argv[]){

    startup();

    uint8_t c[4];
    memset(c, 0, sizeof(char) * 4);

    set_have_piece(c, 19);
    set_have_piece(c, 0);
    set_have_piece(c, 12);
    set_have_piece(c, 5);
    set_have_piece(c, 30);

    printf("%d %d %d %d\n", c[0], c[1], c[2], c[3]);

    for(int i = 0; i < 32; i++){
        bool a = havePiece(c, i);
        printf("%d %d\n", i, a);

    }

    set_have_piece(myBitfield, 0);
    set_have_piece(myBitfield, 1);
    set_have_piece(myBitfield, 2);
    set_have_piece(myBitfield, 3);
    set_have_piece(myBitfield, 4);
    set_have_piece(myBitfield, 5);
    set_have_piece(myBitfield, 6);
    set_have_piece(myBitfield, 7);
    set_have_piece(myBitfield, 8);
    set_have_piece(myBitfield, 9);
    set_have_piece(myBitfield, 10);
    set_have_piece(myBitfield, 11);
    printf("%d", have_all_piece());

}
*/

