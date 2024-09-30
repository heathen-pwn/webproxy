#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

static inline uint32_t murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51; // Multiply with a constant
    k = (k << 15) | (k >> 17); // Rotate left
    k *= 0x1b873593; // Multiply with another constant
    return k;
}

uint32_t murmur3_32(const char* key, size_t len, uint32_t seed, uint32_t table_size) {
    // Check for NULL key and handle zero-length case
    if (!key || len == 0) {
        return seed; // Return the seed if the key is NULL or length is zero
    }

    uint32_t h = seed; // Initialize hash value
    uint32_t k; // Variable for chunks of the key

    // Read in groups of 4 bytes
    for (size_t i = len >> 2; i; i--) {
        memcpy(&k, key, sizeof(uint32_t)); // Read 4 bytes from key
        key += sizeof(uint32_t); // Move the key pointer forward
        h ^= murmur_32_scramble(k); // Scramble and mix into hash
        h = (h << 13) | (h >> 19); // Rotate
        h = h * 5 + 0xe6546b64; // Mix constant
    }

    // Process remaining bytes (1 to 3)
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8; // Shift left for the next byte
        k |= key[i - 1]; // Add the byte to k
    }

    // Final mix for remaining bytes
    h ^= murmur_32_scramble(k);

    // Finalize hash
    h ^= len; // Mix in the length
    h ^= h >> 16; // Final mixing
    h *= 0x85ebca6b; // Another mixing constant
    h ^= h >> 13; // More mixing
    h *= 0xc2b2ae35; // Final mixing constant
    h ^= h >> 16; // Final hash value

    return h % table_size; // Return the computed hash
}
