#ifndef SESSION_H
#define SESSION_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
// Protecting session race condition
extern pthread_mutex_t session_mutex;

typedef struct
{
    char *session_id;
    char *session_key;
    time_t update;
    char *session_url; 
    size_t session_url_size;
} Session;

typedef struct SessionNode
{
    Session *session;
    struct SessionNode *next;
} SessionNode;

typedef struct
{
    SessionNode **table; // Table array that contains the nodes
    int table_size;
    int node_count;
} SessionTable; // The hash table itself

SessionTable *create_sessions_table(int table_size);
Session *get_session(SessionTable *session_table, const char *session_id);
Session *create_session(void *cls);
void register_session(SessionTable *session_table, Session *session);

void free_session(void *cls, Session *session);
void free_sessions_table(SessionTable *session_table);
void free_session_node(SessionNode *node);

void update_session_tick(Session *ses);
void collect_session_garbage(void *arg);

void resize_sessions_table(void *cls, int new_table_size);
void scale_sessions_table(void *cls);



#endif /* SESSION_H */