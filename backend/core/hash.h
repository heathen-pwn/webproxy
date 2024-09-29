#ifndef HASH_H
#define HASH_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
// Function declarations go here

uint32_t murmur3_32(const char* key, size_t len, uint32_t seed, uint32_t table_size);

#endif /* HASH_H */