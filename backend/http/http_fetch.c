#include <stdlib.h>
#include "microhttpd.h"
#include "curl.h"
#include "http_parse.h"


size_t write_webdata(char *response_content, size_t size /*size per chunk*/, size_t nmemb /*number of chunks*/, char **webdata) {
    size_t total_size = size * nmemb;
    if(!webdata || !response_content || total_size == 0) {
        fprintf(stderr, "No place to dump the content at or empty response");
        return 0;
    }
    // Resize the memory to the appropriate size received from the website
    size_t curchunk_length = *webdata ? strlen(*webdata) : 0;
    char* resized_data = realloc(*webdata, curchunk_length + total_size + 1); 

    if(!resized_data) {
        fprintf(stderr, "Could not assign memory block");
        return 0;
    }

    // memcpy(resized_data, response_content, total_size); 
    
    // Fill in the block
    memcpy(*webdata + curchunk_length, response_content, total_size);
    *webdata = resized_data;
    printf("The retrieved data in this chunk is: %s\n", response_content);
    // Return size
    return total_size;

}
char * fetch_website(const char *url) {
    // Extracting URL
    UriQueryListA *list = parse_query(url);
    if(!list) {
        printf("Provided url could not be parsed into a list @fetch_website\n");
        return NULL;
    }
    const char *website = get_key_value(list, "get");
    if(!website) {
        fprintf(stderr, "Key could not be retrieved");
        return NULL;
    }

    // Processing the GET command asynchronously:
    printf("Getting from the URL: %s\n", website);
    char *webdata = malloc(1);
    if(!webdata) {
        fprintf(stderr, "Failed; Could not allocate memory for webdata");
        return NULL;
    }
    int still_running;
    CURLM *multi = curl_multi_init();
    if(!multi) {
        fprintf(stderr, "curl_multi_init failed");
        free(webdata);
        return NULL;
    }
    CURL *op = curl_easy_init(); // an http operation
    if(!op) {
        fprintf(stderr, "curl_easy_init failed");
        free(webdata);
        return NULL;
    }
    
    //Settings
    curl_easy_setopt(op, CURLOPT_URL, website);
    curl_easy_setopt(op, CURLOPT_WRITEDATA, &webdata);
    curl_easy_setopt(op, CURLOPT_WRITEFUNCTION, write_webdata);
    CURLMcode res = curl_multi_add_handle(multi, op);
    if(res != CURLM_OK) {
        fprintf(stderr, "curl_multi_add_handle failed; code: %d", res);
        free(webdata);
        return NULL;
    }
    // Execute
    do {
        res = curl_multi_perform(multi,  &still_running);
        if(!res && still_running) { // no error 
            res = curl_multi_poll(multi, NULL, 0, 1000, NULL);
            if(res) {
                fprintf(stderr, "curl_multi_poll failed, code %d", res);
                break; 
            }
        }
    }
    while(still_running);

    return webdata;
    // free(webdata); needs to be done by caller after using it

}

