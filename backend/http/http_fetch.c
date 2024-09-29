#include <stdlib.h>
#include "microhttpd.h"
#include "http_fetch.h"
#include "http_parse.h"
#include "curl.h"
#include "http_server.h"


size_t write_webdata(char *response_content, size_t size /*size per chunk*/, size_t nmemb /*number of chunks in current transfer*/, void *webdata)
{
    if (!webdata)
    {
        fprintf(stderr, "Webdata points to nothing");
        return 0;
    }
    size_t current_chunk_size = size * nmemb;
    Memory *mem = (Memory *)webdata;
    if (!mem)
    {
        fprintf(stderr, "No place to dump the content at");
        return 0;
    }
    if (!response_content || current_chunk_size == 0)
    {
        fprintf(stderr, "Empty response");
        return 0;
    }
    // Adjusting size
    printf("Current size: %zu, Requested new size: %zu\n", mem->size, mem->size + current_chunk_size + 1);
    char *ptr = realloc(mem->response, mem->size + current_chunk_size + 1);
    if (!ptr)
    {
        fprintf(stderr, "Could not assign memory block");
        return 0;
    }
    printf("Current size: %zu, Requested new size: %zu\n", mem->size, mem->size + current_chunk_size + 1);
    // Assigning to new memory block (the new size)
    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), response_content, current_chunk_size);
    mem->size += current_chunk_size;
    mem->response[mem->size] = 0;
    return current_chunk_size;
}

Memory *fetch_website(const char *url)
{
    printf("[fetch_website]: %s\n", url);

    // Processing the GET command asynchronously:
    printf("Getting from the URL: %s\n", url);
    Memory *mem = malloc(sizeof(Memory));
    if (!mem)
    {
        fprintf(stderr, "Could not allocate memory for Memory structure. Failed!");
        return NULL;
    }
    mem->response = NULL;
    mem->size = 0;
    //
    int still_running;
    CURLM *multi = curl_multi_init();
    if (!multi)
    {
        fprintf(stderr, "curl_multi_init failed");
        return NULL;
    }
    CURL *op = curl_easy_init(); // an http operation
    if (!op)
    {
        fprintf(stderr, "curl_easy_init failed");
        return NULL;
    }

    // HTTP GET FROM BACKEND (this might usefully be put in a separate command later for header-specific considerations)
    curl_easy_setopt(op, CURLOPT_URL, url);
    curl_easy_setopt(op, CURLOPT_WRITEDATA, (void *)mem);
    curl_easy_setopt(op, CURLOPT_WRITEFUNCTION, write_webdata);
    curl_easy_setopt(op, CURLOPT_FOLLOWLOCATION, 1L);
    CURLMcode res = curl_multi_add_handle(multi, op);
    if (res != CURLM_OK)
    {
        fprintf(stderr, "curl_multi_add_handle failed; code: %d", res);
        return NULL;
    }
    // Execute
    do
    {
        res = curl_multi_perform(multi, &still_running);
        if (!res && still_running)
        { // no error
            res = curl_multi_poll(multi, NULL, 0, 1000, NULL);
            if (res)
            {
                fprintf(stderr, "curl_multi_poll failed, code %d", res);
                break;
            }
        }
    } while (still_running);

    curl_multi_cleanup(multi);
    curl_easy_cleanup(op);
    return mem;
    // Returned dynamic memory needs to be freed by caller
}

enum MHD_Result handle_get_request(void *cls, const char *url)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;

    int itersize = MHD_get_connection_values(request_essentials->connection, MHD_HEADER_KIND, process_headers, request_essentials);
    int argcount = MHD_get_connection_values(request_essentials->connection, MHD_GET_ARGUMENT_KIND, process_args, request_essentials);
    int cookies_count = MHD_get_connection_values(request_essentials->connection, MHD_COOKIE_KIND, process_cookies, request_essentials);

    printf("\n\n");

    printf("Iterants %d\nCookies %d\nArguments %d", itersize, cookies_count, argcount);

    // No response, meaning none of the previous commands had filled a response yet.. so let's look for more ways to generate a response:
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
            query = redirect_resources(request_essentials->connection, url);
            printf("REDIRECTING RESOURCES FROM %s\n", query);
            Memory *buffer = fetch_website(query);
            request_essentials->response = MHD_create_response_from_buffer(buffer->size, buffer->response, MHD_RESPMEM_MUST_FREE);
            // Response is being freed but not buffer
        }
    }
    return MHD_YES;
}