#include "ty.h"
#include "pprint.h"

void ty_pprint(struct pprint *pp, struct ty *ty) {
    if (ty->kind == Ty_Pointer) {
        ty_pprint(pp, ty->inner);
        pprintf(pp, "*");
    } else {
        pprintf(pp, "int");
    }
}

