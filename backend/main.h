#ifndef MAIN_H
#define MAIN_H


#include <stdio.h>
#include "curl.h"
#include "http_fetch.h"
#include "session.h"

typedef struct {
    SessionTable *sessionsTable;
} App;

#include "http_server.h"

// Function declarations go here


#endif /* MAIN_H */