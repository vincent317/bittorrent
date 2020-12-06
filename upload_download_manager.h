
#ifndef DOWNLOAD_MANAGER_H_INCLUDED
#define DOWNLOAD_MANAGER_H_INCLUDED

#include "shared.h"
#include "peer_manager.h"

/*
    A structure you can add arguments to for creating an
    upload or download manager
    -   write_fd = the descriptor upload manager will write updates to
    -   peer = the remote peer we're uploading to / downloading from
    -   pieceIndex = a 0-based index for the piece
    -   begin = a 0-based index for which byte to begin at, relative to the piece!
    -   len = number of bytes to upload. -1 for downloading.

    Note: For download managers, begin should ALWAYS be 0. This is because we do not
    support downloading partial pieces. All 'request' messages sent by this client
    will contain begin=0, so piece begin=0 in the 'piece' response.
*/

typedef struct {
    int is_upload;
    int write_fd;
    struct Peer* peer;
    int pieceIndex;
    int begin;
    int len;
} UploadDownloadManagerArgs;

void* begin_upload_download(void* vargp);

#endif
