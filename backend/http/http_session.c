#include "http_server.h"
#include "session.h"

void manage_session(void *cls) {
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    const char *client_session = MHD_lookup_connection_value(request_essentials->connection, MHD_COOKIE_KIND, "SessionID");
    // There is no session in the HTTP request, so set one up
    Session *ses = is_valid_session(request_essentials, client_session);
    if (!ses)
    {
        // Session must be freed from its hash table when session ends (not in this function)
        printf("Invalid session received from client: %s (@manage_session)\n", client_session);
        setup_http_session(request_essentials);    
    } 
    return;
}
Session *setup_http_session(void *cls) {
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    Session *ses = NULL;
    const char *client_session = MHD_lookup_connection_value(request_essentials->connection, MHD_COOKIE_KIND, "SessionID");
    if(!is_valid_session(request_essentials, client_session))
    {
        // Creating session
        ses = create_session((void *)request_essentials); // MUST BE FREED when session ends
        if (!ses) {
            fprintf(stderr, "create_session failed  (@setup_http_session)\n");
        }
        // Figure otu a way to process this without returning null
        if(!request_essentials->request_full_url) {
            fprintf(stderr, "There is no URL to associatie with session\n");
            return NULL; 
        }
        size_t session_url_size = strlen(request_essentials->request_full_url);
        char *session_url_in_memory = malloc(session_url_size);
        if(!session_url_in_memory) {
            fprintf(stderr, "Failed to allocate memory for session url");
        } else {
            memcpy(session_url_in_memory, request_essentials->request_full_url, session_url_size);
            ses->session_url = session_url_in_memory;
        }

        printf("The session %s is proxying this website: %s\n", ses->session_id, ses->session_url);
        if(request_essentials->response) {
            Cookie cookie = {0};
            cookie.key = "SessionID";
            cookie.value = ses->session_id;
            add_cookie(request_essentials->response, &cookie);
        }
        register_session(request_essentials->context->sessionsTable, ses);
        printf("SessionID: %s\nSessionKey: %s  (@setup_http_session)\n", ses->session_id, ses->session_key);
    }
    return ses;
}
Session *is_valid_session(void *cls, const char *client_session) {
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    
    if (!client_session) {
        return NULL;  // No session cookie sent by the client
    }

    Session *ses = get_session(request_essentials->context->sessionsTable, client_session);
    
    if (!ses) { // Session not found or invalid
        return NULL;
    }
    return ses;
}
