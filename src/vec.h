#pragma once

#include <stdlib.h>

struct vec {
    void **data;
    size_t count;
    size_t capacity;
    size_t elem_size;
};

struct vec *vec_new(size_t elem_size);
void vec_free(struct vec *v);

size_t vec_append(struct vec *v, void *elem);
void *vec_get(struct vec *v, size_t idx);

void vec_reserve(struct vec *v, size_t extra_capacity);
void vec_shrink(struct vec *v);
size_t vec_into_raw(struct vec *v, void **out);

