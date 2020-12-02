#include <stdlib.h>
#include <stdint.h>
#include "torrent_runtime.h"
#include "peer_manager.h"
#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Changes
 * 1. The Peer ip address (in peer_manager.h) has been changed to an array of length 4 instead of an array of length 16 to avoid ambiguity and match with the bittorrent
 * protocol specification. Currently I am storing in the big endian format so it's easir to print. 
 * ex. for (int i =0; i< 4; i++)
 *          printf("%d ", address[i]);
 * It should give you 128 8 126 63 for example
 * 
 * Questions
 * 1. For the handshake part, do I just use a for loop to handshake with every peers and read their handshake responses usinig poll? 
 *    I should not mess with multithread at all right?
 * 2. 
 * 
 * Periodic functions I need to do:
 * 1. CLI, piece manager (piece_manager_check_upload_download() ?), & torrent runtime.
 * 2. Choking algorithm
 * 3. Send keep alive messages to peers.
*/

struct TrackerResponse{
    uint8_t interval;
    char *tracker_id;
    uint8_t complete;
    uint8_t incomplete;
    struct Peer *peers;
};

//Global variables
Torrent *mytorrent;
char peer_id[21] = "zackdazhithong417fin";
int interval; //interval that the client should send request to the tracker
int complete; //number of peers with the entire file, i.e. seeders (integer)
int incomplete; //number of non-seeder peers, aka "leechers" (integer)
struct Peer *head_peer = NULL;
int number_of_peers = 0;

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

void print_peer_list() {
    struct Peer *peer = head_peer;
   
    while(peer != NULL) {
        print_ip_address(peer->address);
        printf("Port: %d\n", peer->port);
        printf("Socket: %d\n", peer->socket);
        printf("am_choking: %d, am_interested: %d, peer_choking: %d, peer_interested: %d\n", 
            peer->am_choking, peer->am_interested, peer->peer_choking, peer->peer_interested);
        printf("Download rate: %d\n", peer->download_rate);
        //TODO: printbitfield
        peer = peer->next;
    }
    putchar('\n');
}

int create_peer_connection_socket(uint8_t addr, uint16_t port){
    int clientSock;
    if ((clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        fprintf(stderr, "socket creation failed\n");
        return -1;
    }
    
    struct sockaddr_in servAddr; // Server address
    memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
    servAddr.sin_family = AF_INET; // IPv4 address family
   // inet_pton(AF_INET, ipAddr, &servAddr.sin_addr.s_addr);
    servAddr.sin_port = port;

    if (connect(clientSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
        fprintf(stderr, "socket connection timeout\n");
        return -1;
    }
    return clientSock;
}

void insert_peerlist_ifnotexists(uint8_t *ip_arr, uint16_t port){
    if(head_peer == NULL){
        struct Peer *peer = calloc(sizeof(struct Peer), 1);
        memcpy(peer->address, ip_arr, 4);
        peer->port = port;
        peer->next = NULL;
        head_peer = peer;
    }else{
        struct Peer *prev = NULL;
        struct Peer *cur = head_peer;

        while(cur != NULL){
            if(memcmp(cur->address, ip_arr, 4) == 0 && cur->port == port)
                return;
            prev = cur;
            cur = cur->next;
        }

        cur = calloc(sizeof(struct Peer), 1);
        memcpy(cur->address, ip_arr, 4);
        cur->port = port;
        cur->next = NULL;
        prev->next = cur;
    }
}

/**
 * Parse the string and update the peers linked list. 
 * @param string A string consisting of multiples of 6 bytes. First 4 bytes are the IP address and last 2 bytes are the port number. 
 *               All in network (big endian) notation.
 * @param length The length of the string
*/
int parse_peers_string(char *string, int length){
    number_of_peers = length/6;

    for(int i = 0; i<number_of_peers;i++){
        uint16_t port;
        memcpy(&port, string + i*6 +4, 2);
        uint8_t ip_arr[4];
        memcpy(ip_arr, string + i*6, 4);

        insert_peerlist_ifnotexists(ip_arr, port);
    }
    print_peer_list();
}

int parse_tracker_response(bencode_t* tracker_response_bencode){
    const char* val;
    char* str;
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
            printf("The interval the client should send request to the tracker is %d\n", interval);
        } else if(strncmp(key, "complete", keylen) == 0) {
            bencode_int_value(&dict_entry, &complete);
            printf("Complete (# of seeders) is %d\n", complete);
        } else if(strncmp(key, "incomplete", keylen) == 0) {
            bencode_int_value(&dict_entry, &incomplete);
            printf("Incomplete (# of leechers) is %d\n", incomplete);
        } else if(strncmp(key, "peers", keylen) == 0) {
            bencode_string_value(&dict_entry, &val, &len);
            if(parse_peers_string(val, len) == 0){
                printf("Parsing peers string failed\n");
            }
        }
    }
    return 0;
}

//starts the peer manager
int start_peer_manager(Torrent *torrent){
    CURL *curl;
    CURLcode res;
    
    mytorrent = torrent;
    curl = curl_easy_init();
    if(curl) {
        char url[1000] = {0};

        char *info_hash_encoded = curl_easy_escape(curl, (const char*)torrent->info_hash, 20);
        char *peer_id_encoded = curl_easy_escape(curl, peer_id, 20);
        
        sprintf(url, "%s?info_hash=%s&peer_id=%s&port=6881&uploaded=0&downloaded=0&left=%ld&compact=1&event=started", 
            torrent->tracker_url, info_hash_encoded, peer_id_encoded, torrent->length);

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

struct Peer *get_root_peer(){
    return head_peer;
}

uint8_t get_piece_length(){
    return mytorrent->piece_length;
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
    return NULL;
}

