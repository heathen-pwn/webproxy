#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "http_server.h"
#include "http_fetch.h"

// HTTP Response
enum MHD_Result  handle_request(void *cls, struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
    
    // Fetch
    // Getting the value of the string query parameter, q stands for the website URL to query through the proxy
    const char *query = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q");
    if (!query) {
        fprintf(stderr, "No URL to proxy; query has failed\n");
        return MHD_NO;
    }
    Memory *buffer = fetch_website(query);
    if(!buffer) {
        fprintf(stderr, "Failed to fetch website: %s\n", query);
        return MHD_NO;
    }
    //Responding
    struct MHD_Response *response = MHD_create_response_from_buffer(buffer->size, buffer->response, MHD_RESPMEM_MUST_FREE);
    if(!response) {
        fprintf(stderr, "MHD_create_response_from_buffer failed");
        free(buffer);
        return MHD_NO;
    }
    int res = MHD_queue_response(connection, MHD_HTTP_OK, response);
    if(res != MHD_YES) {
        fprintf(stderr, "MHD_queue response failed!");
    }
    free(buffer);
    return res;
}

// Listener
struct MHD_Daemon* start_http_server(unsigned int flags, uint16_t port) {
    struct MHD_Daemon *daemon = 
        MHD_start_daemon(flags, port, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);
    return daemon;
}



