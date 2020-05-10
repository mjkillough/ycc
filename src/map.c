#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"

#define TOMBSTONE ((void *)-1)

static uint32_t string_key_fnv_hasher(const void *key) {
    const char *str = key;
    uint32_t hash = 2166136261;
    for (; *str; str++) {
        hash ^= *str;
        hash *= 16777619;
    }
    return hash;
}

static bool string_key_eq(const void *key1, const void *key2) {
    return !strcmp(key1, key2);
}

struct map_key *map_key_string = &(struct map_key){
    .hasher = string_key_fnv_hasher,
    .eq = string_key_eq,
};

static uint32_t pointer_key_fnv_hasher(const void *key) {
    // NOTE: Casting pointer to integer is implementation defined.
    const char *ptr = (const char *)&key;
    uint32_t hash = 2166136261;
    for (size_t i = 0; i < sizeof(const void *); i++) {
        hash ^= *(ptr + i);
        hash *= 16777619;
    }
    return hash;
}

static bool pointer_key_eq(const void *key1, const void *key2) {
    return key1 == key2;
}

struct map_key *map_key_pointer = &(struct map_key){
    .hasher = pointer_key_fnv_hasher,
    .eq = pointer_key_eq,
};

struct entry {
    const void *key;
    void *ptr;
};

struct map {
    struct map_key *key;
    struct entry *entries;
    size_t count;
    size_t capacity;
};

struct map *map_new(struct map_key *key) {
    struct map *m = calloc(1, sizeof(struct map));
    m->entries = calloc(16, sizeof(struct entry));
    m->count = 0;
    m->capacity = 16;
    m->key = key;
    return m;
}

void map_free(struct map *m) {
    free(m->entries);
    free(m);
}

static bool overloaded(struct map *m) {
    // 0.5 load factor
    return m->count > (m->capacity - 1);
}

static size_t entry(struct map *m, const void *key) {
    return m->key->hasher(key) & (m->capacity - 1);
}

static bool key_eq(struct map *m, const void *key1, const void *key2) {
    if (key1 == NULL || key1 == TOMBSTONE || key2 == NULL ||
        key2 == TOMBSTONE) {
        return false;
    }
    return m->key->eq(key1, key2);
}

static void resize(struct map *m) {
    struct entry *old_entries = m->entries;
    size_t old_capacity = m->capacity;

    m->count = 0; // everything will be re-inserted
    m->capacity *= 2;
    m->entries = calloc(m->capacity, sizeof(struct entry));

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].key != NULL && old_entries[i].key != TOMBSTONE) {
            map_insert(m, old_entries[i].key, old_entries[i].ptr);
        }
    }

    free(old_entries);
}

void map_insert(struct map *m, const void *key, void *ptr) {
    if (overloaded(m)) {
        resize(m);
    }

    size_t first_tombstone = m->capacity; // not found
    size_t i = entry(m, key);
    for (;; i = (i + 1) & (m->capacity - 1)) {
        if (key_eq(m, m->entries[i].key, key)) {
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
            m->entries[i] = (struct entry){.key = key, .ptr = ptr};
            m->count++;
            return;
        }
    }
}

void *map_get(struct map *m, const void *key) {
    size_t i = entry(m, key);
    for (;; i = (i + 1) & (m->capacity - 1)) {
        if (key_eq(m, m->entries[i].key, key)) {
            return m->entries[i].ptr;
        } else if (m->entries[i].key == NULL) {
            return NULL;
        }
    }
}

void map_remove(struct map *m, const void *key) {
    size_t i = entry(m, key);
    for (;; i = (i + 1) & (m->capacity - 1)) {
        if (key_eq(m, m->entries[i].key, key)) {
            m->entries[i].key = TOMBSTONE;
            m->count--;
            return;
        } else if (m->entries[i].key == NULL) {
            return;
        }
    }
}

bool map_iter(struct map *m, void *context,
              bool (*callback)(void *context, const void *key, void *value)) {
    for (struct entry *it = m->entries; it < (m->entries + m->capacity); it++) {
        if (it->key != NULL && it->key != TOMBSTONE) {
            if (!callback(context, it->key, it->ptr)) {
                return false;
            }
        }
    }
    return true;
}
