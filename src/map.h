#pragma once

#include <stdbool.h>

typedef struct map map;

map *map_new();
void map_insert(map *m, const char *key, void *ptr);
void *map_get(map *m, const char *key);
void map_remove(map *m, const char *key);

// Iterates over key/value pairs in the map (in arbitrary order). Stops
// iterating when the provided callback returns false. The callback must not
// mutate the key, value or any other entry in the map. The provided context
// will be passed to each invocation of the callback.
// Returns true unless callback stops iteration early.
bool map_iter(map *m, void *context,
              bool (*callback)(void *context, const char *key, void *value));
