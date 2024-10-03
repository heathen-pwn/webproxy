#include <microhttpd.h>
#include "curl.h"
#include "http_fetch.h"
#include "http_server.h"
#include "http_parse.h"
#include "session.h"
#include "http_session.h"

enum MHD_Result process_headers(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    printf("%s: %s; ", key, value);
    return MHD_YES;
}

// Headers of an HTTP request to the API with arguments (i.e: /proxy?q=icanhazip.com)
enum MHD_Result process_args(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if (!key)
    {
        return MHD_NO;
    }
    if (!strcmp(key, "q")) // Turn this into a function proxy_target();
    { // Proxy request to target website (value = URL)
        if (!value)
        {
            return MHD_NO;
        }
        Memory *buffer = fetch_website(value);
        request_essentials->request_full_url = value;
        request_essentials->buffer = buffer;
        if (!buffer)
        {
            fprintf(stderr, "Failed to fetch website: %s\n", value);
            return MHD_NO;
        }
        request_essentials->response = MHD_create_response_from_buffer(buffer->size, buffer->response, MHD_RESPMEM_MUST_FREE);
        if (!request_essentials->response)
        {
            fprintf(stderr, "MHD_create_response_from_buffer failed");
            free(buffer);
            return MHD_NO;
        }
        // Buffer needs to be freed in success as well!! by caller
    }
    return MHD_YES;
    // MHD_YES keeps iterating
}

// Processing cookies as they come from each http request
enum MHD_Result process_cookies(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    // printf("[cookie] %s: %s;\n", key, value);
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if (!strcmp(key, "SessionID"))
    {
        if (key)
        {
            printf("Connection Tick update @process_cookies!\n");
            update_session_tick(get_session(request_essentials->context->sessionsTable, value));
        }
    }
    return MHD_YES;
    // MHD_YES keeps iterating
}

/*
 * This function returns the FULL URL of an (online) resource
 */
char *redirect_resources(void *cls, const char *resource_url)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    /*
    *    Session-based resource redirection:
    */

    if(!request_essentials->context->sessionsTable) {
        printf("There is no table.. @redirect_resources\n");
        return NULL;
    }
    const char *client_session = MHD_lookup_connection_value(request_essentials->connection, MHD_COOKIE_KIND, "SessionID");
    printf("client_session %s @redirect_resources\n", client_session);
    if (client_session) // If the request has a session cookie defined
    {
        Session *ses = get_session(request_essentials->context->sessionsTable, client_session);
        if(ses) { // The request has a valid session cookie
            if (ses->session_url) // If a target website is defined in the session
            {
                size_t size = snprintf(NULL, 0, "%s%s", ses->session_url, resource_url) + 1; // +1 null terminator
                char *full_url = malloc(size);
                if (!full_url) // change this to full_url and add the code below in it to continue code execution in case of failure of malloc
                {

                    snprintf(full_url, size, "%s%s", ses->session_url, resource_url);
                    printf("FULL URL of session %s thats proxying %s: %s\n", ses->session_id, ses->session_url, full_url);
                    return full_url; // caller needs to free this 

                } else {
                    fprintf(stderr, "Buffer allocation for full_url string failed! @redirect_resources\n");
                }

            } else {
                fprintf(stderr, "ses->target is null\n");
            }
        } else {
            fprintf(stderr, "get_session failed in redirect_resources\n");
        }

    }

    /*
    *   Stateless Resource Redirection
    */ 
    const char *target = MHD_lookup_connection_value(request_essentials->connection, MHD_HEADER_KIND, "Referer");
    UriQueryListA *keyvalue = parse_query(target);
    target = get_key_value(keyvalue, "q");

    size_t size = snprintf(NULL, 0, "%s%s", target, resource_url) + 1; // +1 null terminator
    char *full_url = malloc(size);
    if (!full_url)
    {
        fprintf(stderr, "Buffer allocation for full_url string failed!");
        return NULL;
    }

    snprintf(full_url, size, "%s%s", target, resource_url);

    return full_url; // Caller needs to free full_url
}