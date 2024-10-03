#include <stdio.h>
#include <curl.h>
#include <pthread.h>
#include <json-c/json.h>
#include "main.h"
#include "http_fetch.h"
#include "session.h"

pthread_mutex_t session_mutex;
pthread_cond_t cond_collect_garbage;

int main() {

    // Loading configurations (app.conf tbd)

    #define PORT 8080
    curl_global_init(CURL_GLOBAL_DEFAULT); // Initilaizing curl.. called once per program lifetime
    int table_size = 32;
    
    pthread_mutex_init(&session_mutex, NULL);
    pthread_cond_init(&cond_collect_garbage, NULL);

    // Set up sessions table

    App *context = malloc(sizeof(App));
    if(!context) {
        fprintf(stderr, "Could not allocate memory for application context! Failed");
        return -1; 
    }

    context->sessionsTable = create_sessions_table(table_size);
    context->sessions_threshold = 1;
    context->sessions_timeout = 5; // in seconds
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

    // Threading
    pthread_t garbage_collector; // Garbage collector 
    int res = pthread_create(&garbage_collector, NULL, &collect_session_garbage, (void *)context);
    if(res != 0) {
        fprintf(stderr, "Could not create garbage collector thread!");
    }



    getchar();
    
    // Cleanup
    free_sessions_table(context->sessionsTable);
    curl_global_cleanup();
}

