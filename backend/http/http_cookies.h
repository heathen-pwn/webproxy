#ifndef HTTP_COOKIES_H
#define HTTP_COOKIES_H

#include <microhttpd.h>
#include "session.h"
#include "../main.h"

typedef struct {
    const char *key;
    char *value;
} Cookie;

// Function declarations go here

void add_cookie(struct MHD_Response *response, Cookie *cookie);

#endif /* HTTP_COOKIES_H */