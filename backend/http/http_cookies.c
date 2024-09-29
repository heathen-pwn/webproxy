#include "http_cookies.h"
#include "session.h"
#include <microhttpd.h>


// Add cookie to specified HTTP response
void add_cookie(struct MHD_Response *response, Cookie *cookie)
{
    const size_t cookie_len = snprintf(NULL, 0, "%s=%s", cookie->key, cookie->value);
    char *cookie_header = malloc(cookie_len + 1); // null terminator
    snprintf(cookie_header, cookie_len + 1, "%s=%s", cookie->key, cookie->value);
    if (cookie_header)
    {
        MHD_add_response_header(response, "Set-Cookie", cookie_header);
        printf("Added cookie to response: %s\n", cookie_header);
        free(cookie_header);
    } else {
        fprintf(stderr, "Could not add cookie to header; add_cookie failed");
    }
}

