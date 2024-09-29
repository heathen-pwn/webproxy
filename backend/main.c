#include "main.h"

int main() {
    // Loading configurations
    #define PORT 8080
    curl_global_init(CURL_GLOBAL_DEFAULT); // Initilaizing curl.. called once per program lifetime

    // Entry
    App *context = malloc(sizeof(App));
    if(!context) {
        fprintf(stderr, "Could not allocate memory for application context! Failed");
        return -1; 
    }
    context->sessionsTable = create_sessions_table();
    if(!context->sessionsTable) {
        fprintf(stderr, "Could not create sessions table! Failed");
        return -1;
    }
    // Starting the API 
    struct MHD_Daemon *daemon = start_http_server(
            context,
            MHD_USE_AUTO_INTERNAL_THREAD,
            PORT
    );
    if(daemon == NULL) {
        printf("Could not start server!\n");
        return 1;
    }
    
    printf("[microhttpdaemon] Listening on %d\n", PORT);

    // Set up sessions table


    

    getchar();
    
    // Cleanup
    free_sessions_table(context->sessionsTable);
    curl_global_cleanup();
}