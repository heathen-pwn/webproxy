// http_server.h
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "../main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "http_fetch.h"
#include "proxy.h"
#include "http_cookies.h"
#include "http_session.h"

typedef struct {
    const App *context;  // Read only
    struct MHD_Connection *connection; 
    struct MHD_Response *response; 
    
} RequestEssentials;

// struct MHD_Daemon* start_http_server(unsigned int flags, uint16_t port);
struct MHD_Daemon* start_http_server(App *context, unsigned int flags, uint16_t port);
enum MHD_Result  handle_request(void *cls, struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) ;

#endif // HTTP_SERVER_H
