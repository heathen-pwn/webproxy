#include <microhttpd.h>
#include "curl.h"
#include "http_fetch.h"
#include "http_server.h"
#include "http_parse.h"
#include "session.h"
#include "http_session.h"
#include <stdlib.h>
#include <string.h>

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

        // Serve JS to protect client from IP leaks; this is causing fetch_webstie to crash when fetching proxy.js (altho request_essentials suppsoed to deinitialize per request)
        allocate_process(request_essentials);
        

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
                char *full_url_from_session = malloc(size);
                if (full_url_from_session) // change this to full_url and add the code below in it to continue code execution in case of failure of malloc
                {
                    snprintf(full_url_from_session, size, "%s%s", ses->session_url, resource_url);
                    printf("FULL URL of session %s thats proxying %s: %s\n", ses->session_id, ses->session_url, full_url_from_session);
                    return full_url_from_session; // caller needs to free this 

                } else {
                    fprintf(stderr, "Buffer allocation for full_url_from_session string failed! @redirect_resources; memory size requested %ld\n", size);
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
    char *full_url_from_referer = malloc(size);
    if (!full_url_from_referer)
    {
        fprintf(stderr, "Buffer allocation for full_url_from_referer string failed!");
        return NULL;
    }

    snprintf(full_url_from_referer, size, "%s%s", target, resource_url);

    return full_url_from_referer; // Caller needs to free full_url
}

// Protect client from leaking their IP to servers
void allocate_process(void *cls) {
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    const char *sec_fetch_dest = MHD_lookup_connection_value(request_essentials->connection, MHD_HEADER_KIND, "sec-fetch-dest");
    if(sec_fetch_dest && !strcmp(sec_fetch_dest, "document")) {
        serve_js(request_essentials);    
        return;
    } 
    // content-type its not here, it should be at libcurl fetch then give an indicator to serve_js
    // const char *content_type = MHD_lookup_connection_value(request_essentials->connection, MHD_HEADER_KIND, "content-type"); 
    // else if(content_type && !strcmp(content_type, "text/html")) {
    //     serve_js(request_essentials);
    //     return;
    // }    
}
void serve_js(void *cls) {
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if(!request_essentials) {
        fprintf(stderr, "No request context serve_js failed\n");
        return;
    }

    char *js_line = "<script type=\"text/javascript\" src=\"/proxy.js\"></script>";
    size_t js_line_len = strlen(js_line);
    if(!request_essentials->buffer) {
        fprintf(stderr, "No valid buffer @serve_js\n");
        return;
    }
    size_t haystack_len = request_essentials->buffer->size + js_line_len;
    char *haystack = realloc(request_essentials->buffer->response, haystack_len);
    if(!haystack) {
        printf("Reallocation failed @serve_js\n");
        return;
    }

    request_essentials->buffer->response = haystack;


    char *needle = "<head>";
    size_t needle_len = strlen(needle);
    // Find needle
    char *substr = memmem(haystack, haystack_len, needle, needle_len);
    if(!substr) {
        printf("memmem (find binary) failed @serve_js");
        return;
    }
    // Length of the substring
    size_t substr_len = (haystack/*beginning of buffer*/ + request_essentials->buffer->size/*the old buffer size*/)/*the full old buffer*/ - substr/* <head> and whats after it*/; 
    
    // Shift to the right to make space for inserted data, this is causing all the problems!
    memmove(substr + js_line_len, substr, substr_len);

    // Add js_line
    memmove(substr + needle_len, js_line, js_line_len);


    request_essentials->buffer->size += js_line_len;
}

// extern void *memmem (const void *__haystack, size_t __haystacklen,
// 		     const void *__needle, size_t __needlelen)
//      __THROW __attribute_pure__ __nonnull ((1, 3))
//     __attr_access ((__read_only__, 1, 2))
//     __attr_access ((__read_only__, 3, 4));