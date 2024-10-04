#include <stdio.h>
#include <curl.h>
#include <pthread.h>
#include <json-c/json.h>
#include "main.h"
#include "http_fetch.h"
#include "session.h"
#include <unistd.h>
#include "thread_pool.h"

pthread_mutex_t session_mutex;
pthread_cond_t cond_collect_garbage;

int main() {
    // Entry
    App *context = malloc(sizeof(App));
    if(!context) {
        fprintf(stderr, "Could not allocate memory for application context! Exiting..");
        return -1; 
    }  
    thread_pool_t *thread_pool = thread_pool_init(2); // Garbage collection & scaling
    if(thread_pool) {
        context->thread_pool = thread_pool;
    } else {
        fprintf(stderr, "Thread pool could not be allocated");
        return -1;
    }
    // Setting up configurations
    load_config(context);

    curl_global_init(CURL_GLOBAL_DEFAULT); // Initilaizing curl.. called once per program lifetime
    pthread_mutex_init(&session_mutex, NULL);
    pthread_cond_init(&cond_collect_garbage, NULL);

    // Set up sessions table (can be integrated in load_config as part of the configuration file later)
    context->sessionsTable = create_sessions_table(context->default_table_size);
    if(!context->sessionsTable) {
        fprintf(stderr, "Could not create sessions table! Failed");
        return -1;
    }
    // Starting the API 
    // Daemon creates a separate thread
    struct MHD_Daemon *daemon = start_http_server(
            context,
            MHD_USE_AUTO_INTERNAL_THREAD,
            context->port
    );
    if(daemon == NULL) {
        printf("Could not start server!\n");
        return 1;
    }

    printf("[microhttpdaemon] Listening on %d\n", context->port);

    // Threading
    // pthread_t garbage_collector; 
    // int res = pthread_create(&garbage_collector, NULL, &collect_session_garbage, (void *)context);
    // if(res != 0) {
    //     fprintf(stderr, "Could not create garbage collector thread!");
    // }

    getchar();
    
    // Cleanup
    free_sessions_table(context->sessionsTable);
    curl_global_cleanup();
}

// Load configurations
enum APP_RESULT load_config(App *context) {
    if(!context) {
        fprintf(stderr, "Invalid context pointer");
        return APP_NO;
    }
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd) ) != NULL) {
        
        // Read configurations
        size_t path_size = strlen(cwd) + strlen("/app.conf") + 1;
        
        char *path = malloc(path_size);
        if(!path) {
            fprintf(stderr, "Path failed @load_config\n");
            return APP_NO;
        }
         
        snprintf(path, path_size, "%s/app.conf", cwd);

        FILE *configfile = fopen(path, "r");
        if(!configfile) {
            fprintf(stderr, "Configuration file not found!\n");
            free(path);
            return APP_NO;
        }
        fseek(configfile, 0, SEEK_END);
        long length = ftell(configfile);
        fseek(configfile, 0, SEEK_SET);
        char *data = malloc(length + 1);
        fread(data, 1, length, configfile);
        data[length] = '\0'; // Null-terminate the string
        fclose(configfile);
        free(path);

        // Parse JSON Data
        struct json_object *parsed_json = json_tokener_parse(data);
        printf("PARSED: %s\n", data);
        free(data);

        struct json_object *app_obj;
        
        // Loading general program configurations
        if(json_object_object_get_ex(parsed_json, "app", &app_obj)) {
            struct json_object *max_sessions_obj;
            printf("JSON: Got 'app' object");
            if(json_object_object_get_ex(app_obj, "default_table_size", &max_sessions_obj)) {
                context->default_table_size = json_object_get_int(max_sessions_obj);
            }
        }
        struct json_object *proxy_obj;
        // Loading proxy-related configurations from app.conf
        if(json_object_object_get_ex(parsed_json, "proxy", &proxy_obj)) {
            
            struct json_object *port_obj;
            struct json_object *sessions_threshold_obj;
            struct json_object *sessions_timeout_obj;
            // struct json_object *max_sessions_obj;
            printf("JSON: Got 'proxy' Object \n");
            if (json_object_object_get_ex(proxy_obj, "port", &port_obj)) {
                context->port = json_object_get_int(port_obj);
                printf("Loaded port: %d\n", context->port);
            } 

            if (json_object_object_get_ex(proxy_obj, "sessions_threshold", &sessions_threshold_obj)) {
                context->sessions_threshold = json_object_get_double(sessions_threshold_obj);
                printf("Loaded threshold: %f\n", context->sessions_threshold);
            }
            
            if (json_object_object_get_ex(proxy_obj, "sessions_timeout", &sessions_timeout_obj)) {
                context->sessions_timeout = json_object_get_int(sessions_timeout_obj);
                printf("Loaded sessions timeout: %d\n", context->sessions_timeout);
            }
            
            // Pro-tip: can make create_sessions_table here as a json setting (context->sessionsTable = create_sessions_table(context->sessionsTable->table_size))    
            // if (json_object_object_get_ex(proxy_obj, "max_sessions", &max_sessions_obj)) {
            //     context->sessionsTable->table_size = json_object_get_int(max_sessions_obj);
            //     printf("Loaded table size: %d\n", context->sessionsTable->table_size);
            // }
            
        
        } else {
            printf("Proxy configuration object not found!\n");
        }

        // Load default values to ensure smooth program execution regardless of conf
        if(!context->default_table_size)
            context->default_table_size = 32;
        if(!context->port) 
            context->port = 8080;
        if(!context->sessions_threshold)
            context->sessions_threshold = 0.75;
        if(!context->sessions_timeout) 
            context->sessions_timeout = 15;  // in minutes
        if(context->sessionsTable) {
            if(!context->sessionsTable->table_size) 
                context->sessionsTable->table_size = 32;
        }
         

        json_object_put(parsed_json);

    } else {
        fprintf(stderr, "getcwd failed");
        return APP_NO;
    }
    return APP_YES;
}
