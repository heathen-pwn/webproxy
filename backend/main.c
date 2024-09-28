#include <stdio.h>
#include "http/http_server.h"
#include "include/curl.h"
#include "http/http_fetch.h"


int main() {
    // Loading configurations
    #define PORT 8080
    curl_global_init(CURL_GLOBAL_DEFAULT); // Initilaizing curl.. called once per program lifetime

    // Starting the API 
    struct MHD_Daemon *daemon = start_http_server(
            MHD_USE_AUTO_INTERNAL_THREAD,
            PORT
    );
    if(daemon == NULL) {
        printf("Could not start server!\n");
        return 1;
    }

    printf("[microhttpdaemon] Listening on %d\n", PORT);

    getchar();
    
    // Cleanup
    curl_global_cleanup();
}