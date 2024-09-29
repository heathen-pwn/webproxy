#include "http_server.h"


// Handling HTTP requests
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{

    const App *context = (App *)cls; 
    
    RequestEssentials request_essentials = {0};
    request_essentials.context = context;

    if(context->sessionsTable) {
        printf("There is a sessions table loaded\n");
    } else {
        fprintf(stderr, "No sessions table!\n");
    }
    // HTTP Headers

    int itersize = MHD_get_connection_values(connection, MHD_HEADER_KIND, process_headers,  &request_essentials);
    MHD_get_connection_values(connection, MHD_POSTDATA_KIND, process_headers, NULL);
    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, process_headers, NULL);
    MHD_get_connection_values(connection, MHD_FOOTER_KIND, process_headers, NULL);
    MHD_get_connection_values(connection, MHD_COOKIE_KIND, process_cookies, &request_essentials);
    printf("\n");
    

    //
    printf("Iterants %d\n", itersize);
    // URI argument (?q=)
    const char *query = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q"); // This query contains the URL that should be fetched (vis-a-vis the proxy)
    if(!query)
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
            // MEMORY LEAK HERE! redirect_resources buffer not being freed after use
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

    // Building response..
    struct MHD_Response *response = MHD_create_response_from_buffer(buffer->size, buffer->response, MHD_RESPMEM_MUST_FREE);
    if (!response)
    {
        fprintf(stderr, "MHD_create_response_from_buffer failed");
        free(buffer);
        return MHD_NO;
    }
    
    // Session management
    const char *client_session = MHD_lookup_connection_value(connection, MHD_COOKIE_KIND, "SessionID");
    // There is no session in the HTTP request, so set one up
    if(!client_session) { 

        // Creating session
        Session *ses = create_session(); // MUST BE FREED when session ends
        if(!ses) fprintf(stderr, "create_session failed @ session management");
        insert_session(context->sessionsTable, ses); // seg fault
        printf("SessionID: %s\nSessionKey: %s\n", ses->session_id, ses->session_key);

        // Setting up session cookie
        Cookie cookie = {0};
        cookie.key = "SessionID";
        cookie.value = ses->session_id;
        add_cookie(response, &cookie);

    }

    // Queuing response...
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
struct MHD_Daemon *start_http_server(App *context, unsigned int flags, uint16_t port)
{
    struct MHD_Daemon *daemon =
        MHD_start_daemon(flags, port, NULL, NULL, &handle_request, (void *)context, MHD_OPTION_END);
    return daemon;
}
