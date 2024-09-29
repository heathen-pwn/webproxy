#include "http_server.h"

// Handling HTTP requests
// There are many memory leaks here from the buffers inside the functions
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    printf("-----------------[HTTP: %s]-----------------\n", method);
    const App *context = (App *)cls;

    RequestEssentials request_essentials = {0};
    request_essentials.context = context;
    request_essentials.connection = connection;
    request_essentials.response = NULL;

    if (context->sessionsTable)
    {
        printf("There is a sessions table loaded\n");
    }
    else
    {
        fprintf(stderr, "No sessions table!\n");
    }

    if (!strcmp(method, "GET")) 
    { // HTTP GET
        handle_get_request(&request_essentials, url);
    }

    // Session management
    handle_session_management(&request_essentials);
    // Queuing response...

    if (request_essentials.response)
    {
        int res = MHD_queue_response(connection, MHD_HTTP_OK, request_essentials.response);
        if (res != MHD_YES)
        {
            fprintf(stderr, "MHD_queue response failed!\n");
            return MHD_NO;
        }
    }
    else
    { 
        fprintf(stderr, "No response.. ending request\n");
        return MHD_NO;
    }
    // Buffers are not being freed (redirect resources and process_args)
    // Need to free the resource URL returned by redirect_resources... but if i Just free(query) here there's a chance its the PROXY.. so need to separate them; MEMORY LEAK RN!
    printf("-----------------[HTTP: REQUEST END]-----------------");
    printf("\n\n\n\n");
    return MHD_YES;
}

// Listener
struct MHD_Daemon *start_http_server(App *context, unsigned int flags, uint16_t port)
{
    struct MHD_Daemon *daemon =
        MHD_start_daemon(flags, port, NULL, NULL, &handle_request, (void *)context, MHD_OPTION_END);
    return daemon;
}
