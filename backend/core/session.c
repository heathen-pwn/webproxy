#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "hash.h"
#include "uniques.h"
#include "session.h"
#include "../main.h"
// A session will expire after timeout, meant to stay for the period of the connection
// When using keep-alive, we can get the signal when the user closes the connection so we free the session memory
// Otherwise cleanup sessions after timeout (garbage collection)

// Lookup session from session table
Session *get_session(SessionTable *session_table, const char *session_id)
{

    uint32_t index = murmur3_32(session_id, strlen(session_id), 0, session_table->table_size);
    SessionNode *current = session_table->table[index];

    // Collision handling

    while (current != NULL)
    {
        // printf("Comparing node %s with given %s\n", current->session->session_id, session_id);
        if (strcmp(current->session->session_id, session_id) == 0)
        {
            printf("Session %s found in the server\n", session_id);
            return current->session; // Session found
        }
        current = current->next; // Move to the next node
    }

    return NULL; // Session not found
}

// Create session, session needs to be added to the hashtable with register_session
Session *create_session(void *cls)
{
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    Session *session = malloc(sizeof(Session)); // must be freed by caller
    if (!session)
    {
        fprintf(stderr, " malloc failed @create_session");
        return NULL;
    }
    pthread_mutex_lock(&session_mutex);
    
    session->session_id = generate_uuid();
    session->session_key = generate_uuid();

    request_essentials->context->sessionsTable->node_count++;

    pthread_mutex_unlock(&session_mutex);

    printf("Node Count: %d|", request_essentials->context->sessionsTable->node_count);
    printf("Table_Size %d|", request_essentials->context->sessionsTable->table_size);
    printf("Threshold %.2f\n", request_essentials->context->sessions_threshold);

    // Scaling
    thread_pool_add(request_essentials->context->thread_pool, scale_sessions_table, (void *)request_essentials);

    return session;
}
void scale_sessions_table(void *cls) {
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    SessionTable *sessions_table = NULL;
    if(!request_essentials->context) {
        fprintf(stderr, "NO CONTEXT!!!\n");
        return;
    }
    if(!request_essentials->context->sessionsTable) {
        fprintf(stderr, "NO TABLE IN REQUEST_ESSENTIALS_CONTEXT\n");
        return;
    }
    // Scale up
    pthread_mutex_lock(&session_mutex);

    if(request_essentials->context->sessionsTable->node_count > request_essentials->context->sessionsTable->table_size*(request_essentials->context->sessions_threshold)) {
        resize_sessions_table(request_essentials->context->sessionsTable, (request_essentials->context->sessionsTable->table_size)*2, (void *)request_essentials);
        thread_pool_add(request_essentials->context->thread_pool, collect_session_garbage, (void *)request_essentials->context);
    } else if (request_essentials->context->sessionsTable->table_size != request_essentials->context->default_table_size) {
        if(request_essentials->context->sessionsTable->node_count < (request_essentials->context->sessionsTable->table_size)/2) { // Scale down
            resize_sessions_table(request_essentials->context->sessionsTable, (request_essentials->context->sessionsTable->table_size)/2, (void *)request_essentials);
        }
    }

    pthread_mutex_unlock(&session_mutex);
}
void resize_sessions_table(SessionTable *session_table, int new_table_size, void *cls) {
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    // Create new scaled table
    SessionNode **scaled_session_table = calloc(new_table_size, sizeof(SessionNode *));
    if(!scaled_session_table) {
        fprintf(stderr, "Error allocating scaled table!");
        return;
    }
    // Rehash existing entries to the new table
    for (size_t i = 0; i < request_essentials->context->sessionsTable->table_size; ++i) {
        SessionNode *node = request_essentials->context->sessionsTable->table[i];
        while (node) {
            // Calculate new index based on new_table_size
            size_t new_index = murmur3_32(node->session->session_id, strlen(node->session->session_id), 0, new_table_size);
            //murmur3_32(session->session_id, strlen(session->session_id), 0, session_table->table_size);
            // Move node to the new table
            SessionNode *next_node = node->next;
            node->next = scaled_session_table[new_index];
            scaled_session_table[new_index] = node;
            node = next_node;
        }
    }
    // Free the old table and update table reference and size
    free(request_essentials->context->sessionsTable->table);
    request_essentials->context->sessionsTable->table = scaled_session_table;
    request_essentials->context->sessionsTable->table_size = new_table_size;

    printf("Scaled sessions table to size %d\n", new_table_size);
}
// Function to insert a session into the hash table
void register_session(SessionTable *session_table, Session *session)
{
    if (!session_table || !session)
    {
        fprintf(stderr, "register_session failed\n");
        return;
    }
    uint32_t index = murmur3_32(session->session_id, strlen(session->session_id), 0, session_table->table_size);

    if (index > session_table->table_size)
    {
        fprintf(stderr, "register_session failed; index array out of bounds\n");
        return;
    }
    SessionNode *new_node = malloc(sizeof(SessionNode)); // Caller must free
    if (!new_node)
    {
        fprintf(stderr, "register_session failed; could not allocate memory for new node\n");
        return;
    }
    // Note here that the session mutex is being used to access the hash table, this can cause perf. overhead in a highly concurrent environment
    // Solution: Separate table mutex from session mutex, and you can even create a mutex for each bucket of the hash table for higher perf.
    pthread_mutex_lock(&session_mutex);

    new_node->session = session;
    new_node->next = session_table->table[index];
    session_table->table[index] = new_node; // Insert at the beginning of the linked list

    pthread_mutex_unlock(&session_mutex);

    printf("Session %s successfully registered at index %d @register_session\n", session_table->table[index]->session->session_id, index);
}


// Create a Sessions table
SessionTable *create_sessions_table(int table_size)
{
    SessionTable *session_table = malloc(sizeof(SessionTable));
    if (!session_table)
    {
        fprintf(stderr, "create_session_table failed: memory could not be allocated");
        return NULL;
    }
    session_table->table = malloc(sizeof(SessionNode *) * table_size);
    if (!session_table->table)
    {
        fprintf(stderr, "create_session table failed: table array memory allocation failed");
        return NULL;
    }
    for (int i = 0; i < table_size; i++)
    {
        pthread_mutex_lock(&session_mutex);

        session_table->table[i] = NULL; // Initialize all nodes to NULL
        session_table->table_size = table_size;

        pthread_mutex_unlock(&session_mutex);
    }
    return session_table;
}

// Free session
void free_session(void *cls, Session *session)
{
    App *context = (App *)cls;
    if (session)
    {
        free(session->session_id);
        free(session->session_key);
        free(session->session_url);
        free(session);
        session = NULL;
        context->sessionsTable->node_count--;
    }
}
// Free sessions table
void free_sessions_table(SessionTable *session_table)
{

    for (int i = 0; i < session_table->table_size; i++)
    {
        SessionNode *current = session_table->table[i];
        while (current != NULL)
        {
            SessionNode *temp = current;

            pthread_mutex_lock(&session_mutex);

            current = current->next; // Move to the next node
            free_session_node(temp); // Free the current node

            pthread_mutex_unlock(&session_mutex);
        }
    }
    pthread_mutex_lock(&session_mutex);

    free(session_table->table); // Free the table array
    free(session_table);        // Free the hash table itself

    pthread_mutex_unlock(&session_mutex);
}
// Free a single session node inside table array inside hashtable
void free_session_node(SessionNode *node)
{
    if (node)
    {
        free(node);            // Free the node itself
    }
}

void update_session_tick(Session *ses)
{
    if (ses)
    {
        pthread_mutex_lock(&session_mutex);
        ses->update = time(NULL);
        pthread_mutex_unlock(&session_mutex);
    }
}
// Close session (connection: close)


// Removing stale sessions
void collect_session_garbage(void *arg)
{
    App *context = (App *)arg;

    pthread_mutex_lock(&session_mutex);
    // pthread_cond_wait(&cond_collect_garbage, &session_mutex);
    printf("collect_session_garbage @ signaled!\n");
    
    for (int i = 0; i < context->sessionsTable->table_size; i++)
    {
        SessionNode *current = context->sessionsTable->table[i];
        printf("accessing table index %d\n", i);
        SessionNode *previous = NULL;
        // Collision handling

        while (current != NULL)
        {
            if(current->session == NULL) {
                printf("current->session is NULL\n\n");
                break;
            }
            if(!current->session->update) {
                printf("update is empty->");
                break;
            }
            printf("Time: %ld", current->session->update);
            printf("Time check: %d", current->session->update < time(NULL) - context->sessions_timeout);
            if (current->session->update < time(NULL) - 10)//context->sessions_timeout)
            {
                printf("passed time check->");
                // Freed session causing crash?.. check who is accessing it? (after adding cookie to response?)
                SessionNode *next_node = current->next;
                free_session(context, current->session);
                
                // Remove current node from the list
                if(previous == NULL) 
                {
                    context->sessionsTable->table[i] = current->next; 
                    free_session_node(current);
                    printf("free node->");
                    // Crashing after this
                
                } else { // bypass deleted item
                    previous->next = current->next;
                    printf("bypass deleted item->");
                }
                current = next_node;
                printf("session at index %d expired", i);
                // printf("[collect_session_garbage] Session ID %s has expired! Index: %d", current->session->session_id, i);
            } else {
                previous = current;
                current = current->next; // Move to the next node
                printf("didnt pass time check, moving on to next node->");
            }
        }
    };
    pthread_mutex_unlock(&session_mutex);
    return;
}
// TBD: Garbage collection for sessions after timeout/close connection
