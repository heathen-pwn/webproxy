#include "curl.h"
#include "Uri.h"

// Convery a query string into a manageable query list of key-value pairs that can be retrieved with get_key_value
UriQueryListA *parse_query(const char *query) {
    // Initializing Uri instance
    UriUriA uri;
    const char *errorpos = NULL;
    int err;
    err = uriParseSingleUriA(&uri, query, &errorpos);
    
    if(err != URI_SUCCESS) {
        fprintf(stderr, "URI could not be initialized from provided query @parse_query\n");
        return NULL;
    }

    // Parsing the query into a list
    UriQueryListA *querylist = NULL;
    int itemcount;
    err = uriDissectQueryMallocA(&querylist, &itemcount, uri.query.first, uri.query.afterLast);
    if(err != URI_SUCCESS) {
        fprintf(stderr, "URI could not dissect query list (err: %d)\n", err);
        return NULL;
    }
    if(querylist == NULL) {
        printf("querylist is NULL but dissect didn't fail?\n");
    }
    // List has been parsed into an array list and now can be returned, the caller must free querylist
    uriFreeUriMembersA(&uri);
    return querylist;
}

const char* get_key_value(UriQueryListA *querylist, char *key) {
    UriQueryListA *current = querylist;
    if(!current) { 
        return NULL;
    }

    while(current) {
        printf("[get_key_value] current key %s\n", current->key);
        if(!strcmp(current->key, key)) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}
// Return the full query of a provided URL (after the /?)
char *get_query(const char *url) {
    printf("Url being passed is %s\n", url);
    // Getting the part of the URL that we need..
    char *query = NULL;
    CURLUcode res = 0; // error handler
    CURLU *handle = curl_url();
    curl_url_set(handle, CURLUPART_URL, url, 0);
    if(!res) { // success
        // Why is it &query and not 'query'? Because with &query we are passing the ADDRESS of the pointer so the function can edit it
        // If you just pass 'query', the function will get a copy of the pointer, and editing that won't help us since we will just return NULL bc. we won't get it updated on our pointer.
        res = curl_url_get(handle, CURLUPART_QUERY, &query, 0);
        if(!res) { // Success
            return query;
        }  
    }
    return NULL;
}

