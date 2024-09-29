#include <microhttpd.h>
#include "curl.h"
#include "http_server.h"
#include "http_parse.h"
#include "session.h"


// Headers of an HTTP request to the API
enum MHD_Result process_headers(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    printf("%s: %s; ", key, value);
    return MHD_YES;
    // MHD_YES keeps iterating
}


// Processing cookies as they come from each http request
enum MHD_Result process_cookies(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    printf("[cookie] %s: %s; ", key, value);
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if(!strcmp(key, "SessionID")) {
        update_session_tick(get_session(request_essentials->context->sessionsTable, key));
    }
    return MHD_YES;
    // MHD_YES keeps iterating
}

/*
 * This function receives a connection and the url that is sent towards the proxy server
 * What it needs to do is to return the real URL where the resource comes from so the proxy server can fetch it
 * Caller needs to free full_url
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