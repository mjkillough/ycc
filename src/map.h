#pragma once

#include <stdbool.h>

typedef struct {
    const char *key;
    void *ptr;
} map_entry_t;

typedef struct {
    map_entry_t *entries;
    size_t count;
    size_t capacity;
} map_t;

map_t *map_new();
void map_insert(map_t *m, const char *key, void *ptr);
void *map_get(map_t *m, const char *key);
void map_remove(map_t *m, const char *key);
bool map_iter(map_t *m, map_entry_t **entry);
