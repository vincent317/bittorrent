
#ifndef UPLOAD_MANAGER_H_INCLUDED
#define UPLOAD_MANAGER_H_INCLUDED

/*
    Function to create a new upload manager
    -   write_fd = the descriptor upload manager will write updates to
    -   torrent = the torrent which is being uploaded
    -   peer = the peer which this torrent is being uploaded to
    -   start = a 0-based index for which byte to start upload from
    -   len = number of bytes to upload to the peer
*/
void create_upload_manager(int write_fd, struct Torrent* torrent,
    struct TorrentPeer* peer, uint64_t start, uint64_t bytes);

#endif
