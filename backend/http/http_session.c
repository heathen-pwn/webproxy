#include "http_server.h"
#include "session.h"

void *handle_session_management(void *cls) {
    RequestEssentials *request_essentials = (RequestEssentials *)cls;
    const char *client_session = MHD_lookup_connection_value(request_essentials->connection, MHD_COOKIE_KIND, "SessionID");
    // There is no session in the HTTP request, so set one up
    if (!client_session)
    {
        // Creating session
        Session *ses = create_session(); // MUST BE FREED when session ends
        if (!ses) {
            fprintf(stderr, "create_session failed @ session management");
        }
        insert_session(request_essentials->context->sessionsTable, ses); // seg fault
        printf("SessionID: %s\nSessionKey: %s\n", ses->session_id, ses->session_key);

        // Setting up session cookie
        Cookie cookie = {0};
        cookie.key = "SessionID";
        cookie.value = ses->session_id;
        add_cookie(request_essentials->response, &cookie);
    }
    return MHD_YES;
}