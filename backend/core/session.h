#ifndef SESSION_H
#define SESSION_H

#include <stdlib.h>

// TBD: dynamic resizing 
#define MAX_SESSIONS 32

typedef struct
{
    char *session_id;
    char *session_key;
    time_t update;
    const char *session_url; // must be freed (malloc)
} Session;

typedef struct SessionNode
{
    Session *session;
    struct SessionNode *next;
} SessionNode;

typedef struct
{
    SessionNode **table; // Table array that contains the nodes
} SessionTable; // The hash table itself

SessionTable *create_sessions_table();
Session *get_session(SessionTable *session_table, const char *session_id);
Session *create_session();
void register_session(SessionTable *session_table, Session *session);

void free_session(Session *session);
void free_sessions_table(SessionTable *session_table);
void free_session_node(SessionNode *node);

void update_session_tick(Session *ses);


#endif /* SESSION_H */