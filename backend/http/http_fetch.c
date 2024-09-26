#include <stdlib.h>
#include "microhttpd.h"
#include "curl.h"
#include "http_parse.h"


size_t write_webdata(char *ptr, size_t size, size_t nmemb, void *web_data) {

}
char * fetch_website(const char *url) {
    // Extracting URL
    char *query = NULL; // Points to NULL, initializing pointer, holds a pointer to the extracted string
    query = get_query(url);
    if(query != NULL)
        printf("Query: %s", query);
    else printf("Query returned NULL");
    
    // Processing the GET command asynchronously:
    void *web_data = malloc(1);
    int still_running;
    CURLM *multi = curl_multi_init();
    CURL *op = curl_easy_init(); // an http operation

    curl_easy_setopt(op, CURLOPT_URL, query);
    curl_easy_setopt(op, CURLOPT_WRITEDATA, &web_data);
    curl_easy_setopt(op, CURLOPT_WRITEFUNCTION, write_webdata);
    curl_multi_add_handle(multi, op);
    CURLMcode res = curl_multi_perform(multi,  &still_running);
    return web_data;
    // free(web_data); needs to be done by caller after using it

}

