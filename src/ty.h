#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "map.h"
#include "pprint.h"

enum basic_ty {
    // BasicTy_Char,

    // Signed Integers:
    // BasicTy_SignedChar,
    // BasicTy_ShortInt,
    BasicTy_Int,
    // BasicTy_LongInt,
    // BasicTy_LongLongInt,

    // Unsigned Integers:
    // BasicTy_UnsignedChar,
    // BasicTy_UnsignedShortInt,
    // BasicTy_UnsignedInt,
    // BasicTy_UnsignedLongInt,
    // BasicTy_UnsignedLongLongInt,

    // BasicTy_Bool
};

struct ty_member;

struct ty {
    enum {
        Ty_Basic,
        Ty_Pointer,
        Ty_Struct,
        Ty_Incomplete,
    } kind;
    union {
        enum basic_ty basic;
        // Ty_Pointer:
        struct ty *inner;
        // Ty_Struct:
        struct {
            // Optional:
            struct ident *tag;

            // map[struct ident*]struct ty_member*
            // Owns descriptions of its members.
            struct map *members;

            // Owns descriptions of anonymous members.
            struct ty_member *anonymous;
            size_t nanonymous;
        };
    };
};

struct ty_member {
    struct ty *ty;
    // span
};

void ty_pprint(struct pprint *pp, struct ty *ty);
bool ty_compatible(struct ty *a, struct ty *b);

