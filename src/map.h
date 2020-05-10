#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct map_key {
    uint32_t (*hasher)(const void *);
    bool (*eq)(const void *, const void *);
};

extern struct map_key *map_key_string;
extern struct map_key *map_key_pointer;

struct map;

struct map *map_new(struct map_key *key);
void map_free(struct map *);

void map_insert(struct map *m, const void *key, void *ptr);
void *map_get(struct map *m, const void *key);
void map_remove(struct map *m, const void *key);

// Iterates over key/value pairs in the map (in arbitrary order). Stops
// iterating when the provided callback returns false. The callback must not
// mutate the key, value or any other entry in the map. The provided context
// will be passed to each invocation of the callback.
// Returns true unless callback stops iteration early.
bool map_iter(struct map *m, void *context,
              bool (*callback)(void *context, const void *key, void *value));
