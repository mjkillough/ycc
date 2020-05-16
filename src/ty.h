#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "map.h"
#include "pprint.h"

enum basic_ty {
    BasicTy_Char,

    // Signed Integers:
    // BasicTy_SignedChar,
    BasicTy_ShortInt,
    BasicTy_Int,
    BasicTy_LongInt,
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

            // Owns descriptions of all members in the order that they are
            // defined. The full type (i.e. struct/union type) are included for
            // layout purposes.
            struct ty_member *members;
            size_t nmembers;

            // map[struct ident*]struct ty_member*
            // Non-owning. Allows efficient lookup of type for a member's ident
            // which is useful for type-checking. Anonymous members' members are
            // inlined into this lookup.
            struct map *lookup;
        };
    };
};

struct ty_member {
    bool anonymous;
    // Optional:
    struct ident *ident;
    struct ty *ty;
    // span
};

void ty_pprint(struct pprint *pp, struct ty *ty);
bool ty_compatible(struct ty *a, struct ty *b);

