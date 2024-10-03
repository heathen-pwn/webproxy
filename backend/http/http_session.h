#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

// Function declarations go here
enum SESSION_RESULT {
    SESSION_NO = 0,
    SESSION_YES = 1
};

void manage_session(void *cls);
Session *setup_http_session(void *cls);
enum SESSION_RESULT is_valid_session(void *cls, const char *client_session);


#endif /* HTTP_SESSION_H */