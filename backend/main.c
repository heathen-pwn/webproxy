#include <stdio.h>
#include "http/http_server.h"
#include "include/curl.h"
#include "http/http_fetch.h"


int main() {
    // Loading configurations
    #define PORT 80
    curl_global_init(CURL_GLOBAL_DEFAULT); // Initilaizing curl.. called once per program lifetime
    curl_easy_init();
    curl_multi_init();

    // Entry of application
    // ?
    // ?
    // ?

    // Starting the server 
    struct MHD_Daemon *daemon = start_http_server(
            MHD_USE_AUTO_INTERNAL_THREAD,
            PORT
    );
    if(daemon == NULL) {
        printf("Could not start server!\n");
        return 1;
    }

    printf("[microhttpdaemon] Listening on %d\n", PORT);

    char * content = fetch_website("http://localhost/?get=icanhazip.com");
    if(!content) {
        fprintf(stderr, "fetch_website failed\n");
    }

    printf("%s\n", content);

    getchar();
    
    // etc...

    curl_global_cleanup();
}