#include <stdlib.h>
#include <stdint.h>
#include "torrent_runtime.h"
#include "peer_manager.h"
#include <curl/curl.h>

//20 bytes peer id
char peer_id[21] = "zackdazhithong417fin";
int tracker_response_length;

struct TrackerResponse{
    uint8_t interval;
    char *tracker_id;
    uint8_t complete;
    uint8_t incomplete;
    struct Peer *peers;
};

size_t write_func(void *ptr, size_t size, size_t nmemb, char **write_stream){
    *write_stream = calloc(nmemb, size);
    if (*write_stream == NULL) {
        fprintf(stderr, "calloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(*write_stream, ptr, size*nmemb);
    tracker_response_length = size*nmemb;
    return size*nmemb;
}

int parse_tracker_response(bencode_t* tracker_response_bencode, struct TrackerResponse *trackerresponse){
    while (bencode_dict_has_next(tracker_response_bencode)){
        const char* key;
        int keylen;
        bencode_t dict_entry;

        if (!bencode_dict_get_next(tracker_response_bencode, &dict_entry, &key, &keylen))
            return 1;
        
        if (strncmp(key, "announce", keylen) == 0) {
    }
}

//starts the peer manager
int start_peer_manager(Torrent *torrent){
    CURL *curl;
    CURLcode res;
 
    curl = curl_easy_init();
    if(curl) {
        char url[1000] = {0};
        sprintf(url, "%s?info_hash=%s&peer_id=%s&port=6881&uploaded=0&downloaded=0&left=%ld&compact=1&event=started", 
            torrent->tracker_url,torrent->info_hash, peer_id, torrent->length);
        curl_easy_setopt(curl, CURLOPT_URL, torrent->tracker_url);

        char *response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

        bencode_t torrent_bencode;
        bencode_init(&torrent_bencode, response, tracker_response_length);   
        
        struct TrackerResponse tracker_response = {0};
        if(parse_tracker_response(&torrent_bencode, &tracker_response)) {
            printf("Parsing the tracker response failed\n");
            exit(1);
        }  

        /* always cleanup */ 
        free(response);
        curl_easy_cleanup(curl);
    }
}

int main(int argc, const char** argv){
    if (argc != 2 && argc != 3) {
        printf("Invalid execution, use: ./bittorrent <torrent path> [file/folder?]\n");
        return 0;
    }

    const char* torrent_path = argv[1];
    const char* seed_location = (argc == 2) ? argv[2] : NULL;

    create_torrent_runtime(torrent_path, seed_location);

    return 0;
}