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
// Add implementation to account for the change of the target URL in current session
enum MHD_Result process_args(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if (!key)
    {
        return MHD_NO;
    }
    if (!strcmp(key, "q")) // Turn this into a function proxy_target();
    {                      // Proxy request to target website (value = URL)
        if (!value)
        {
            return MHD_NO;
        }
        Memory *buffer = fetch_website(value);
        request_essentials->request_full_url = value;
        if (!buffer)
        {
            fprintf(stderr, "Failed to fetch website: %s\n", value);
            return MHD_NO;
        }
        request_essentials->buffer = buffer;
        
        // Register redirection if session exists
        const char *client_session = MHD_lookup_connection_value(request_essentials->connection, MHD_COOKIE_KIND, "SessionID");
        Session *ses = is_valid_session(request_essentials, client_session);
        if(ses) {
            // Extract authoritative domain
            StringExtract *extracted_url = find_domain(value);
            //
            size_t url_len = extracted_url->size + 1; // null terminator
            char *session_url = malloc(url_len);
            if(session_url) {
                if(ses->session_url != NULL) { // If there is an existing session URL, free it first
                    free(ses->session_url);
                }

                // --
                memcpy(session_url, extracted_url->start, extracted_url->size); 
                // Parse the URL to be just the domainname.com
                ses->session_url = session_url;
                ses->session_url_size = extracted_url->size;
                printf("Changed session URL to %s\n", session_url);
            }
            
        }
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

    if (!request_essentials->context->sessionsTable)
    {
        printf("There is no table.. @redirect_resources\n");
        return NULL;
    }
    const char *client_session = MHD_lookup_connection_value(request_essentials->connection, MHD_COOKIE_KIND, "SessionID");
    printf("client_session %s @redirect_resources\n", client_session);
    if (client_session) // If the request has a session cookie defined
    {
        Session *ses = get_session(request_essentials->context->sessionsTable, client_session);
        if (ses)
        {                         // The request has a valid session cookie
            if (ses->session_url) // If a target website is defined in the session
            {
                size_t size = snprintf(NULL, 0, "%s%s", ses->session_url, resource_url) + 1; // +1 null terminator
                char *full_url_from_session = malloc(size);
                if (full_url_from_session) // change this to full_url and add the code below in it to continue code execution in case of failure of malloc
                {
                    snprintf(full_url_from_session, size, "%s%s", ses->session_url, resource_url);
                    printf("FULL URL of session %s thats proxying %s: %s\n", ses->session_id, ses->session_url, full_url_from_session);
                    return full_url_from_session; // caller needs to free this
                }
                else
                {
                    fprintf(stderr, "Buffer allocation for full_url_from_session string failed! @redirect_resources; memory size requested %ld\n", size);
                }
            }
            else
            {
                fprintf(stderr, "ses->target is null\n");
            }
        }
        else
        {
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
void allocate_process(void *cls)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    const char *sec_fetch_dest = MHD_lookup_connection_value(request_essentials->connection, MHD_HEADER_KIND, "sec-fetch-dest");
    if (sec_fetch_dest && !strcmp(sec_fetch_dest, "document"))
    {
        serve_js(request_essentials);
        // return;
    }

    // Sanitize 'request_essentials->buffer->response' to prevent compromising redirectives
    /*
     * <meta http-equiv="refresh" content="5; URL='https://example.com/'" />
     * window.location.href = "https://example.com";
     * window.location.replace("https://example.com");
     * window.location.assign("https://example.com");
     * */
    sanitize_response((void *)request_essentials);
}
void serve_js(void *cls)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if (!request_essentials)
    {
        fprintf(stderr, "No request context serve_js failed\n");
        return;
    }

    char *js_line = "<script type=\"text/javascript\" src=\"/proxy.js\"></script>";
    size_t js_line_len = strlen(js_line);
    if (!request_essentials->buffer)
    {
        fprintf(stderr, "No valid buffer @serve_js\n");
        return;
    }
    size_t haystack_len = request_essentials->buffer->size + js_line_len;
    char *haystack = realloc(request_essentials->buffer->response, haystack_len);
    if (!haystack)
    {
        printf("Reallocation failed @serve_js\n");
        return;
    }

    request_essentials->buffer->response = haystack;

    char *needle = "<head>";
    size_t needle_len = strlen(needle);
    // Find needle
    char *substr = memmem(haystack, haystack_len, needle, needle_len);
    if (!substr)
    {
        printf("memmem (find binary) failed @serve_js");
        return;
    }
    // Length of the substring
    size_t substr_len = (haystack /*beginning of buffer*/ + request_essentials->buffer->size /*the old buffer size*/) /*the full old buffer*/ - substr /* <head> and whats after it*/;

    // Shift content; THIS IS CORRUPTING THE BOUNDS/METADATA --> problem
    memmove(substr + js_line_len, substr, substr_len);

    // Add js_line
    memcpy(substr + needle_len, js_line, js_line_len);

    request_essentials->buffer->size += js_line_len;
}
void sanitize_response(void *cls)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    if (!request_essentials)
        return;
    if (!request_essentials->buffer)
        return;
    if (!request_essentials->buffer->response)
        return;

    const char *gospel = "/?proxy=";
    const size_t gospel_len = strlen(gospel);

    size_t temp_haystack_len = request_essentials->buffer->size;
    size_t *haystack_len = &(temp_haystack_len);
    char *haystack = malloc(*haystack_len + gospel_len);

    if (!haystack)
        return;
    // Make it all lower case for case insensitive lookups
    memcpy(haystack, request_essentials->buffer->response, request_essentials->buffer->size);
    for (int i = 0; i < *haystack_len; i++)
        haystack[i] = tolower((unsigned char)haystack[i]);

    // SANITIZE META TAGS
    char *refresh_needle = "<meta http-equiv=\"refresh\"";
    char *op_meta_start = strstr(haystack, refresh_needle);
    if (op_meta_start) // if the http-equiv exists, remember http-equiv can also be an HTTP header, thats why its a metatag
    {
        char *op_meta_end = strchr(op_meta_start, '>'); // pre addition
        char *url_directive = strstr(op_meta_start, "url");
        if (url_directive)
        {

            char *url_end = strchr(url_directive, '"');
            if (url_end > op_meta_end)
            {
                url_end = strchr(url_directive, '\'');
            }

            // now the beginning of the url is the first find after equals thats not a white space
            char *equal = strchr(url_directive, '=');
            if (!equal)
            { // invalid meta tag syntax
                free(haystack);
                return;
            }
            char *url_start = equal + 1;
            while (*url_start == ' ' || *url_start == '\t')
            {
                url_start++;
            }
            if (url_start < url_end) // Making sure its the URL that we're concerned with
            {
                // Inject in case-insensitive temp buffer
                char *inject_res = inject_data(gospel, gospel_len, haystack, haystack_len, url_start);
                printf("Injected: %s\n", inject_res);

                // Update new memory locations for later use
                char *op_meta_start = strstr(haystack, refresh_needle);
                char *op_meta_end = strchr(op_meta_start, '>');

                // Create the string of the full metatag <meta..\>
                size_t injection_size = (op_meta_end + 1 /*1 byte after where the end is*/) - op_meta_start;
                char *injection = malloc(injection_size);
                if (!injection)
                    return;
                memcpy(injection, op_meta_start, injection_size); // memcpy is not getting the '>' in the end

                // Get the beginning of the main response buffer's meta start and end
                char *buf_meta_start = strstr(request_essentials->buffer->response, refresh_needle);
                if (!buf_meta_start)
                    return;
                char *buf_meta_end = strchr(buf_meta_start, '>');
                if (!buf_meta_end)
                    return;

                // Replace old meta tag with sanitized meta tag
                erase_data(request_essentials->buffer->response, &(request_essentials->buffer->size), buf_meta_start, buf_meta_end);
                char *injection_pos = strstr(request_essentials->buffer->response, "<head>");
                if (injection_pos)
                {
                    injection_pos += 6;
                }
                inject_data(injection, injection_size, request_essentials->buffer->response, &(request_essentials->buffer->size), injection_pos);

                // Free the injection segment
                free(injection);
            }
        }
    }
    // Free the temporary case insensitive buffer
   free(haystack);
    /* SANITIZE JAVASCRIPT TAGS: */

    // document.location
    StringExtract *tag_val = NULL;
    tag_val = find_directive(request_essentials->buffer->response, ".location", "=", NULL);
    if(tag_val) {
        inject_data(gospel, gospel_len, request_essentials->buffer->response, &(request_essentials->buffer->size), tag_val->start);
    }
    // window.location.href window.location.hash window.location.pathname window.location.search
    tag_val = find_directive(request_essentials->buffer->response, "location.", "=", NULL);
    if(tag_val) {
        inject_data(gospel, gospel_len, request_essentials->buffer->response, &(request_essentials->buffer->size), tag_val->start);
    }


 
}
// https://canonical.com/blog/canonical-offers-12-year-lts-for-any-open-source-docker-imageQ/static/js/dist/main.js.map
StringExtract *find_domain(const char *url) {
    // Look for a dash (i.e., http:// or https://)
    char *begin = strstr(url, "://");
    char *end = NULL;
    if(!begin) { // no protocol mentioned
        // Maybe its a www.canonical.com
        begin = strstr(url, "www."); 
        if(!begin) { // There is no protocol and no www. so assume first byte is the authoritative domain
            begin = (char *)url; // warning: assignment discards 'const' qualifier from pointer target type
        }
    } else {
        begin += 3; // skip the ://
    }
    end = strchr(begin, '/'); // look for the first slash, i.e., canonical.com/ 
    if(!end) { // no slash, the end is the end.
        end = begin + strlen(url);
    }
    if(begin < end) {  // Make sure the content we have makes sense
        StringExtract *directive_info = malloc(sizeof(StringExtract));  // must be freed
        memset(directive_info, 0, sizeof(StringExtract));
        directive_info->start = begin;
        directive_info->end = end;
        directive_info->size = end - begin;
        return directive_info;
    }
    return NULL;
}
// Separate buffer/string logic in another core file
StringExtract *find_directive(const char *haystack, const char *directive, const char *symbol, const char *end_symbol)
{ // Returns structure for beginning and end
    char *url_directive = strstr(haystack, directive);
    if (!url_directive)
    {
        return NULL;
    }

    // Locate the start of the keyword by finding the directive's starting symbol
    size_t symlen = strlen(symbol);
    char *equal = NULL;
    if (!symlen)
    {
        return NULL;
    }
    if (symlen == 1)
    {
        equal = strchr(url_directive, symbol[0]);
    }
    else if (symlen > 1)
    {
        equal = strstr(url_directive, symbol);
    }

    if (equal == NULL)
    {
        return NULL; // Invalid syntax, no '=' found
    }

    // Locate the beginning of the value to be assigned to the directive (i.e., URL, etc.)
    char *url_start = equal + 1;
    // Skip white spaces and symbols
    while (*url_start == ' ' || *url_start == '\t' || tolower(*url_start) < 'a' || tolower(*url_start) > 'z')
    {
        url_start++;
    }

    // Locate the end of the value  by finding the first double or single quote
    char *url_end = NULL;
    if(end_symbol != NULL) { 
        size_t end_symlen = strlen(end_symbol);
        if(!end_symlen) {
            return NULL;
        }
        if(end_symlen == 1) {
            url_end = strchr(url_start, end_symbol[0]);
        } else if(end_symlen > 1) {
            url_end = strstr(url_start, end_symbol);
        }
    } else { // By default ending symbol is double quotations
        url_end = strchr(url_start, '"');
    }
    if (!url_end || url_end > strchr(url_start, '\0')) // look for single quotation
    {
        url_end = strchr(url_start, '\'');
    }
    // Couldn't locate the end of the value, at least locate the end of the current HTML line
    if (!url_end)
    {
        url_end = strchr(url_start, '>');
    }
    if (!url_end)
    {
        url_end = strchr(url_start, '>');
    }
    if (!url_end)
    { // give up
        return NULL;
    }

    // Ensure URL starts before the ending quote
    if (url_start < url_end)
    {
        StringExtract *directive_info = malloc(sizeof(StringExtract)); 
        memset(directive_info, 0, sizeof(StringExtract));
        directive_info->start = url_start;
        directive_info->end = url_end;
        directive_info->size = url_end - url_start;
        return directive_info; // must be freed
    }

    return NULL; // URL not found within expected bounds
}

char *inject_data(const char *injection, const size_t injection_size, char *buffer, size_t *buffer_size, char *injection_pos)
{
    if (!buffer || injection_size < 1 || !injection_pos || *buffer_size < 1)
    {
        fprintf(stderr, "Invalid parameters @inject_data");
        return NULL;
    }

    size_t injection_offset = injection_pos - buffer;
    size_t shifted_buffer_size = *buffer_size - injection_offset;

    // Resize content to fit addition
    size_t buf_new_size = *buffer_size + injection_size;
    char *temp_buffer = realloc(buffer, buf_new_size);
    if (!temp_buffer)
    {
        fprintf(stderr, "Failed to reallocate memory for temp_buffer @inject_data.");
        return NULL;
    }

    // Update injection pos
    injection_pos = temp_buffer + injection_offset;

    // Shift content to the right
    // void *memmove(void *dest_str, const void *src_str, size_t numBytes)
    char *shift_ptr = memmove(injection_pos + injection_size, injection_pos, shifted_buffer_size);
    if (!shift_ptr)
    {
        fprintf(stderr, "Shifting failed @inject_data");
        return NULL;
    }

    // add injection
    // void *memcpy(void *dest_str, const void * src_str, size_t n)
    char *add_ptr = memcpy(injection_pos, injection, injection_size);
    if (!add_ptr)
    {
        fprintf(stderr, "Injection failed");
        return NULL;
    }

    // Update buffer to the new allocated memory and buffer size
    buffer = temp_buffer;
    *buffer_size = buf_new_size;

    // Return pointer to injected data
    return buffer;
}

char *erase_data(char *buffer, size_t *buffer_size, char *start_pos, char *end_pos)
{
    if (!buffer || !start_pos || !end_pos || *buffer_size < 1 || start_pos > end_pos)
    {
        fprintf(stderr, "Invalid parameters @erase_data");
        return NULL;
    }

    // Calculate the size of the segment to be removed
    size_t erase_size = end_pos - start_pos;

    // Ensure erase_size is within buffer bounds
    if (erase_size >= *buffer_size || start_pos < buffer || end_pos > buffer + *buffer_size)
    {
        fprintf(stderr, "Invalid range for erase @erase_data");
        return NULL;
    }

    // Calculate the size of the data after the end position
    size_t remaining_size = *buffer_size - (end_pos - buffer);

    // Shift remaining data to the left to fill the gap
    char *shift_ptr = memmove(start_pos, end_pos, remaining_size);
    if (!shift_ptr)
    {
        fprintf(stderr, "Shifting failed @erase_data");
        return NULL;
    }

    // Resize buffer to remove the erased segment
    size_t new_buffer_size = *buffer_size - erase_size;
    char *temp_buffer = realloc(buffer, new_buffer_size);
    if (!temp_buffer && new_buffer_size > 0)
    {
        fprintf(stderr, "Failed to reallocate memory for temp_buffer @erase_data.");
        return NULL;
    }

    // Update buffer pointer and buffer size
    buffer = temp_buffer;
    *buffer_size = new_buffer_size;

    return buffer;
}
