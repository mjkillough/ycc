#include <stdlib.h>
#include <string.h>

#include "vec.h"

struct vec *vec_new(size_t elem_size) {
    size_t capacity = 1;
    struct vec *v = calloc(1, sizeof(struct vec));
    v->data = calloc(capacity, elem_size);
    v->count = 0;
    v->capacity = capacity;
    v->elem_size = elem_size;
    return v;
}

void vec_free(struct vec *v) {
    free(v->data);
    free(v);
}

static void *idx_ptr(struct vec *v, size_t idx) {
    return (char *)v->data + (idx * v->elem_size);
}

size_t vec_append(struct vec *v, void *elem) {
    // Grow capacity exponentially.
    if (v->count == v->capacity) {
        vec_reserve(v, v->capacity);
    }

    size_t idx = v->count++;
    void *ptr = idx_ptr(v, idx);
    memcpy(ptr, elem, v->elem_size);

    return idx;
}

void *vec_get(struct vec *v, size_t idx) {
    // TODO: Bounds check?
    return idx_ptr(v, idx);
}

void vec_reserve(struct vec *v, size_t extra_capacity) {
    v->capacity += extra_capacity;
    // TODO: check NULL return
    v->data = realloc(v->data, v->capacity * v->elem_size);
}

void vec_shrink(struct vec *v) {
    v->capacity = v->count;
    // TODO: check NULL return
    v->data = realloc(v->data, v->capacity * v->elem_size);
}

size_t vec_into_raw(struct vec *v, void **out) {
    vec_shrink(v);
    *out = v->data;
    size_t count = v->count;
    free(v);
    return count;
}

