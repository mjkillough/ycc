#pragma once

#include <stdlib.h>

#include "map.h"
#include "pprint.h"
#include "ty.h"

size_t alignment_padding(size_t base, size_t alignment);

struct layout {
    char alignment;
    size_t size;

    struct layout_member *members;
    size_t nmembers;

    // map[struct ident*]struct layout_member*
    struct map *lookup;
};

struct layout_member {
    struct ident *ident;
    size_t offset;
    struct layout *layout;
};

struct layout *layout_ty(struct ty *ty);

void layout_pprint(struct pprint *pp, struct layout *layout);

