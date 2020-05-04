#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "diag.h"
#include "map.h"
#include "pprint.h"
#include "ty.h"

void ty_pprint(struct pprint *pp, struct ty *ty) {
    if (ty->kind == Ty_Pointer) {
        ty_pprint(pp, ty->inner);
        pprintf(pp, "*");
    } else {
        pprintf(pp, "int");
    }
}

bool ty_compatible(struct ty *a, struct ty *b) { return a->kind == b->kind; }

