
#include "piece_manager.h"

uint8_t * myBitfield;
int maxNumPiece;
Torrent * torrentCopy;

void piece_manager_startup(Torrent * torrent){
    // Set initial for list
    torrentCopy = torrent;

    // Set initial for list
    init_download_pipe();
    init_upload_pipe();
    init_requested_piece();

    // Create folder for pieces if don't exist
    char cwd1[PATH_MAX];
    if (getcwd(cwd1, sizeof(cwd1)) != NULL) {
        DEBUG_PRINTF("1 Current working dir: %s\n", cwd1);
    }
    
    mkdir(".torrent_data", 0777);
    chdir(".torrent_data");
    mkdir(torrent->hash_str, 0777);
    chdir(torrent->hash_str);
    mkdir("temp", 0777);
    chdir("../../");

    char cwd2[PATH_MAX];
    if (getcwd(cwd2, sizeof(cwd2)) != NULL) {
        DEBUG_PRINTF("2 Current working dir: %s\n", cwd2);
    }    


//    uint32_t fileLen = torrent->length;        // Get the length field in the torrent file
//    uint32_t pieceLen = torrent->piece_length;      // Get the piece length in the torrent file
    DEBUG_PRINTF("HERE! num pieces: %ld\n", torrent->num_pieces);



    maxNumPiece = torrent->num_pieces;
    myBitfield = malloc((int) ceil((double) maxNumPiece / 8));
    memset(myBitfield, 0, (int) ceil((double) maxNumPiece / 8));


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
                    DEBUG_PRINTF("ERROR: Corrupted piece cache. Piece length is not 40, name: '%s'!", pieceName);
                    continue;
                };

                // Convert pieceHash to byte
                uint8_t pieceHash[20];
                hexstr_to_sha1(pieceHash, pieceName);

                // Get piece index from hash and set bitfield
                double pieceIndex = torrent_hash_to_piece_index(pieceHash);
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

void piece_manager_cancel_request_for_peer(struct Peer *  peer){
    remove_request_from_peer(peer->socket);
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

    DEBUG_PRINTF("[Piece Manager] begin up/dl. begin=%d, len=%d, piece=%d\n",
        begin, len, pieceIndex);

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
    // TODO: Remove
    DEBUG_PRINTF("Check TEMP_CURRENTLY_DOWNLOADING %d\n", TEMP_CURRENTLY_DOWNLOADING);
    //print_bitfield(myBitfield, (int) ceil((double) maxNumPiece / 8));
    DEBUG_PRINTF("\n");
    struct Peer * peerList = peer_manager_get_root_peer();

    bool anyInterested = false;
    while (peerList != NULL)
    {
        anyInterested = anyInterested || piece_manager_am_interested(peerList);
        peerList = peerList->next;
    }

    // DEBUG_PRINTF("Is there any peer that I am still interested in? %d\n", anyInterested);
   
    if (TEMP_CURRENTLY_DOWNLOADING) {
        DEBUG_PRINTF("Skipping begin now download, debug mode enabled & download in progress....\n");
        return;
    }
    
    struct Peer * p = peer_manager_get_root_peer();
    int numOpen = 0;
    while(p != NULL){
        if(p->peer_choking == 0 && p->curr_up != 1){
            numOpen++;
        }
        p = p->next;
    }

    struct OpenPeer * listOpenPeer = malloc(numOpen * sizeof(struct OpenPeer));
    p = peer_manager_get_root_peer();
    int index = 0;
    while(p != NULL){
        if(p->peer_choking == 0){
            listOpenPeer[index].peer = p;
            index++;
        }
        p = p->next;
    }

    struct Peer * smallest = NULL;      // The peer that have the minPiece that client will send request to
    int minOccur = INT_MAX;             // The number of peer that have the minPiece 
    int minPiece = -1;                  // The current rarest piece

    DEBUG_PRINTF("[Piece Manager] initiating download.. num pieces: %d!!\n", maxNumPiece);

    for(int i = 0; i < maxNumPiece; i++){
        // Check that client don't have piece and had not send a request for the piece
        /*
        if(i == 85) {
            DEBUG_PRINTF("Checking 86th piece\n");
            DEBUG_PRINTF("Do I not have the piece %d\n Am I currently requesting the piece? %d\n Peer Sock %d\n ",
            !have_piece(myBitfield, i),
            !currently_requesting_piece(i),
            get_peer_socket_from_piece(i));
        }
        */

        if(!have_piece(myBitfield, i) && !currently_requesting_piece(i)){
            // printf("checking for piece %d\n", i);            
            int chance = 6;
            int currentOccur = 0;
            int pos;
            //struct Peer * currentPeer = peer_manager_get_root_peer();
            struct Peer * currentSmallest = NULL;

            for(pos = 0; pos < numOpen; pos++){
                // Client is interested and peer is not choking and 
                // have current look at piece that client don't have
                if(i == 85){
                    /*
                    DEBUG_PRINTF("Had not requesting from this peer %d \n not downloading from this peer %d \n peer is not choking %d \n peer have this piece %d\n", 
                    !currently_requesting_piece_from(listOpenPeer[pos].peer->socket),
                    !(listOpenPeer[pos].peer)->curr_dl,
                    (listOpenPeer[pos].peer)->peer_choking == 0,
                    have_piece((listOpenPeer[pos].peer)->bitfield, i));
                    */
                }

                if(
                    !currently_requesting_piece_from(listOpenPeer[pos].peer->socket) && 
                    !(listOpenPeer[pos].peer)->curr_dl &&
                    (listOpenPeer[pos].peer)->peer_choking == 0 && 
                    have_piece((listOpenPeer[pos].peer)->bitfield, i)
                ) {
                    currentOccur++;

                    // listOpenPeer[pos].peer is sometimes null

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
        DEBUG_PRINTF("[Piece Manager] beginning download on piece index : %d\n", minPiece);

        if (smallest == NULL) {
            DEBUG_PRINTF("-- error: piece manager selected NULL peer!\n");
        }

        // Track piece that have send request for
        add_requested_piece(smallest->socket, minPiece);
    } else {
        DEBUG_PRINTF("[Piece Manager] could not identify piece to download!\n");
    }

    struct Peer* ptr = peer_manager_get_root_peer();
    uint16_t num_unchoked = 0;

    while (ptr != NULL) {
        if (ptr->peer_choking == 0) {
            num_unchoked++;
        }
        
        ptr = ptr->next;
    }

    DEBUG_PRINTF("- %d peers UNCHOKING us\n", num_unchoked);

    free(listOpenPeer);
}

bool piece_manager_cancel_request(uint32_t pieceIndex){
    if(!is_currently_downloading_piece(pieceIndex)){
        remove_requested_piece(pieceIndex);        
        return true;
    }
    return false;
}

void piece_manager_periodic() {
    
    piece_manager_initiate_download();

    struct timeval waitingTime;
    fd_set downloadPipeSet;
    fd_set uploadPipeSet;
    struct pairList * downloadPipeList = get_download_pipe();
    struct pairList * uploadPipeList = get_upload_pipe();

    // Process download pipe
    struct pairList * currentElem = downloadPipeList->next;
    int maxPipe = 0;

    FD_ZERO(&downloadPipeSet);
    while(currentElem != downloadPipeList){
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

    DEBUG_PRINTF("[Piece Manager Periodic] Checking to see if any new DL info came in\n");
    currentElem = downloadPipeList->next;
    while(currentElem != downloadPipeList){
        if(FD_ISSET(currentElem->sock, &downloadPipeSet)){
            char buffer[1];
            read(currentElem->sock, buffer, 1);
            DEBUG_PRINTF("[Piece Manager Periodic] Got code %c\n", buffer[0]);

            int currentPipe = currentElem->sock;
            uint32_t currentPieceIndex = currentElem->pieceIndex;
            int peerSocket = currentElem->peerSock;
            struct Peer * currentPeer = NULL;
            if(peerSocket != -1){
                struct Peer * p = peer_manager_get_root_peer();
                while(p != NULL){
                    if(p->socket == peerSocket){
                        currentPeer = p;
                        break;
                    }
                    p = p->next;
                }
            }

            if(buffer[0] == 's') {
                uint32_t totalRemainingByte = torrentCopy->length - (currentElem->pieceIndex * torrentCopy->piece_length);
                uint32_t length = torrentCopy->piece_length < totalRemainingByte ? torrentCopy->piece_length : totalRemainingByte;
                uint32_t total_subpieces = ceil(length / PIECE_DOWNLOAD_SIZE);

                currentElem = currentElem->prev;
                remove_upload_download_pipe(0, currentPipe);

                // Downloaded the whole piece
             /*   if(!(currentPeer->curr_dl_next_subpiece < total_subpieces)){
                    set_have_piece(myBitfield, currentPieceIndex);
                    remove_requested_piece(currentPieceIndex);
                }
                */
               
                // Delete below when the above is uncommented
                set_have_piece(myBitfield, currentPieceIndex);
                remove_requested_piece(currentPieceIndex);
                

                if(peerSocket != -1){
                    peer_manager_upload_download_complete(0, currentPeer, currentPieceIndex);    
                }

                if(have_all_piece()){
                    printf("------ALL PIECES DOWNLOADED!\n------");
                    peer_manager_complete();
                    file_assembler_begin(torrentCopy);
                }
            }
            else if(buffer[0] == 'r'){
                uint8_t rateMsg[8];
                int total = 0;
                int gotNum = 0;
                while(total < 8){
                    gotNum = read(currentPipe, rateMsg + total, 8 - total);
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
                remove_upload_download_pipe(0, currentPipe);
                remove_requested_piece(currentPieceIndex);

                if(peerSocket != -1){
                    peer_manager_inform_disconnect(currentPeer);  
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
    FD_ZERO(&uploadPipeSet);
    while(currentElem != uploadPipeList){
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
            int currentPipe = currentElem->sock;
            int peerSocket = currentElem->peerSock;
            int currentPieceIndex = currentElem->pieceIndex;

            struct Peer * currentPeer = NULL;
            if(peerSocket != -1){
                struct Peer * p = peer_manager_get_root_peer();
                while(p != NULL){
                    if(p->socket == peerSocket){
                        currentPeer = p;
                        break;
                    }
                    p = p->next;
                }
            }

            if(strcmp(buffer, "s") == 0){
                currentElem = currentElem->prev;
                remove_upload_download_pipe(1, currentPipe);
                peer_manager_upload_download_complete(1, currentPeer, currentPieceIndex);    
            }
            else if(strcmp(buffer, "f") == 0){
                currentElem = currentElem->prev;
                remove_upload_download_pipe(1, currentPipe);

                if(peerSocket != -1){
                    peer_manager_inform_disconnect(currentPeer);
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
