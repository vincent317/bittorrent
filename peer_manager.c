#include <stdlib.h>
#include <stdint.h>
#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <time.h>
#include "shared.h"
#include "cli.h"
#include "torrent_runtime.h"
#include "peer_manager.h"
#include "piece_manager.h"

/*
 * Periodic functions the peer manager need to do:
 * 1. CLI, piece manager, torrent runtime
 * 2. Choking algorithm
 * 3. Send keep alive messages to peers.
 * 4. Send request messages to the tracker 
*/

//Global variables
char peer_id[21] = "zackdazhithong417fin";
long int interval; //interval that the client should send request to the tracker
int complete; //number of peers with the entire file, i.e. seeders (integer)
int incomplete; //number of non-seeder peers, aka "leechers" (integer)
struct Peer *head_peer = NULL;
int number_of_peers = 0;
struct pollfd *peers_sockets;
struct Peer *unchoked_four[4];
Torrent * g_torrent;

void print_ip_address(uint8_t *addr){
    printf("IP Address: ");
    for(int i =0;i<4;i++){
        if(i!=3)
            printf("%d.", addr[i]);
        else
            printf("%d", addr[i]);
    }
    putchar('\n');
}

void print_peer(struct Peer *peer){
    print_ip_address(peer->address);
    printf("Port: %d\n", peer->port);
    printf("Socket: %d\n", peer->socket);
    printf("am_choking: %d, am_interested: %d, peer_choking: %d, peer_interested: %d\n", 
        peer->am_choking, peer->am_interested, peer->peer_choking, peer->peer_interested);
    printf("Download rate: %ld\n", peer->download_rate);
    //TODO: printbitfield
    putchar('\n');
}

void print_peer_list() {
    struct Peer *peer = head_peer;
   
    while(peer != NULL) {
        print_peer(peer);
        peer = peer->next;
    }
}

//Tried to establish connection with the peer, timeouts around 5 secs
int create_peer_connection_socket(uint8_t *addr, uint16_t port){
    int peer_socket;
    if ((peer_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        fprintf(stderr, "socket creation failed\n");
        return -1;
    }

    struct timeval timeout;
	timeout.tv_sec  = 5;  
	timeout.tv_usec = 0; 
	setsockopt(peer_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	setsockopt(peer_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in servAddr; // Server address
    memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
    servAddr.sin_family = AF_INET; // IPv4 address family
    memcpy(&servAddr.sin_addr.s_addr, addr, 4);
    servAddr.sin_port = port;
  
    if (connect(peer_socket, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
        close(peer_socket);
        fprintf(stderr, "socket connection timeout\n");
        return 0;
    }

    return peer_socket;
}

int insert_peerlist_ifnotexists(uint8_t *ip_arr, uint16_t port){
    struct Peer *prev = NULL;
    struct Peer *cur = head_peer;

    while(cur != NULL){
        if(memcmp(cur->address, ip_arr, 4) == 0 && cur->port == port)
            return 1;
        prev = cur;
        cur = cur->next;
    }

    int socket = create_peer_connection_socket(ip_arr, port);
    if(socket){
        number_of_peers ++;
        cur = calloc(sizeof(struct Peer), 1);
        memcpy(cur->address, ip_arr, 4);
        cur->port = port;
        cur->next = NULL;
        cur->socket = socket;
        cur->handshaked = 0;
        cur->peer_choking = 1;
        cur->am_choking = 1;
        gettimeofday(&(cur->last_received_message_time), NULL);
        gettimeofday(&(cur->last_sent_message_time), NULL);
        if(head_peer == NULL){
            head_peer = cur;
        }else{
            prev->next = cur;
        }
        return 1;
    }else{
        return 0;
    }
}

void parse_peers_string(const char *string, int tn){
    for(int i = 0; i < tn;i++){
        uint16_t port;
        memcpy(&port, string + i*6 +4, 2);
        uint8_t ip_arr[4];
        memcpy(ip_arr, string + i*6, 4);

        print_ip_address(ip_arr);
        insert_peerlist_ifnotexists(ip_arr, port);
    }
    print_peer_list();
}

int parse_tracker_response(bencode_t* tracker_response_bencode){
    const char* val;
    int len;

    while (bencode_dict_has_next(tracker_response_bencode)){
        const char* key;
        int keylen;
        bencode_t dict_entry;

        if (!bencode_dict_get_next(tracker_response_bencode, &dict_entry, &key, &keylen))
            return 1;
        
        if (strncmp(key, "failure reason", keylen) == 0) {
            bencode_string_value(&dict_entry, &val, &len);
            printf("Tracker Response with fail: %s\n", val);
            return 1;
        } else if(strncmp(key, "interval", keylen) == 0) {
            bencode_int_value(&dict_entry, &interval);
            printf("The interval the client should send request to the tracker is %ld\n", interval);
        } else if(strncmp(key, "complete", keylen) == 0) {
            bencode_int_value(&dict_entry, (long int *)&complete);
            printf("Complete (# of seeders) is %d\n", complete);
        } else if(strncmp(key, "incomplete", keylen) == 0) {
            bencode_int_value(&dict_entry, (long int *)&incomplete);
            printf("Incomplete (# of leechers) is %d\n", incomplete);
        } else if(strncmp(key, "peers", keylen) == 0) {
            bencode_string_value(&dict_entry, &val, &len);
            parse_peers_string(val, len/6);
        }
    }
    return 0;
}

size_t write_func(void *ptr, size_t size, size_t nmemb, char **write_stream){
    //*write_stream = NULL;
    bencode_t tracker_response_bencode;
    bencode_init(&tracker_response_bencode, ptr, size*nmemb);   
  
    if(parse_tracker_response(&tracker_response_bencode)) {
        printf("Parsing the tracker response failed\n");
        exit(1);
    }
    return size*nmemb;
}

void send_tracker_request(){
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        char url[1000] = {0};

        char *info_hash_encoded = curl_easy_escape(curl, (const char*) g_torrent->info_hash, 20);
        char *peer_id_encoded = curl_easy_escape(curl, peer_id, 20);
        
        sprintf(url, "%s?info_hash=%s&peer_id=%s&port=6881&uploaded=0&downloaded=0&left=%ld&compact=1&event=started", 
            g_torrent->tracker_url, info_hash_encoded, peer_id_encoded, g_torrent->length);

        curl_easy_setopt(curl, CURLOPT_URL, url);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

        /* always cleanup */ 
        curl_free(info_hash_encoded);
        curl_free(peer_id_encoded);
        curl_easy_cleanup(curl);
    }
}

//remove the peer from the linked list, free the bitfield and the peer itself
int remove_from_peer_linked_list(struct Peer *target){
    struct Peer *cur = head_peer;
    struct Peer *prev = NULL;

    while(cur != NULL){
        if(memcmp(cur->address, target->address, 4) == 0 && cur->port == target->port){
            if(cur == head_peer){
                head_peer = cur->next;
            }else{
                if(cur != NULL)
                    prev->next = cur->next;
                else
                    prev->next = NULL;
            }
            if(cur->bitfield != NULL){
                free(cur->bitfield);
            }
            close(cur->socket);
            free(cur);
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }

    return 0;
}

void temp_remove_from_peer_linked_list(struct Peer *target){
    struct Peer *cur = head_peer;
    struct Peer *prev = NULL;

    while(cur != NULL){
        if(memcmp(cur->address, target->address, 4) == 0 && cur->port == target->port){
            if(cur == head_peer){
                head_peer = cur->next;
            }else{
                if(cur != NULL)
                    prev->next = cur->next;
                else
                    prev->next = NULL;
            }
            if(cur->bitfield != NULL){
                cur->bitfield_length = 0;
                free(cur->bitfield);
            }
            free(cur);
        }
        prev = cur;
        cur = cur->next;
    }
}

//Make the pollfd array matches to the sockets in the peers linked list
void update_pollfd(){
    if(peers_sockets != NULL){
        free(peers_sockets);
        peers_sockets = NULL;
    }
    
    if(number_of_peers > 0)
        peers_sockets = malloc(sizeof(struct pollfd) * number_of_peers);
    
    int i = 0;
    struct Peer *peer = head_peer;
    while(peer != NULL){
        peers_sockets[i].fd = peer->socket;
        peers_sockets[i].events = POLLIN;
        peer = peer->next;
        i++;
    }
}

int send_handshake_message(struct Peer *peer){
    uint8_t send_string[68]; //49+19=68
    uint8_t bittorrent_protocol[20] = "BitTorrent protocol";
    uint8_t reserved[8] = {0};
    int temp = 19;
    memcpy(send_string, &temp, 1);
    memcpy(send_string + 1, bittorrent_protocol, 19);
    memcpy(send_string + 20, reserved, 8);
    memcpy(send_string + 28, g_torrent->info_hash, 20);
    memcpy(send_string + 48, peer_id, 20);
    gettimeofday (&(peer->last_sent_message_time), NULL);

    if(send_n_bytes(send_string, 68, peer->socket) == -1){ 
        close(peer->socket);
        remove_from_peer_linked_list(peer);
        update_pollfd();
        number_of_peers --;
        return 0;
    }

    return 1;
}

void send_keep_alve_message(struct Peer *peer){
    uint32_t len = 0;
    if(send_n_bytes(&len, 4, peer->socket) == -1){
        remove_from_peer_linked_list(peer);
        update_pollfd();
        number_of_peers--;
    }else{
        gettimeofday(&(peer->last_sent_message_time), NULL);
    }
}

void send_interested_message(struct Peer *peer, int interested){
    uint32_t len = 1;
    len = htobe32(len);
    uint8_t ID;
    if(interested){
        ID = 2;
    }else{
        ID = 3;
    }
    uint8_t buffer[5];
    memcpy(buffer, &len, 4);
    memcpy(buffer + 4, &ID, 1);
    if(send_n_bytes(buffer, 5, peer->socket) == -1){
        remove_from_peer_linked_list(peer);
        number_of_peers --;
    }else{
        gettimeofday(&(peer->last_sent_message_time), NULL);
    }
}

int send_choked_message(struct Peer *peer, int choked){
    uint32_t len = 1;
    len = htobe32(len);
    uint8_t ID;
    if(choked){
        ID = 0;
    }else{
        ID = 1;
    }
    uint8_t buffer[5];
    memcpy(buffer, &len, 4);
    memcpy(buffer + 4, &ID, 1);
    if(send_n_bytes(buffer, 5, peer->socket) == -1){
        remove_from_peer_linked_list(peer);
        number_of_peers --;
        return -1;
    }else{
        gettimeofday(&(peer->last_sent_message_time), NULL);
        return 0;
    }
}

void send_have_message(struct Peer *peer, int piece_index){
    uint32_t len =5;
    len = htobe32(len);
    uint8_t ID = 4;
    uint8_t buffer[9];
    memcpy(buffer, &len, 4);
    memcpy(buffer + 4, &ID, 1);
    uint32_t p_i = piece_index;
    p_i = htobe32(p_i);

    memcpy(buffer + 5, &p_i, 4);
    if(send_n_bytes(buffer, 9, peer->socket) == -1){
        remove_from_peer_linked_list(peer);
        update_pollfd();
        number_of_peers --;
    }else{
        gettimeofday(&(peer->last_sent_message_time), NULL);
    }
}

void peer_manager_upload_download_complete(uint8_t is_upload, struct Peer* peer, int piece_index){
    //put the peer back to the linked list and the pollfd
    if(head_peer == NULL){
        head_peer = peer;
    }else{
        struct Peer *prev = NULL;
        struct Peer *cur = head_peer;

        while(cur != NULL){
            if(memcmp(cur->address, peer->address, 4) == 0 && cur->port == peer->port)
                return;
            prev = cur;
            cur = cur->next;
        }
        prev->next = cur;
    }
    update_pollfd();

    //Broadcast the have message
    if(!is_upload){
        struct Peer *cur = head_peer;
        while(cur != NULL){
            struct Peer *next = cur->next;
            send_have_message(cur, piece_index);
            cur = next;
        }
        piece_manager_initiate_download();
    }
}

void choking_algorithm(){
    struct Peer *peer = head_peer;

    while(peer != NULL){
        if(peer->bitfield != NULL && peer->peer_choking == 0){
            if(unchoked_four[0] == NULL || peer->download_rate > unchoked_four[0]->download_rate){
                unchoked_four[2] = unchoked_four[1];
                unchoked_four[1] = unchoked_four[0];
                unchoked_four[0] = peer;
            }else if(unchoked_four[1] == NULL || peer->download_rate > unchoked_four[1]->download_rate){
                unchoked_four[2] = unchoked_four[1];
                unchoked_four[1] = peer;
            }else if(unchoked_four[2] == NULL || peer->download_rate > unchoked_four[2]->download_rate){
                unchoked_four[2] = peer;
            }
        }
        peer = peer->next;
    }

    if(unchoked_four[0] != NULL && send_choked_message(unchoked_four[0], 0) == -1){
        unchoked_four[0] = unchoked_four[1];
        unchoked_four[1] = unchoked_four[2];
        unchoked_four[2] = NULL;
    }
    if(unchoked_four[1] != NULL && send_choked_message(unchoked_four[1], 0) == -1){
        unchoked_four[1] = unchoked_four[2];
        unchoked_four[2] = NULL;
    }
    if(unchoked_four[2] != NULL && send_choked_message(unchoked_four[2], 0) == -1){
        unchoked_four[2] = NULL;
    }
    update_pollfd();
}

void optimistic_unchoking(){
    struct Peer *interested_arr[number_of_peers];
    int counter = 0;

    struct Peer *peer;
    while(peer != NULL){
        if(piece_manager_am_interested(peer) && peer->peer_choking == 0 && peer != unchoked_four[0] && peer != unchoked_four[1] && peer != unchoked_four[2]){
            interested_arr[counter] = peer;
            counter ++;
        }
        peer = peer->next;
    }

    if(counter > 0){
        int rand_unchoked = rand() % (counter + 1 - 0) + 0;
        unchoked_four[3] = interested_arr[rand_unchoked];
    }

    if(unchoked_four[3] != NULL && send_choked_message(unchoked_four[3], 0) == -1){
        update_pollfd();
        unchoked_four[3] = NULL;
    }

}

//returns the peer from the given socket
struct Peer *get_peer_from_socket(int socket){
    struct Peer *peer = head_peer;

    while(peer != NULL){
        if(peer->socket == socket)
            return peer;
        peer = peer->next;
    }
    return NULL;
}

// Returns 0 if theres no such peer; 1 otherwise
// TODO: Close the connection if it's open. Clean up anything leftover from the peer.
// TODO: Make sure the piece manager is aware of this as well, or the piece manager adapts correctly
int peer_manager_inform_disconnect(struct Peer *peer) {
    printf("[Peer Manager] Handling peer disconnect...\n");

    if (remove_from_peer_linked_list(peer)) {
        update_pollfd();
        number_of_peers--;
        return 1;
    }
    // piece_manager_initiate_download();

    return 0;
};

//starts the peer manager
int start_peer_manager(Torrent *torrent){
    piece_manager_periodic();
    torrent_runtime_periodic();
    cli_periodic();

    struct timeval choking_algorithm_time, optimistic_unchoking_time, periodic_function_time, tracker_request_time;
    gettimeofday(&choking_algorithm_time, NULL);
    gettimeofday(&optimistic_unchoking_time, NULL);
    gettimeofday(&periodic_function_time, NULL);
    gettimeofday(&tracker_request_time, NULL);
    g_torrent = torrent;
  
    send_tracker_request();
    
    struct Peer *peer = head_peer;
    while(peer != NULL){
        struct Peer *next = peer->next;
        if(send_handshake_message(peer) == 0){
            printf("send handshaking message failed\n");
        }
        peer = next;
    }
    update_pollfd();

    while(1){
        if (poll(peers_sockets, number_of_peers, 0) > 0) {
            int n = number_of_peers;
            for(int i = 0; i<n; i++){
                if(peers_sockets[i].revents == POLLIN){
                    struct Peer *peer = get_peer_from_socket(peers_sockets[i].fd);
                    gettimeofday(&(peer->last_received_message_time), NULL); //refresh the last message received time

                    if(peer->handshaked){
                        uint32_t length;
                        print_ip_address(peer->address);
                        
                        if (read_n_bytes(&length, 4, peers_sockets[i].fd) == -1) {
                            peer_manager_inform_disconnect(peer);
                            continue;
                        };

                        length = be32toh(length);

                        uint8_t ID;

                        if (read_n_bytes(&ID, 1, peers_sockets[i].fd) == -1) {
                            peer_manager_inform_disconnect(peer);
                            continue;
                        };
                        
                        printf("********* Got message from peer! id=%d\n", ID);
                        
                        // choke: <len=0001><id=0>
                        if (ID == 0) {
                            printf("chooked\n");
                            peer->peer_choking = 1;
                            peer->am_choking = 1;
                            send_choked_message(peer, 1);
                        };

                        // unchoke: <len=0001><id=1>
                        if (ID == 1) {
                            printf("unchooked\n");
                            peer->peer_choking = 0;
                            peer->am_choking = 0;
                        };
                        
                        // interested: <len=0001><id=2>
                        if(ID == 2) {
                            printf("interested\n");
                            peer->peer_interested = 1;
                        };
                        
                        // not interested: <len=0001><id=3>
                        if(ID == 3) {
                            printf("uninterested\n");
                            peer->peer_interested = 0;
                        };
                        
                        // have: <len=0005><id=4><piece index>
                        if (ID == 4) {
                            uint32_t piece_index;

                            if (read_n_bytes(&piece_index, 4, peers_sockets[i].fd) == -1) {
                                peer_manager_inform_disconnect(peer);
                                continue;
                            };

                            piece_index = be32toh(piece_index);

                            if(peer->bitfield == NULL){
                                peer->bitfield_length = g_torrent->num_pieces/8 + g_torrent->num_pieces%8;
                                peer->bitfield = calloc(peer->bitfield_length, 1);
                            }
                            set_have_piece(peer->bitfield, piece_index);
                        };

                        // bitfield: <len=0001+X><id=5><bitfield>
                        if(ID == 5){
                            int correct_bitfield = 1;
                            uint8_t bitfield[length-1];

                            if (read_n_bytes(bitfield, length-1, peers_sockets[i].fd) == -1) {
                                peer_manager_inform_disconnect(peer);
                                continue;
                            };

                            print_bitfield(bitfield, length-1);
                            printf("\n");

                            //check if the bitfield length match and if there are any spare bits set
                            if(length-1 ==  (uint32_t) ceil((double) g_torrent->num_pieces / 8)){
                                for(uint32_t j = g_torrent->num_pieces; j < (length-1)*8; j++){
                                    if (have_piece(bitfield, j) == true){
                                        correct_bitfield = 0;
                                    }
                                }
                            }else{
                                correct_bitfield = 0;
                            }

                            if(correct_bitfield){
                                peer->bitfield_length = length-1;
                                peer->bitfield = malloc(length-1);
                                memcpy(peer->bitfield, bitfield, length-1);
                            }else{
                                remove_from_peer_linked_list(peer);
                                number_of_peers--;
                            }
                        };
                        
                        // request: <len=0013><id=6><index><begin><length>
                        if(ID == 6) {
                            //TODO: request mesasge
                        };
                        
                        // piece: <len=0009+X><id=7><index><begin><block>
                        if(ID == 7) {
                            struct timeval timenow;
                            gettimeofday(&timenow, NULL);

                            
                            printf("Got id=7, piece message from peer\n");

                            if((timenow.tv_sec - peer->download_req_sent_time.tv_sec > 5) && 
                                peer->curr_dl == 1){
                                peer->download_rate = 0;
                                peer->curr_dl = 0;
                                peer->curr_dl_piece_idx = 0;
                                peer->am_choking = 1;
                                piece_manager_cancel_request(peer->curr_dl_piece_idx);
                                send_choked_message(peer, 1);
                            }else{
                                gettimeofday(&(peer->last_received_message_time), NULL);
                                struct Peer *new = calloc(sizeof(struct Peer), 1);
                                new->bitfield = calloc(peer->bitfield_length, 1);
                                *new = *peer;
                                memcpy(new->bitfield, peer->bitfield, peer->bitfield_length);
                                //temporarility remove the peer so that the download manager can read the socket
                                temp_remove_from_peer_linked_list(peer);
                                number_of_peers--;
                                piece_manager_create_download_manager(new, new->curr_dl_piece_idx, g_torrent->piece_length, 0);
                            }
                        };
                        
                        // cancel: <len=0013><id=8><index><begin><length>
                        if(ID == 8) {
                            //TODO: cancel message
                        };
                    }else{
                        printf("Handshaking with: %d ", peer->port);
                        print_ip_address(peer->address);
                        uint8_t pstrlen;

                        if (read_n_bytes(&pstrlen, 1, peers_sockets[i].fd) == -1) {
                            peer_manager_inform_disconnect(peer);
                            continue;
                        };

                        uint8_t pstr[pstrlen];

                        if (read_n_bytes(pstr, pstrlen, peers_sockets[i].fd) == -1) {
                            peer_manager_inform_disconnect(peer);
                            continue;
                        };

                        uint8_t reserved[8];

                        if (read_n_bytes(reserved, 8, peers_sockets[i].fd) == -1) {
                            peer_manager_inform_disconnect(peer);
                            continue;
                        };

                        uint8_t infohash[20];

                        if (read_n_bytes(infohash, 20, peers_sockets[i].fd) == -1) {
                            peer_manager_inform_disconnect(peer);
                            continue;
                        };

                        if(memcmp(infohash, g_torrent->info_hash, 20) != 0){
                            remove_from_peer_linked_list(peer);
                            number_of_peers--;
                        }else{
                            peer->handshaked = 1;
                        }

                        uint8_t peerid[21] = {0};

                        if (read_n_bytes(peerid, 20, peers_sockets[i].fd) == -1) {
                            peer_manager_inform_disconnect(peer);
                            continue;
                        };

                        printf("Handshaking success, get peerid %s\n", peerid);
                        printf("Start sending  interested message\n");
                        send_interested_message(peer, 1);
                    }
                }
            }
        }

        update_pollfd();

        //Periodic functions
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        if(current_time.tv_sec - choking_algorithm_time.tv_sec >= 10){
            choking_algorithm();
            gettimeofday(&choking_algorithm_time, NULL);
        }
        if(current_time.tv_sec - optimistic_unchoking_time.tv_sec >= 30){
            optimistic_unchoking();
            gettimeofday(&optimistic_unchoking_time, NULL);
        }
        if(current_time.tv_sec - periodic_function_time.tv_sec > 1){
            printf("---- [running piece manager periodic]\n");
            piece_manager_periodic();
            printf("---- [running torrent periodic]\n");
            torrent_runtime_periodic();
            printf("---- [running cli periodic]\n");
            cli_periodic();
            printf("---- PERIODIC FINISHED ----\n\n\n");
            gettimeofday(&periodic_function_time, NULL);
        }
        if(current_time.tv_sec - tracker_request_time.tv_sec >= interval){
            send_tracker_request();
            gettimeofday(&tracker_request_time, NULL);
        }

        //Drop any peer who has not sent us any messages in 120 seconds
        peer = head_peer;
        struct timeval now;
        gettimeofday(&now, NULL);
        while(peer != NULL){
            struct Peer *next = peer->next;
            if(now.tv_sec - peer->last_received_message_time.tv_sec > 120){
                remove_from_peer_linked_list(peer);
                update_pollfd();
                number_of_peers--;
            }
            peer = next;
        }

        //sent keep alive messages 
        peer = head_peer;
        gettimeofday(&now, NULL);
        while(peer != NULL){
            struct Peer *next = peer->next;
            if(now.tv_sec - peer->last_sent_message_time.tv_sec > 100){
                send_keep_alve_message(peer);
            }
            peer = next;
        }

        update_pollfd();
    }
}

struct Peer *peer_manager_get_root_peer(){
    return head_peer;
}

uint8_t peer_manager_get_piece_length(){
    return g_torrent->piece_length;
}

//get the list of four peers which we unchoked, used for uploading & downloading (choking algorithm)
//some entry may be NULL if there isnt enough peers || enough peers we are interested || enough peers has bitfield data
struct Peer **peer_manager_get_am_unchoked(){
    return unchoked_four;
}

//update the download rate for the peer, the peer manager doesnt call it
int peer_manager_update_download_rate(struct Peer *peer, uint64_t download_rate){
    struct Peer *cur = head_peer;

    while(cur != NULL){
        if(memcmp(cur->address, peer->address, 4) == 0 && cur->port == peer->port){
            peer->download_rate = download_rate;
            return 1;
        }
        peer = peer->next;
    }
    return 0;
}

int peer_manager_begin_download(struct Peer* peer, int pieceIndex){
    if(peer->peer_choking) {
        printf("[Peer Manager] Error, cannot download from peer, they are choking us!\n");
        return 0;
    }
    
    // just in case someone is doing deep copy with the peer data structure
    struct Peer *cur = head_peer;
    while(cur != NULL){
        if(memcmp(cur->address, peer->address, 4) == 0 &&cur->port == peer->port){
            break;
        }
        cur = cur->next;
    }
    if(cur == NULL){
        return 0;
    }

    uint32_t totallen = 13;
    totallen = htobe32(totallen);
    uint8_t ID = 6;
    uint32_t pieceIndex_t = pieceIndex;
    pieceIndex_t = htobe32(pieceIndex_t);
    uint32_t begin = 0;
    uint32_t length = g_torrent->piece_length;
    length = htobe32(length);

    uint8_t buffer[17];
    memcpy(buffer, &totallen, 4);
    memcpy(buffer+4, &ID, 1);
    memcpy(buffer+5, &pieceIndex_t, 4);
    memcpy(buffer+9, &begin, 4);
    memcpy(buffer+13, &length, 4);

    if(send_n_bytes(buffer, 17, cur->socket) == -1){
        printf("[Peer Manager] During begin download, sending download request failed\n");
        remove_from_peer_linked_list(cur);
        update_pollfd();
        number_of_peers --;
        return 0;
    } else {
        printf("[Peer Manager] Sent request to download piece %d, to...\n", pieceIndex);
        print_ip_address(peer->address);
    }
    
    gettimeofday(&(cur->last_sent_message_time), NULL);
    cur->curr_dl = 1;
    cur->curr_dl_piece_idx = pieceIndex;
    gettimeofday(&(cur->download_req_sent_time), NULL);

    return 1;
}

//disconnects from all peers, free any associated memory
void peer_manager_complete(){
    struct Peer *peer = head_peer;

    while(peer != NULL){
        struct Peer *next = peer->next;
        remove_from_peer_linked_list(peer);
        number_of_peers--;
        peer = next;
    }

    update_pollfd();
}