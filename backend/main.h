#ifndef MAIN_H
#define MAIN_H


#include <stdio.h>
#include <curl.h>
#include "http_fetch.h"
#include <pthread.h>
#include <json-c/json.h>
#include "session.h"

extern pthread_cond_t cond_collect_garbage;

typedef struct {
    SessionTable *sessionsTable;
    int sessions_threshold;
    int sessions_timeout;
} App;

#include "http_server.h"

// Function declarations go here


#endif /* MAIN_H */