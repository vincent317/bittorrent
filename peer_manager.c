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
 * Periodic functions I need to do:
 * 1. CLI, piece manager, torrent runtime
 * 2. Choking algorithm
 * 3. Send keep alive messages to peers.
*/

// TODO: Call this function every 500ms
void handle_periodic() {
    piece_manager_periodic();
    torrent_runtime_periodic();
    cli_periodic();
};

struct TrackerResponse{
    uint8_t interval;
    char *tracker_id;
    uint8_t complete;
    uint8_t incomplete;
    struct Peer *peers;
};

//Global variables
char peer_id[21] = "zackdazhithong417fin";
long int interval; //interval that the client should send request to the tracker
int complete; //number of peers with the entire file, i.e. seeders (integer)
int incomplete; //number of non-seeder peers, aka "leechers" (integer)
struct Peer *head_peer = NULL;
int number_of_peers = 0;
struct pollfd *peers_sockets;

size_t write_func(void *ptr, size_t size, size_t nmemb, char **write_stream){
    bencode_t tracker_response_bencode;
    bencode_init(&tracker_response_bencode, ptr, size*nmemb);   
  
    if(parse_tracker_response(&tracker_response_bencode)) {
        printf("Parsing the tracker response failed\n");
        exit(1);
    }
    return size*nmemb;
}

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

    // int syn_retries = 1; // Send a total of 3 SYN packets => Timeout ~5s
    // setsockopt(peer_socket, IPPROTO_TCP, TCP_SYNCNT, &syn_retries, sizeof(syn_retries));

    struct timeval timeout;
	timeout.tv_sec  = 2;  
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
    if(head_peer == NULL){
        int temp;
        if((temp = create_peer_connection_socket(ip_arr, port))){
            struct Peer *peer = calloc(sizeof(struct Peer), 1);
            memcpy(peer->address, ip_arr, 4);
            peer->port = port;
            peer->next = NULL;
            peer->socket = temp;
            head_peer = peer;
            return 1;
        }else{
            return 0;
        }
    }else{
        struct Peer *prev = NULL;
        struct Peer *cur = head_peer;

        while(cur != NULL){
            if(memcmp(cur->address, ip_arr, 4) == 0 && cur->port == port)
                return 1;
            prev = cur;
            cur = cur->next;
        }

        int temp;
        if((temp = create_peer_connection_socket(ip_arr, port))){
            number_of_peers ++;
            cur = calloc(sizeof(struct Peer), 1);
            memcpy(cur->address, ip_arr, 4);
            cur->port = port;
            cur->next = NULL;
            cur->socket = temp;
            cur->handshaked = 0;
            prev->next = cur;
            return 1;
        }else{
            return 0;
        }
        
    }
}

/**
 * Parse the string and update the peers linked list. 
 * @param string A string consisting of multiples of 6 bytes. First 4 bytes are the IP address and last 2 bytes are the port number. 
 *               All in network (big endian) notation.
 * @param length The length of the string
*/
void parse_peers_string(char *string){
    for(int i = 0; i<number_of_peers;i++){
        uint16_t port;
        memcpy(&port, string + i*6 +4, 2);
        uint8_t ip_arr[4];
        memcpy(ip_arr, string + i*6, 4);

        if(insert_peerlist_ifnotexists(ip_arr, port) == 0)
            number_of_peers --;
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
            bencode_int_value(&dict_entry, &complete);
            printf("Complete (# of seeders) is %d\n", complete);
        } else if(strncmp(key, "incomplete", keylen) == 0) {
            bencode_int_value(&dict_entry, &incomplete);
            printf("Incomplete (# of leechers) is %d\n", incomplete);
        } else if(strncmp(key, "peers", keylen) == 0) {
            bencode_string_value(&dict_entry, &val, &len);
            number_of_peers = number_of_peers ? number_of_peers : len/6; //The number of peers would be zero in the first tracker request
            parse_peers_string(val);
        }
    }
    return 0;
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

//It does close the socket
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
            free(cur);
            close(cur->socket);
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

void handshake(){
    struct Peer *peer = head_peer;

    while(peer != NULL){
        if(peer->handshaked == 0){
            uint8_t send_string[68]; //49+19=68
            uint8_t bittorrent_protocol[20] = "BitTorrent protocol";
            uint8_t reserved[8] = {0};
            int temp = 19;
            memcpy(send_string, &temp, 1);
            memcpy(send_string + 1, bittorrent_protocol, 19);
            memcpy(send_string + 20, reserved, 8);
            memcpy(send_string + 28, g_torrent->info_hash, 20);
            memcpy(send_string + 48, peer_id, 20);

            if(send_n_bytes(send_string, 68, peer->socket) == -1){ 
                close(peer->socket);
                remove_from_peer_linked_list(peer);
                number_of_peers --;
            }
        }
        peer = peer->next;
    }
}

//Make the pollfd array matches to the sockets in the peers linked list
void update_pollfd(){
    if(peers_sockets != NULL)
        free(peers_sockets);
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

//starts the peer manager
int start_peer_manager(Torrent *torrent){
    send_tracker_request();
    handshake();
    update_pollfd();

    while(1){
        if (poll(peers_sockets, number_of_peers, 0) > 0 ){
            for(int i = 0;i <number_of_peers; i++){
                if(peers_sockets[i].revents == POLLIN){
                    struct Peer *peer = get_peer_from_socket(peers_sockets[i].fd);
                    gettimeofday(&peer->last_message_time, NULL); //refresh the last message received time

                    if(peer->handshaked){
                        uint64_t length;
                        read_n_bytes(&length, 4, peers_sockets[i].fd);
                        length = be64toh(length);

                        uint8_t ID;
                        read_n_bytes(&ID, 1, peers_sockets[i].fd);
                        
                        if(ID == 5){
                            int correct_bitfield = 1;
                            uint8_t bitfield[length-1];
                            read_n_bytes(bitfield, length-1, peers_sockets[i].fd);
                            /*
                                Question:
                                How can we know how many bits are involved in the pass-in bitfield? 
                                Right now I can only check whether the bitfield has any spare bits set. 
                            */
                            for(int j = g_torrent->num_pieces; j < (length-1)*8; j++){
                                if (have_piece(bitfield, j) == true){
                                    correct_bitfield = 0;
                                }
                            }
                            if(correct_bitfield){
                                g_torrent->num_pieces;
                                peer->bitfield = malloc(length-1);
                                memcpy(peer->bitfield, bitfield, length-1);
                            }else{
                                remove_from_peer_linked_list(peer);
                            }
                        // }else if(ID == ){

                        }
                    }else{
                        printf("Handshaking with: %d ", peer->port);
                        print_ip_address(peer->address);
                        uint8_t pstrlen;
                        read_n_bytes(&pstrlen, 1, peers_sockets[i].fd);
                        uint8_t pstr[pstrlen];
                        read_n_bytes(pstr, pstrlen, peers_sockets[i].fd);
                        uint8_t reserved[8];
                        read_n_bytes(pstr, 8, peers_sockets[i].fd);
                        uint8_t infohash[20];
                        read_n_bytes(infohash, 20, peers_sockets[i].fd);
                        if(memcmp(infohash, g_torrent->info_hash, 20) != 0){
                            remove_from_peer_linked_list(peer);
                        }else{
                            peer->handshaked = 1;
                        }
                        uint8_t peerid[21] = {0};
                        read_n_bytes(peerid, 20, peers_sockets[i].fd);
                        printf("Handshaking success, get peerid %s\n", peerid);
                    }
                }
            }
        }

        //Drop any peer who has not sent no messaged in two minutes
        struct Peer *peer = head_peer;
        while(peer != NULL){
            struct timeval now;
            gettimeofday(&now, NULL);
            if(now.tv_sec - peer->last_message_time.tv_sec > 120){
                remove_from_peer_linked_list(peer);
            }
            peer=peer->next;
        }
        update_pollfd();
    }
    
}

struct Peer *get_root_peer(){
    return head_peer;
}

uint8_t get_piece_length(){
    return g_torrent->piece_length;
}

//get the list of four peers which we unchoked, used for uploading & downloading (choking algorithm)
struct Peer *get_am_unchoked(){
    return NULL;
}

//update the download rate for the peer, the peer manager doesnt call it
int update_download_rate(struct Peer *peer, uint64_t download_rate){
    return 0;
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

//Returns 0 if theres no such peer; 1 otherwise
int peer_manager_inform_disconnect(struct Peer *peer){
    if (remove_from_peer_linked_list(peer)) {
        update_pollfd();
        number_of_peers--;
        return 1;
    }else{
        return 0;
    }
}