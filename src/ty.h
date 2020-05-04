#pragma once

#include <stdbool.h>

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

struct ty {
    enum {
        Ty_Basic,
        Ty_Pointer,
    } kind;
    union {
        enum basic_ty basic;
        // Ty_Pointer:
        struct ty *inner;
    };
};

void ty_pprint(struct pprint *pp, struct ty *ty);
bool ty_compatible(struct ty *a, struct ty *b);

