
#include "piece_manager.h"
#include "piece_manager_data.h"

uint8_t * myBitfield;
int maxNumPiece;
Torrent * torrentCopy;

void piece_manager_startup(Torrent * torrent){
    // Set initial for list
  /*  peerBitfieldList = malloc(sizeof(struct peerPiece));
    peerBitfieldList->bitfield = NULL;
    peerBitfieldList->socket = 0;
    peerBitfieldList->next = peerBitfieldList;
    peerBitfieldList->prev = peerBitfieldList;
*/
    torrentCopy = torrent;

    // Set initial for list
    init_download_pipe();
    init_upload_pipe();
    init_requested_piece();



//    uint32_t fileLen = torrent->length;        // Get the length field in the torrent file
//    uint32_t pieceLen = torrent->piece_length;      // Get the piece length in the torrent file
    printf("HERE! num pieces: %d\n", torrent->num_pieces);

    maxNumPiece = torrent->num_pieces;
    myBitfield = malloc((int) ceil((double) maxNumPiece / 8));


    // Bitfield - 1 represent have, 0 represent don't have
    // Part of the Path to the folder that hold the pieces download so far
    
    char folderPath[100];
    memset(folderPath, 0, sizeof(char) * 100);
    folderPath[0] = '\0';
    strcat(folderPath, "./.torrent_data/");
    strcat(folderPath, torrent->hash_str);
   // strcat(folderPath, "/");

    
    DIR *folder;
    struct dirent *file;
    if((folder = opendir(folderPath)) != NULL){
        while((file = readdir(folder)) != NULL){
            if(strlen(file->d_name) > 5){
                char pieceName[41];
                memset(pieceName, 0, sizeof(char) * 41);
                int i = (int) (strrchr(file->d_name, '.') - file->d_name);
                i = i <= 40 ? i : 40;
                memcpy(pieceName, file->d_name, i);
                pieceName[i] = '\0';

                if (strlen(pieceName) != 40) {
                    printf("ERROR: Corrupted piece cache. Piece length is not 40, name: '%s'!", pieceName);
                    exit(1);
                };

                // Convert pieceHash to byte
                uint8_t pieceHash[20];
                hexstr_to_sha1(pieceHash, pieceName);

                // Get piece index from hash and set bitfield
                uint32_t pieceIndex = torrent_hash_to_piece_index(pieceHash);
                if(pieceIndex >= 0){
                    set_have_piece(myBitfield, pieceIndex);
                }
            }
        }
        closedir(folder);
    }
}

int piece_manager_am_interested(struct Peer * peer){
    uint8_t * peerBitfield = peer->bitfield;
    int result = 0;

    for(int i = 0; i < maxNumPiece; i++){
        if(!have_piece(myBitfield, i) && have_piece(peerBitfield, i)){
            result = 1;
            break;
        }
    }

    return result;
}

uint8_t * piece_manager_get_my_bitfield() {
    return myBitfield;
}

int piece_manager_get_my_bitfield_size() {
    return (int) ceil((double) maxNumPiece / 8);
}

/*
    This function will initiate an upload/download by creating
    a new thread and executing "begin_upload_download" - it will
    also create an I/O pipe which will be the primary method of
    communication between the subordiante threead & main thread.
*/
void piece_manager_begin_upload_download(
    int is_upload,
    struct Peer* peer,
    uint32_t pieceIndex,
    int begin,
    int len
) {
    // Create fd's for I/O with the subordinate (0=read, 1=write)
    int fd[2]; // 0=read, 1=write
    pipe(fd);

    // Malloc args for the upload/download manager
    UploadDownloadManagerArgs* args = malloc(sizeof(UploadDownloadManagerArgs));
    args->is_upload = is_upload;
    args->write_fd = fd[1];
    args->peer = peer;
    args->pieceIndex = pieceIndex;
    args->begin = begin;
    args->len = len;

    // Store the descriptor
    record_upload_download_pipe(is_upload, fd[0], pieceIndex, peer->socket);

    // Create a thread for the process
    pthread_t tid;
    pthread_create(&tid, NULL, begin_upload_download, (void*) args);
};

void piece_manager_create_download_manager(
    struct Peer* peer, uint32_t pieceIndex, int pieceSize, int begin
) {
    piece_manager_begin_upload_download(0, peer, pieceIndex, begin, pieceSize);
};

void piece_manager_create_upload_manager(
    struct Peer* peer, uint32_t pieceIndex, int pieceSize, int begin
) {
    piece_manager_begin_upload_download(1, peer, pieceIndex, begin, pieceSize);
};


// Current code can request multiple piece to same peer if that piece is among the rarest.
void piece_manager_initiate_download(){
    struct Peer * p = get_root_peer();
    int numOpen = 0;
    while(p != NULL){
        if(p->am_interested == 1 && p->peer_choking == 0){
            numOpen++;
        }
        p = p->next;
    }

    struct OpenPeer * listOpenPeer = malloc(numOpen * sizeof(struct OpenPeer));
    p = get_root_peer();
    int index = 0;
    while(p != NULL){
        if(p->am_interested == 1 && p->peer_choking == 0){
            listOpenPeer[index].peer = p;
            index++;
        }
        p = p->next;
    }

    struct Peer * smallest = NULL;      // The peer that have the minPiece that client will send request to
    int minOccur = INT_MAX;             // The number of peer that have the minPiece 
    int minPiece = -1;                  // The current rarest piece

    printf("[Piece Manager] initiating download.. num pieces: %d\n", maxNumPiece);

    for(int i = 0; i < maxNumPiece; i++){
        // Check that client don't have piece and had not send a request for the piece
        if(!have_piece(myBitfield, i) && !currently_requesting_piece(i)){
            int chance = 6;
            int currentOccur = 0;
            int pos;
            struct Peer * currentPeer = peer_manager_get_root_peer();
            struct Peer * currentSmallest = NULL;

            for(pos = 0; pos < numOpen; pos++){
                // Client is interested and peer is not choking and 
                // have current look at piece that client don't have
                if(
                    !currently_requesting_piece_from(currentPeer->socket) && 
                    !currentPeer->curr_dl &&
                    // currentPeer->am_interested == 1 && 
                    currentPeer->peer_choking == 0 && 
                    have_piece(currentPeer->bitfield, i)
                ) {
                    currentOccur++;

                    if(chance >= 5){
                        currentSmallest = listOpenPeer[pos].peer;
                    }

                    chance = rand() % 10; 
                }
            }
            if(minOccur > currentOccur && currentOccur != 0){
                minOccur = currentOccur;
                smallest = currentSmallest;
                minPiece = i;
            }
        }
    }


    if(minPiece != -1) {
        printf("[Piece Manager] beginning download on piece index : %d\n", minPiece);
        peer_manager_begin_download(smallest, minPiece);

        // Track piece that have send request for
        add_requested_piece(smallest->socket, minPiece);
    } else {
        printf("[Piece Manager] could not identify piece to download!\n", minPiece);
    }

    free(listOpenPeer);
}

bool piece_manager_cancel_request(uint32_t pieceIndex){
    if(!is_currently_downloading_piece(pieceIndex)){
        remove_requested_piece(pieceIndex);        
        return true;
    }
    return false;
}

void piece_manager_periodic(){
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
            char buffer[1];
            read(currentElem->sock, buffer, 1);

            int currentSocket = currentElem->sock;
            uint32_t currentPieceIndex = currentElem->pieceIndex;
            int peerSocket = currentElem->peerSock;
            struct Peer * currentPeer = NULL;
            if(peerSocket != -1){
                struct Peer * p = get_root_peer();
                while(p != NULL){
                    if(currentPeer->socket == peerSocket){
                        currentPeer = p;
                        break;
                    }
                    p = p->next;
                }
            }

            if(buffer[0] == 's'){
                set_have_piece(myBitfield, currentPieceIndex);
                int peerSocket = get_peer_socket_from_piece_index(currentPieceIndex);
                currentElem = currentElem->prev;
                remove_upload_download_pipe(0, currentSocket);
                remove_requested_piece(currentPieceIndex);

                if(peerSocket != -1){
                    peer_manager_download_complete(currentPeer, currentPieceIndex);    
                }

                if(have_all_piece()){
                    peer_manager_complete();
                    file_assembler_begin(torrentCopy);
                }
            }
            else if(buffer[0] == 'r'){
                uint8_t rateMsg[8];
                int total = 0;
                int gotNum = 0;
                while(total < 8){
                    gotNum = read (currentElem->sock, rateMsg + total, 8 - total);
                    total += gotNum;
                }
                uint64_t downloadRate;
                memcpy(&downloadRate, rateMsg, 8);

                if(peerSocket != -1){
                    peer_manager_update_download_rate(currentPeer, downloadRate);
                }
            }
            else if(buffer[0] == 'f'){
                currentElem = currentElem->prev;
                remove_upload_download_pipe(0, currentSocket);
                remove_requested_piece(currentPieceIndex);

                if(peerSocket != -1){
                    peer_manager_download_complete(currentPeer, currentPieceIndex);    
                }

            }
            else{
                // Unknow message
            }
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
    activity = select(maxPipe + 1, &uploadPipeSet, NULL, NULL, &waitingTime);
    if(activity < 0){
        perror("select() fail for upload pipe");
        exit(EXIT_FAILURE);
    }

    currentElem = uploadPipeList->next;
    while(currentElem != uploadPipeList){
        if(FD_ISSET(currentElem->sock, &uploadPipeSet)){
            char buffer[100];
            read(currentElem->sock, buffer, 19);
            int currentSocket = currentElem->sock;
            int peerSocket = currentElem->peerSock;
            uint32_t currentPieceIndex = currentElem->pieceIndex;

            if(strcmp(buffer, "s") == 0){
                currentElem = currentElem->prev;
                remove_upload_download_pipe(1, currentSocket);
            }
            else if(strcmp(buffer, "f") == 0){
                currentElem = currentElem->prev;
                remove_upload_download_pipe(1, currentSocket);
                
                if(peerSocket != -1){
                    struct Peer * currentPeer = peer_manager_get_root_peer();
                    while(currentPeer != NULL){
                        if(currentPeer->socket == peerSocket){
                            peer_manager_inform_disconnect(currentPeer);
                            break;
                        }
                        currentPeer = currentPeer->next;
                    }
                }
            }
            else{
                // Unknow message
            }
        
        }
        currentElem = currentElem->next;
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

void piece_manager_periodic() {
    // TODO: All periodic code here
    piece_manager_initiate_download();
};
