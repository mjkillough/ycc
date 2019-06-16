#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"

#define TOMBSTONE ((void *)-1)

map_t *map_new() {
    map_t *m = calloc(1, sizeof(map_t));
    m->entries = calloc(16, sizeof(map_entry_t));
    m->count = 0;
    m->capacity = 16;
    return m;
}

static bool overloaded(map_t *m) {
    // 0.5 load factor
    return m->count > (m->capacity - 1);
}

static uint32_t fnv_hash(const char *key) {
    uint32_t hash = 2166136261;
    for (; *key; key++) {
        hash ^= *key;
        hash *= 16777619;
    }
    return hash;
}

static size_t entry(map_t *m, const char *key) {
    return fnv_hash(key) & (m->capacity - 1);
}

static bool key_eq(const char *key1, const char *key2) {
    if (key1 == NULL || key1 == TOMBSTONE || key2 == NULL ||
        key2 == TOMBSTONE) {
        return false;
    }
    return !strcmp(key1, key2);
}

static void resize(map_t *m) {
    map_entry_t *old_entries = m->entries;
    size_t old_capacity = m->capacity;

    m->count = 0; // everything will be re-inserted
    m->capacity *= 2;
    m->entries = calloc(m->capacity, sizeof(map_entry_t));

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].key != NULL && old_entries[i].key != TOMBSTONE) {
            map_insert(m, old_entries[i].key, old_entries[i].ptr);
        }
    }

    free(old_entries);
}

void map_insert(map_t *m, const char *key, void *ptr) {
    if (overloaded(m)) {
        resize(m);
    }

    size_t first_tombstone = m->capacity; // not found
    size_t i = entry(m, key);
    for (;; i = (i + 1) & (m->capacity - 1)) {
        if (key_eq(m->entries[i].key, key)) {
            m->entries[i].ptr = ptr;
        } else if (m->entries[i].key == TOMBSTONE) {
            if (first_tombstone == m->capacity) {
                first_tombstone = i;
            }
        } else if (m->entries[i].key == NULL) {
            // Key isn't in map, fill in first tombstone if we passed one, or
            // current location.
            if (first_tombstone != m->capacity) {
                i = first_tombstone;
            }
            m->entries[i] = (map_entry_t){.key = key, .ptr = ptr};
            m->count++;
            return;
        }
    }
}

void *map_get(map_t *m, const char *key) {
    size_t i = entry(m, key);
    for (;; i = (i + 1) & (m->capacity - 1)) {
        if (key_eq(m->entries[i].key, key)) {
            return m->entries[i].ptr;
        } else if (m->entries[i].key == NULL) {
            return NULL;
        }
    }
}

void map_remove(map_t *m, const char *key) {
    size_t i = entry(m, key);
    for (;; i = (i + 1) & (m->capacity - 1)) {
        if (key_eq(m->entries[i].key, key)) {
            m->entries[i].key = TOMBSTONE;
            m->count--;
            return;
        } else if (m->entries[i].key == NULL) {
            return;
        }
    }
}

bool map_iter(map_t *m, map_entry_t **entry) {
    if (*entry == NULL) {
        *entry = m->entries;
    } else {
        (*entry)++;
    }
    while (*entry != (m->entries + m->capacity)) {
        if ((*entry)->key != NULL && (*entry)->key != TOMBSTONE) {
            return true;
        }
        (*entry)++;
    }
    *entry = NULL;
    return false;
}
