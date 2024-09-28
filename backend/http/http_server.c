#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "http_server.h"
#include "http_fetch.h"
#include "proxy.h"

// Handling HTTP requests
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{

    // HTTP Headers

    int itersize = MHD_get_connection_values(connection, MHD_HEADER_KIND, process_headers, NULL);
    MHD_get_connection_values(connection, MHD_COOKIE_KIND, process_headers, NULL);
    MHD_get_connection_values(connection, MHD_POSTDATA_KIND, process_headers, NULL);
    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, process_headers, NULL);
    MHD_get_connection_values(connection, MHD_FOOTER_KIND, process_headers, NULL);
    printf("Iterants %d\n", itersize);
    // URI argument (?q=)
    const char *query = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q"); // This query contains the URL that should be fetched (vis-a-vis the proxy)
    if (!query)
    {
        fprintf(stderr, "No URL to proxy\nChecking if resources can be served:\n");
        // Check for static resource header here;
        query = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "x-serve-local");
        if (!query)
        {
            fprintf(stderr, "No resources to redirect\n");
            return MHD_NO;
        }
        if (!strcmp(query, "1"))
        {
            query = redirect_resources(connection, url); // returning the url to be fetched with fetch_website
            printf("REDIRECTING RESOURCES FROM %s\n", query);
        }
        if (!query)
            return MHD_NO; // still nothing in query... so return to prevent segmentation fault
    }
    // yes:
    Memory *buffer = fetch_website(query);
    if (!buffer)
    {
        fprintf(stderr, "Failed to fetch website: %s\n", query);
        return MHD_NO;
    }

    // HTTP RESPONSE
    struct MHD_Response *response = MHD_create_response_from_buffer(buffer->size, buffer->response, MHD_RESPMEM_MUST_FREE);
    if (!response)
    {
        fprintf(stderr, "MHD_create_response_from_buffer failed");
        free(buffer);
        return MHD_NO;
    }
    int res = MHD_queue_response(connection, MHD_HTTP_OK, response);
    if (res != MHD_YES)
    {
        fprintf(stderr, "MHD_queue response failed!");
    }
    free(buffer);
    // Need to free the resource URL returned by redirect_resources... but if i Just free(query) here there's a chance its the PROXY.. so need to separate them; MEMORY LEAK RN!
    printf("\n\n\n\n");
    return res;
}

// Listener
struct MHD_Daemon *start_http_server(unsigned int flags, uint16_t port)
{
    struct MHD_Daemon *daemon =
        MHD_start_daemon(flags, port, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);
    return daemon;
}
