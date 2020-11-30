#include <stdlib.h>
#include <stdint.h>
#include "torrent_runtime.h"
#include "peer_manager.h"
#include <curl/curl.h>

struct TrackerResponse{
    uint8_t interval;
    char *tracker_id;
    uint8_t complete;
    uint8_t incomplete;
    struct Peer *peers;
};

//20 bytes peer id
char peer_id[21] = "zackdazhithong417fin";
int tracker_response_length;
struct TrackerResponse tracker_response = {0};

size_t write_func(void *ptr, size_t size, size_t nmemb, char **write_stream){
    *write_stream = calloc(nmemb, size);
    if (*write_stream == NULL) {
        fprintf(stderr, "calloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(*write_stream, ptr, size*nmemb);
    tracker_response_length = size*nmemb;
    
    bencode_t tracker_response_bencode;
   
    bencode_init(&tracker_response_bencode, *write_stream, tracker_response_length);   
  
    if(parse_tracker_response(&tracker_response_bencode, &tracker_response)) {
        printf("Parsing the tracker response failed\n");
        exit(1);
    }  
    return size*nmemb;
}

int parse_tracker_response(bencode_t* tracker_response_bencode, struct TrackerResponse *trackerresponse){
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
            char str[len + 1];
            str[len+1] = 0;
            memcpy(str, val, len);
            printf("Tracker Response with fail: %s\n", str);
            return 1;
        }
    }
}

//starts the peer manager
int start_peer_manager(Torrent *torrent){
    CURL *curl;
    CURLcode res;
 
    curl = curl_easy_init();
    if(curl) {
        char url[1000] = {0};

        char *info_hash_encoded = curl_easy_escape(curl, (const char*)torrent->info_hash, 20);
        char *peer_id_encoded = curl_easy_escape(curl, peer_id, 20);
        
        sprintf(url, "%s?info_hash=%s&peer_id=%s&port=6881&uploaded=0&downloaded=0&left=%ld&compact=1&event=started", 
            torrent->tracker_url, info_hash_encoded, peer_id_encoded, torrent->length);

        curl_easy_setopt(curl, CURLOPT_URL, url);

        char *response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

        /* always cleanup */ 
        curl_free(info_hash_encoded);
        curl_free(peer_id_encoded);
        free(response);
        curl_easy_cleanup(curl);
    }
}

