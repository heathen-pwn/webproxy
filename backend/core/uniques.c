#include "uniques.h"

// Generate UUID
char *generate_uuid()
{
    uuid_t binary_uuid;
    uuid_generate_random(binary_uuid);
    char *uuid = malloc(36 + 1); // null terminator
    if(!uuid) {
        fprintf(stderr, "generate_uuid failed; could not allocate memory for uuid string");
        return NULL;
    }
    uuid_unparse_upper(binary_uuid, uuid);
    return uuid; // caller must free the dynamic memory here
}