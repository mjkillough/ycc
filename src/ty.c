#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "diag.h"
#include "ident.h"
#include "map.h"
#include "pprint.h"
#include "ty.h"

static const char *_basic(enum basic_ty ty) {
    switch (ty) {
    case BasicTy_Char:
        return "char";
    case BasicTy_ShortInt:
        return "short";
    case BasicTy_Int:
        return "int";
    case BasicTy_LongInt:
        return "long";
    }
}

static void ty_pprint_member(struct pprint *pp, struct ty_member *member) {
    if (member->anonymous) {
        pprintf(pp, "Anonymous");
        ty_pprint(pp, member->ty);
        pprintf(pp, ",");
        pprint_newline(pp);
        return;
    }

    pprintf(pp, "(%s, ", ident_to_str(member->ident));
    ty_pprint(pp, member->ty);
    pprintf(pp, "),");
    pprint_newline(pp);
}

void ty_pprint(struct pprint *pp, struct ty *ty) {
    switch (ty->kind) {
    case Ty_Incomplete:
        pprintf(pp, "INCOMPLETE");
        break;

    case Ty_Basic:
        pprintf(pp, _basic(ty->basic));
        break;

    case Ty_Pointer:
        ty_pprint(pp, ty->inner);
        pprintf(pp, "*");
        break;

    case Ty_Struct:
        pprintf(pp, "Struct([");
        pprint_newline(pp);
        pprint_indent(pp);

        for (size_t i = 0; i < ty->nmembers; i++) {
            ty_pprint_member(pp, &ty->members[i]);
        }

        pprint_unindent(pp);
        pprintf(pp, "])");
        break;
    }
}

bool ty_compatible(struct ty *a, struct ty *b) { return a->kind == b->kind; }

