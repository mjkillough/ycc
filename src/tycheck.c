#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "diag.h"
#include "map.h"
#include "ty.h"

struct tycheck {
    // map[string]struct ty
    struct map *decls;
    diag_t *diag;
};

/* map *map_new(); */
/* void map_insert(map *m, const char *key, void *ptr); */
/* void *map_get(map *m, const char *key); */
/* void map_remove(map *m, const char *key); */

struct tycheck *tycheck_new() {
    struct tycheck *tyc = (struct tycheck *)malloc(sizeof(struct tycheck));
    tyc->decls = map_new();
    return tyc;
}

void tycheck_free(struct tycheck *tyc) { free(tyc); }

static void tycheck_statement(struct tycheck *tyc, ast_statement_t *stmt) {
    UNUSED(tyc);
    UNUSED(stmt);

    /* switch (stmt->kind) { */
    /* case Ast_Statement_Decl: */
    /*     // TODO: Check for redefinition. */
    /*     // TODO: Check type of expr. */
    /*     map_insert(tyc->decls, stmt->decl->identifier, &stmt->decl->ty); */
    /*     break; */
    /* case Ast_Statement_Expr: */
    /*     // TODO: Actually check type of expr too. */
    /*     if (stmt->expr->discrim == Ast_Expr_AssignOp) { */
    /*         // XXX: Assume lhs and rhs are Var() */
    /*         if (stmt->expr->lhs->discrim == Ast_Expr_Var && */
    /*             stmt->expr->rhs->discrim == Ast_Expr_Var) { */
    /*             const char *ident1 = stmt->expr->lhs->str; */
    /*             const char *ident2 = stmt->expr->rhs->str; */

    /*             struct ty *ty1 = (struct ty *)map_get(tyc->decls, ident1); */
    /*             struct ty *ty2 = (struct ty *)map_get(tyc->decls, ident2); */
    /*             if (ty1 != NULL && ty2 != NULL) { */
    /*                 if (!ty_compatible(ty1, ty2)) { */
    /*                     printf("incompatible types!\n"); */
    /*                 } */
    /*             } */
    /*         } */
    /*     } */
    /*     break; */
    /* } */
}

static void tycheck_function(struct tycheck *tyc, ast_function_t *func) {
    for (size_t i = 0; i < func->block.nitems; i++) {
        // XXX: Declaration or Statement!
        tycheck_statement(tyc, &func->block.items[i].stmt);
    }
}

static bool _tycheck_program_iter(void *context, const char *key, void *value) {
    UNUSED(key);

    struct tycheck *tyc = (struct tycheck *)context;
    ast_function_t *func = (ast_function_t *)value;

    tycheck_function(tyc, func);

    return true;
}

void tycheck_check(struct tycheck *tyc, ast_program_t *prog) {
    map_iter(prog->functions, tyc, &_tycheck_program_iter);
}

