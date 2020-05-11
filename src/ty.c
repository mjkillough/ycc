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
    case BasicTy_Int:
        return "int";
    }
}

static void ty_pprint_member(struct pprint *pp, struct ident *ident,
                             struct ty_member *member) {
    pprintf(pp, "(%s, ", ident_to_str(ident));
    ty_pprint(pp, member->ty);
    pprintf(pp, "),");
    pprint_newline(pp);
}

static bool ty_pprint_members_iter(void *context, const void *key,
                                   void *value) {
    struct pprint *pp = context;
    struct ident *ident = (void *)key;
    struct ty_member *member = value;

    ty_pprint_member(pp, ident, member);

    return true;
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

        map_iter(ty->members, pp, ty_pprint_members_iter);

        for (size_t i = 0; i < ty->nanonymous; i++) {
            pprintf(pp, "Anonymous");
            ty_pprint(pp, ty->anonymous[i].ty);
            pprintf(pp, ",");
            pprint_newline(pp);
        }

        pprint_unindent(pp);
        pprintf(pp, "])");
        break;
    }
}

bool ty_compatible(struct ty *a, struct ty *b) { return a->kind == b->kind; }

