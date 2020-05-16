#include "scope.h"
#include "map.h"
#include "vec.h"
#include "ident.h"

struct scope {
    struct scope *parent;

    // map[struct identifier*]void*
    // Owns the value pointed to by the void*.
    struct map *idents;

    // []struct scope*.
    // Owns the child scopes.
    struct vec *children;

    // []void*
    // Owns arbitrary pointers relating to the current scope.
    struct vec *owned;
};

struct scope *scope_new() {
    struct scope *s = calloc(1, sizeof(struct scope));
    s->idents = map_new(map_key_pointer);
    s->children = vec_new(sizeof(struct scope *));
    s->owned = vec_new(sizeof(void *));
    return s;
}

struct scope *scope_new_child(struct scope *s) {
    struct scope *child = scope_new();
    child->parent = s;

    vec_append(s->children, &child);

    return child;
}

void scope_declare(struct scope *s, struct ident *ident, void *value) {
    map_insert(s->idents, ident, value);
}

void *scope_get(struct scope *s, struct ident *ident) {
    void *value = map_get(s->idents, ident);
    if (value == NULL) {
        value = scope_get(s->parent, ident);
    }
    return value;
}

void scope_take_ownership(struct scope *s, void *ptr) {
    vec_append(s->owned, &ptr);
}
