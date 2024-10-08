// http_server.h
#ifndef HTTP_FETCH_H
#define HTTP_FETCH_H

#include "microhttpd.h"
#include "curl.h"

typedef struct {
    char *response;
    size_t size;
} Memory;


// struct MHD_Daemon* start_http_server(unsigned int flags, uint16_t port);
size_t write_webdata(char *ptr, size_t size, size_t nmemb, void *web_data);
Memory * fetch_website(const char *url);

#endif // HTTP_FETCH_H
