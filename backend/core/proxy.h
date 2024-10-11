// http_server.h
#ifndef CORE_PROXY_H
#define CORE_PROXY_H

#include <microhttpd.h>
#include "curl.h"
#include "http_server.h"

typedef struct {
    char *start;
    char *end;
    size_t size;
} StringExtract;

enum MHD_Result process_headers(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
enum MHD_Result process_args(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
char *redirect_resources(void *cls, const char *resource_url);
enum MHD_Result process_cookies(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
void allocate_process(void *cls);
void serve_js(void *cls);
void sanitize_response(void *cls);
char *inject_data(const char *injection, const size_t injection_size, char *buffer, size_t *buffer_size, char *injection_pos);
char *erase_data(char *buffer, size_t *buffer_size, char *start_pos, char *end_pos);
StringExtract *find_directive(const char *haystack, const char *directive, const char *symbol, const char *end_symbol);
StringExtract *find_domain(const char *url);
#endif // CORE_PROXY_H
