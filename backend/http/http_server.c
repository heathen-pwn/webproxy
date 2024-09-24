#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "http_server.h"

// HTTP Request handler
enum MHD_Result  handle_request(void *cls, struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
    // MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    const char *buffer = "Hello world!";
    size_t response_size = strlen(buffer);
    char *response_buffer = malloc(response_size + 1);
    strcpy(response_buffer, buffer);
    printf("%s; resp: %s", buffer, response_buffer);
    struct MHD_Response *response = MHD_create_response_from_buffer(response_size, response_buffer, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "text/plain");
    MHD_queue_response(connection, MHD_HTTP_OK, response);
    // printf("Request handle called");
    return MHD_YES;
}

// Start listener function
struct MHD_Daemon* start_http_server(unsigned int flags, uint16_t port) {
    struct MHD_Daemon *daemon = 
        MHD_start_daemon(flags, port, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);
    return daemon;
}
// Answering function
