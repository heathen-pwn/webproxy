#include <stdio.h>
#include "http/http_server.h"


int main() {
    // Loading configurations
    #define PORT 80


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
    getchar();
    
    // etc...
}