#include <stdio.h>
#include <stdlib.h>
#include "session.h"
#include "hash.h"
#include "uniques.h"

// A session will expire after timeout, meant to stay for the period of the connection
// When using keep-alive, we can get the signal when the user closes the connection so we free the session memory
// Otherwise cleanup sessions after timeout (garbage collection)

// Lookup session from session table
Session *get_session(SessionTable *session_table, const char *session_id)
{

    uint32_t index = murmur3_32(session_id, strlen(session_id), 0, MAX_SESSIONS);
    SessionNode *current = session_table->table[index];

    // Collision handling

    while (current != NULL)
    {
        printf("Comparing node %s with given %s\n", current->session->session_id, session_id);
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
Session *create_session()
{
    Session *session = malloc(sizeof(Session)); // must be freed by caller
    if (!session)
    {
        fprintf(stderr, " malloc failed @create_session");
        return NULL;
    }
    session->session_id = generate_uuid();
    session->session_key = generate_uuid();
    return session;
}

// Function to insert a session into the hash table
void register_session(SessionTable *session_table, Session *session)
{
    if (!session_table || !session)
    {
        fprintf(stderr, "register_session failed\n");
        return;
    }
    uint32_t index = murmur3_32(session->session_id, strlen(session->session_id), 0, MAX_SESSIONS);
    
    if (index > MAX_SESSIONS)
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
    new_node->session = session;
    new_node->next = session_table->table[index];
    session_table->table[index] = new_node; // Insert at the beginning of the linked list
    printf("Session %s successfully registered @register_session\n", session_table->table[index]->session->session_id);
}

// Create a Sessions table
SessionTable *create_sessions_table()
{
    SessionTable *session_table = malloc(sizeof(SessionTable));
    if (!session_table)
    {
        fprintf(stderr, "create_session_table failed: memory could not be allocated");
        return NULL;
    }
    session_table->table = malloc(sizeof(SessionNode *) * MAX_SESSIONS);
    if (!session_table->table)
    {
        fprintf(stderr, "create_session table failed: table array memory allocation failed");
        return NULL;
    }
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        session_table->table[i] = NULL; // Initialize all nodes to NULL
    }

    return session_table;
}

// Free session
void free_session(Session *session)
{
    if (session)
    {
        free(session->session_id);
        free(session->session_key);
        free(session);
    }
}
// Free sessions table
void free_sessions_table(SessionTable *session_table)
{
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        SessionNode *current = session_table->table[i];
        while (current != NULL)
        {
            SessionNode *temp = current;
            current = current->next; // Move to the next node
            free_session_node(temp); // Free the current node
        }
    }
    free(session_table->table); // Free the table array
    free(session_table);        // Free the hash table itself
}
// Free a single session node inside table array inside hashtable
void free_session_node(SessionNode *node)
{
    if (node)
    {
        free_session(node->session); // Free the session associated with the node
        free(node);                  // Free the node itself
    }
}

void update_session_tick(Session *ses) {
    if(ses) {
        ses->update = time(NULL);
    }
}
// Close session (connection: close)
void close_session(Session *ses) {

}
// Removing stale sessions
void sessions_garbage_collector() {

} 

// TBD: Garbage collection for sessions after timeout/close connection
