#include "http_server.h"

// Handling HTTP requests
// There are many memory leaks here from the buffers inside the functions
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    printf("-----------------[HTTP: %s]-----------------\n", method);
    const App *context = (App *)cls;

    if(!context) {
        fprintf(stderr, "There is no application context");
        return MHD_NO;
    }

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
    manage_session(&request_essentials);
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

    // Freeing full url buffer
    if(request_essentials.buffer) {
        free(request_essentials.buffer);
        request_essentials.buffer = NULL;
    } 
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

enum MHD_Result handle_get_request(void *cls, const char *url)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;

    int itersize = MHD_get_connection_values(request_essentials->connection, MHD_HEADER_KIND, process_headers, request_essentials);
    printf("\n");
    //process_proxy:
    int argcount = MHD_get_connection_values(request_essentials->connection, MHD_GET_ARGUMENT_KIND, process_args, request_essentials);
    printf("\n");
    int cookies_count = MHD_get_connection_values(request_essentials->connection, MHD_COOKIE_KIND, process_cookies, request_essentials);


    printf("Request metadata: Number of Headers %d; Number of Cookies %d; Number of Arguments in URL %d\n", itersize, cookies_count, argcount);

    // No response, meaning none of the previous functions had filled a response yet.. so let's look for more ways to generate a response:
    if (!request_essentials->response)
    {
        handle_resource_redirection(request_essentials, url);
    }
    return MHD_YES;
}

enum MHD_Result handle_resource_redirection(void *cls, const char *url)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    const char *query = MHD_lookup_connection_value(request_essentials->connection, MHD_HEADER_KIND, "x-serve-local");
    if (query /*REDIRECTING RESOURCES*/)
    {
        if (!strcmp(query, "1")) // serve the resources
        {
            char *full_url = redirect_resources(request_essentials, url);
            printf("REDIRECTING RESOURCES FROM %s\n", query);
            
            Memory *buffer = fetch_website(full_url);

            request_essentials->buffer = buffer;
            request_essentials->response = MHD_create_response_from_buffer(buffer->size, buffer->response, MHD_RESPMEM_MUST_FREE);

            free(full_url);
            //  Memory *buffer is freed when queuing HTTP request
        }
    }
    return MHD_YES;
}