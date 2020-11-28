#include "piece_manager.h"
#include "piece_manager_data.h"

struct peerPiece * peerBitfieldList;



uint8_t * myBitfield;
int maxNumPiece;

void startup(){
    // Set initial for list
    peerBitfieldList = malloc(sizeof(struct peerPiece));
    peerBitfieldList->bitfield = NULL;
    peerBitfieldList->socket = 0;
    peerBitfieldList->next = peerBitfieldList;
    peerBitfieldList->prev = peerBitfieldList;

    // Set initial for list
    initDownloadPipe();
    initUploadPipe();
    initRequestedPiece();



    uint32_t fileLen = 100;        // Get the length field in the torrent file
    uint32_t pieceLen = 9;      // Get the piece length in the torrent file
    maxNumPiece = ceil((double) fileLen / pieceLen);
    myBitfield = malloc((int) ceil((double) maxNumPiece / 8));
    printf("Num piece %d\n", maxNumPiece);
    printf("Size of bitfield %d\n", (int)ceil((double) maxNumPiece / 8));
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

    printf("Folder %s\n", folderPath);
    
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

                printf("%s\n", pieceName);

                // Convert pieceHash to byte
                uint8_t pieceHash[20];
                convert(pieceHash, pieceName, strlen(pieceName));

                // TODO: Find piece index of the pieceHash
            }
        }
        closedir(folder);
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


void addBitfield(int sock, uint8_t * bitfield, int bitfieldSize){
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

void updateBitfield(int sock, int pieceIndex){
    struct peerPiece * t = peerBitfieldList;
    while(t->next != peerBitfieldList){
        if(t->socket == sock){
            setHavePiece(t->bitfield, pieceIndex);
            break;
        }
        t = t->next;
    }
}


void setHavePiece(uint8_t * bitfield, int pieceIndex){
    int posByte = pieceIndex / 8;
    int pos = 7 - (pieceIndex % 8);
    int m = 1 << pos;
    bitfield[posByte] = bitfield[posByte] | m;
}

// Check if the given bitfield have the given piece index
bool havePiece(uint8_t * bitfield, int pieceIndex){
    int posByte = pieceIndex / 8;
    int pos = 7 - (pieceIndex % 8);
    int m = 1 << pos;
    uint8_t temp = bitfield[posByte]; 
    return (temp & m) > 0;
}

// Check if all pieces had been downloaded
bool haveAllPiece(){
    bool result = true;
    for(int i = 0; i < maxNumPiece; i++){
        bool a = havePiece(myBitfield, i);
        printf("%d %d\n", i, a);
        result = result && a;
    }

    return result;
}

void removeBitfield(int sock){
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


void sendPiece(int sock, uint8_t * pieceHash){
    struct downloadArg * data = malloc(sizeof(struct downloadArg));
    data->sock = sock;
    data->pieceHash = malloc(20);
    memcpy(data->pieceHash, pieceHash, 20);

    pthread_t tid1;
    pthread_create(&tid1, NULL, threadSendPiece, (void *)data); 
}

void * threadSendPiece(void *vargp){
    struct downloadArg * data = (struct downloadArg *) vargp;
    int sock = data->sock;
    uint8_t * pieceHash = data->pieceHash;

    int threadID = pthread_self();
    // fd[0] - read
    // fd[1] - write
    int fd[2];
    pipe(fd);
    addUploadPipe(fd[1]);

    // TODO: Call the function to download from download manager

    
    free(data->pieceHash);
    free(vargp);
}


void getPiece(int sock, int size){
    struct uploadArg * data = malloc(sizeof(struct uploadArg));
    data->sock = sock;
    data->msgSize = size;

    pthread_t tid1;
    pthread_create(&tid1, NULL, threadGetPiece, (void *)data); 

}

void * threadGetPiece(void *vargp){
    struct uploadArg * data = (struct uploadArg *) vargp;
    int sock = data->sock;
    int msgSize = data->msgSize;

    int threadID = pthread_self();
    // fd[0] - read
    // fd[1] - write
    int fd[2];
    pipe(fd);
    addDownloadPipe(fd[1]);

    // TODO: Call the function to download from download manager


    free(vargp);
}





int main(int argc, char *argv[]){

    startup();

    uint8_t c[4];
    memset(c, 0, sizeof(char) * 4);

    setHavePiece(c, 19);
    setHavePiece(c, 0);
    setHavePiece(c, 12);
    setHavePiece(c, 5);
    setHavePiece(c, 30);

    printf("%d %d %d %d\n", c[0], c[1], c[2], c[3]);
/*
    for(int i = 0; i < 32; i++){
        bool a = havePiece(c, i);
        printf("%d %d\n", i, a);

    }
*/
    setHavePiece(myBitfield, 0);
    setHavePiece(myBitfield, 1);
    setHavePiece(myBitfield, 2);
    setHavePiece(myBitfield, 3);
    setHavePiece(myBitfield, 4);
    setHavePiece(myBitfield, 5);
    setHavePiece(myBitfield, 6);
    setHavePiece(myBitfield, 7);
    setHavePiece(myBitfield, 8);
    setHavePiece(myBitfield, 9);
    setHavePiece(myBitfield, 10);
    setHavePiece(myBitfield, 11);
    printf("%d", haveAllPiece());

}