#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "http_server.h"
#include "http_fetch.h"


// HTTP Response
enum MHD_Result  handle_request(void *cls, struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
    // MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    
    // The response is obviously not hello world, but should fetch the website that the user requested... so now I need a function that fetches a website and then sends it back to the user
    
    // Fetch
    fetch_website(url);

    // (testing)
    const char *buffer = "Hello world!";
    size_t response_size = strlen(buffer);
    char *response_buffer = malloc(response_size + 1);
    strcpy(response_buffer, buffer);
    // ------

    //Responding
    struct MHD_Response *response = MHD_create_response_from_buffer(response_size, response_buffer, MHD_RESPMEM_MUST_FREE);
    int res = MHD_queue_response(connection, MHD_HTTP_OK, response);
    return res;
}

// Listener
struct MHD_Daemon* start_http_server(unsigned int flags, uint16_t port) {
    struct MHD_Daemon *daemon = 
        MHD_start_daemon(flags, port, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);
    return daemon;
}



