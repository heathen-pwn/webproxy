// http_server.h
#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include "curl.h"
#include "Uri.h"

// struct MHD_Daemon* start_http_server(unsigned int flags, uint16_t port);
char *get_query(const char *url);
UriQueryListA *parse_query(const char *query);
const char* get_key_value(UriQueryListA *querylist, const char *key);

#endif // HTTP_PARSE_H