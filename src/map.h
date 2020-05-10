#pragma once

#include <stdbool.h>

struct map;

struct map *map_new();
void map_free(struct map *);

void map_insert(struct map *m, const char *key, void *ptr);
void *map_get(struct map *m, const char *key);
void map_remove(struct map *m, const char *key);

// Iterates over key/value pairs in the map (in arbitrary order). Stops
// iterating when the provided callback returns false. The callback must not
// mutate the key, value or any other entry in the map. The provided context
// will be passed to each invocation of the callback.
// Returns true unless callback stops iteration early.
bool map_iter(struct map *m, void *context,
              bool (*callback)(void *context, const char *key, void *value));
