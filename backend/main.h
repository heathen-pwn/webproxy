#ifndef MAIN_H
#define MAIN_H


#include <stdio.h>
#include <curl.h>
#include "http_fetch.h"
#include <pthread.h>
#include <json-c/json.h>
#include "session.h"
#include "thread_pool.h"

extern pthread_cond_t cond_collect_garbage;

typedef struct {
    int port;
    SessionTable *sessionsTable;
    float sessions_threshold;
    int sessions_timeout;
    int default_table_size;
    float minimum_threshold;
    thread_pool_t *thread_pool;
} App;

enum APP_RESULT {
    APP_NO = 0,
    APP_YES = 1
};

#include "http_server.h"

// Function declarations go here

enum APP_RESULT load_config(App *context);

#endif /* MAIN_H */