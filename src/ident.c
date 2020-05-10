#include <string.h>

#include "map.h"
#include "vec.h"

#include "ident.h"

struct ident {
    const char *str;
};

struct ident_table {
    // Same const char* in both map and vec.
    // Owned by the ident_table.

    // map[const char*]size_t*
    struct map *map;
    // [size_t]struct ident*
    struct vec *vec;
};

struct ident_table *ident_table_new() {
    struct ident_table *t = malloc(sizeof(struct ident_table));
    t->map = map_new();
    t->vec = vec_new(sizeof(struct ident *));
    return t;
}

void ident_table_free(struct ident_table *t) {
    // TODO: free all size_t* and const char*

    map_free(t->map);
    vec_free(t->vec);
    free(t);
}

size_t ident_table_len(struct ident_table *t) { return vec_len(t->vec); }

struct ident *ident_from_str(struct ident_table *t, const char *str) {
    // TODO: map_get_or_insert() would probably be faster.
    size_t *existing_idx = map_get(t->map, str);
    if (existing_idx != NULL) {
        return *(struct ident **)vec_get(t->vec, *existing_idx);
    }

    // TODO: It'd be nice not to be able to put size_t directly in the map
    // instead of having to malloc().
    const char *owned = strdup(str);
    struct ident *ident = malloc(sizeof(struct ident));
    ident->str = owned;
    size_t *idx = malloc(sizeof(size_t));
    *idx = vec_append(t->vec, &ident);
    map_insert(t->map, ident->str, idx);

    return *(struct ident **)vec_get(t->vec, *idx);
}

const char *ident_to_str(struct ident *ident) { return ident->str; }

