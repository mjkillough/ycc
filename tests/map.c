#include <stdio.h>
#include <string.h>

#include "map.h"

#include "framework.h"

TEST(string_keys) {
    char *key1a = "one", *value1 = "a";
    char *key1b = strdup(key1a);
    char *key2 = "two", *value2 = "b";
    char *key3 = "three", *value3 = "c";

    struct map *m = map_new(map_key_string);
    map_insert(m, key1a, value1);
    map_insert(m, key2, value2);
    map_insert(m, key3, value3);

    ASSERT(strcmp(map_get(m, key1a), value1) == 0);
    ASSERT(strcmp(map_get(m, key2), value2) == 0);
    ASSERT(strcmp(map_get(m, key3), value3) == 0);

    // key1b should return same element as key1a
    ASSERT(strcmp(map_get(m, key1b), value1) == 0);

    // inserting key1b should overwrite entry at key1a
    char *value4 = "d";
    map_insert(m, key1b, value4);
    ASSERT(strcmp(map_get(m, key1a), value4) == 0);
}

TEST(pointer_keys) {
    char *key1a = "one", *value1 = "a";
    char *key1b = strdup(key1a);
    char *key2 = "two", *value2 = "b";
    char *key3 = "three", *value3 = "c";

    struct map *m = map_new(map_key_pointer);
    map_insert(m, key1a, value1);
    map_insert(m, key2, value2);
    map_insert(m, key3, value3);

    ASSERT(strcmp(map_get(m, key1a), value1) == 0);
    ASSERT(strcmp(map_get(m, key2), value2) == 0);
    ASSERT(strcmp(map_get(m, key3), value3) == 0);

    // key1b should not return value1, as key1a != key1b
    ASSERT(map_get(m, key1b) == NULL);

    // inserting key1b should not overwrite entry at key1a
    char *value4 = "d";
    map_insert(m, key1b, value4);
    ASSERT(strcmp(map_get(m, key1a), value1) == 0);
    ASSERT(strcmp(map_get(m, key1b), value4) == 0);
}

