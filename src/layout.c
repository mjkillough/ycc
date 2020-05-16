#include <stdlib.h>

#include "ident.h"
#include "layout.h"
#include "map.h"
#include "pprint.h"
#include "ty.h"
#include "vec.h"

size_t alignment_padding(size_t base, size_t alignment) {
    size_t delta = (base % alignment);
    if (delta == 0) {
        return 0;
    }
    return alignment - delta;
}

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

static void layout_pprint_member(struct pprint *pp,
                                 struct layout_member *member) {
    pprintf(pp, "(%s, %zu, ", ident_to_str(member->ident), member->offset);
    layout_pprint(pp, member->layout);
    pprintf(pp, ")");
}

void layout_pprint(struct pprint *pp, struct layout *layout) {
    pprintf(pp, "Layout(alignment = %zu, size = %zu", layout->alignment,
            layout->size);

    if (layout->nmembers == 0) {
        pprintf(pp, ")");
        return;
    }

    pprintf(pp, ", ");
    if (layout->nmembers > 1) {
        pprintf(pp, "[");
        pprint_newline(pp);
        pprint_indent(pp);
    }

    for (size_t i = 0; i < layout->nmembers; i++) {
        layout_pprint_member(pp, &layout->members[i]);

        if (layout->nmembers > 1) {
            pprintf(pp, ",");
            pprint_newline(pp);
        }
    }

    if (layout->nmembers > 1) {
        pprint_unindent(pp);
        pprintf(pp, "]");
    }
    pprintf(pp, ")");
    pprint_newline(pp);
}

static struct layout *layout_ty_struct(struct ty *ty) {
    char alignment = 0;
    size_t size = 0;

    struct vec *members = vec_new(sizeof(struct layout_member));
    for (size_t i = 0; i < ty->nmembers; i++) {
        struct layout *layout = layout_ty(ty->members[i].ty);

        size += alignment_padding(size, layout->alignment);
        struct layout_member member = {
            .ident = ty->members[i].ident,
            .offset = size,
            .layout = layout,
        };
        vec_append(members, &member);

        size += layout->size;

        // Align the struct to the max alignment of its members.
        if (layout->alignment > alignment) {
            alignment = layout->alignment;
        }
    }

    // Add padding at the end so that we're a multiple of our alignment
    // (stride). This will allow us to have an array of structs and have each
    // aligned properly.
    size += alignment_padding(size, alignment);

    struct layout *layout = malloc(sizeof(layout));
    layout->alignment = alignment;
    layout->size = size;
    layout->nmembers = vec_into_raw(members, (void **)&layout->members);
    layout->lookup = map_new(map_key_pointer);

    return layout;
}

static struct layout *layout_ty_union(struct ty *ty) {
    char alignment = 0;
    size_t size = 0;

    struct vec *members = vec_new(sizeof(struct layout_member));
    for (size_t i = 0; i < ty->nmembers; i++) {
        struct layout *layout = layout_ty(ty->members[i].ty);

        struct layout_member member = {
            .ident = ty->members[i].ident,
            .offset = 0,
            .layout = layout,
        };
        vec_append(members, &member);

        // Size the struct to the max size of its members.
        if (layout->size > size) {
            size = layout->size;
        }

        // Align the struct to the max alignment of its members.
        if (layout->alignment > alignment) {
            alignment = layout->alignment;
        }
    }

    // Add padding at the end so that we're a multiple of our alignment
    // (stride). This will allow us to have an array of structs and have each
    // aligned properly.
    size += alignment_padding(size, alignment);

    struct layout *layout = malloc(sizeof(layout));
    layout->alignment = alignment;
    layout->size = size;
    layout->nmembers = vec_into_raw(members, (void **)&layout->members);
    layout->lookup = map_new(map_key_pointer);

    return layout;
}

static struct layout *layout_basic_ty(enum basic_ty ty) {
    struct layout *layout = malloc(sizeof(struct layout));
    layout->nmembers = 0;

    switch (ty) {
    case BasicTy_Char:
        layout->alignment = 1;
        layout->size = 1;
        break;
    case BasicTy_ShortInt:
        layout->alignment = 2;
        layout->size = 2;
        break;
    case BasicTy_Int:
        layout->alignment = 4;
        layout->size = 4;
        break;
    case BasicTy_LongInt:
        layout->alignment = 8;
        layout->size = 8;
        break;
    }

    return layout;
}

struct layout *layout_ty(struct ty *ty) {
    switch (ty->kind) {
    case Ty_Basic:
        return layout_basic_ty(ty->basic);

    case Ty_Struct:
        return layout_ty_struct(ty);

    case Ty_Union:
        return layout_ty_union(ty);
    }
}

