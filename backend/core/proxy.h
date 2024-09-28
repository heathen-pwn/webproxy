// http_server.h
#ifndef CORE_PROXY_H
#define CORE_PROXY_H

#include <microhttpd.h>
#include "curl.h"
#include "http_server.h"

enum MHD_Result process_headers(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
char *redirect_resources(struct MHD_Connection *connection, const char *resource_url);

#endif // CORE_PROXY_H
