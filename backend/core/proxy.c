#include <microhttpd.h>
#include "curl.h"
#include "http_fetch.h"
#include "http_server.h"
#include "http_parse.h"
#include "session.h"

enum MHD_Result process_headers(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    printf("%s: %s; ", key, value);
    return MHD_YES;
}


// Headers of an HTTP request to the API with arguments (i.e: /proxy?q=icanhazip.com)
enum MHD_Result process_args(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if(!key) {
        return MHD_NO;
    }
    if(!strcmp(key, "q")) { // Proxy request to target website (value = URL)
        if(!value) {
            return MHD_NO;
        }
        Memory *buffer = fetch_website(value);
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
    printf("%s: %s; ", key, value);
    return MHD_YES;
    // MHD_YES keeps iterating
}


// Processing cookies as they come from each http request
enum MHD_Result process_cookies(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    printf("[cookie] %s: %s;\n", key, value);
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if(!strcmp(key, "SessionID")) {
        if(key) {
            update_session_tick(get_session(request_essentials->context->sessionsTable, key));
        }
    }
    return MHD_YES;
    // MHD_YES keeps iterating
}

/*
 * This function returns the FULL URL of an (online) resource
 */
char *redirect_resources(struct MHD_Connection *connection, const char *resource_url)
{
    // Session-based resource redirection (tba)
    


    // Stateless Resource Redirection
    const char *target = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Referer");
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