#include <stdlib.h>


// A session will expire after timeout, meant to stay for the period of the connection
// When using keep-alive, we can get the signal when the user closes the connection so we free the session memory
// Otherwise cleanup sessions after timeout (garbage collection)
typedef struct {
    char session_id[33]; // 32 + 1 null terminator
    int user_id;
} SessionNode;

void *create_session(int user_id) {

}

void *get_session(const char* session_id) {

}
