#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "diag.h"
#include "map.h"
#include "ty.h"
#include "vec.h"

struct scope {
    struct scope *parent;

    // map[struct identifier*]void*
    // Owns the value pointed to by the void*.
    struct map *idents;

    // []struct scope*.
    // Owns the child scopes.
    struct vec *children;

    // []void*
    // Owns arbitrary pointers relating to the current scope.
    struct vec *owned;
};

struct scope *scope_new() {
    struct scope *s = calloc(1, sizeof(struct scope));
    s->idents = map_new(map_key_pointer);
    s->children = vec_new(sizeof(struct scope *));
    s->owned = vec_new(sizeof(void *));
    return s;
}

struct scope *scope_new_child(struct scope *s) {
    struct scope *child = scope_new();
    child->parent = s;

    vec_append(s->children, &child);

    return child;
}

void scope_declare(struct scope *s, struct ident *ident, void *value) {
    map_insert(s->idents, ident, value);
}

void *scope_get(struct scope *s, struct ident *ident) {
    void *value = map_get(s->idents, ident);
    if (value == NULL) {
        value = scope_get(s->parent, ident);
    }
    return value;
}

void scope_take_ownership(struct scope *s, void *ptr) {
    vec_append(s->owned, &ptr);
}

struct tycheck {
    struct {
        struct scope *tags;
    } namespaces;
};

struct tycheck *tycheck_new() {
    struct tycheck *tyc = malloc(sizeof(struct tycheck));
    tyc->namespaces.tags = scope_new();
    return tyc;
}

void tycheck_free(struct tycheck *tyc) {
    // TODO: free scope
    // scope_free(tyc->scope);
    free(tyc);
}

static struct ty *ty_from_ast_basic(enum ast_basic_type ast_basic) {
    enum basic_ty basic;

    switch (ast_basic) {
    case Ast_BasicType_Int:
        basic = BasicTy_Int;
        break;
    }

    struct ty *ty = malloc(sizeof(struct ty));
    ty->kind = Ty_Basic;
    ty->basic = basic;

    return ty;
}

static void tycheck_struct_declarator(struct tycheck *tyc, struct map *members,
                                      struct ty *ty,
                                      struct ast_declarator *decl) {
    UNUSED(tyc);

    for (size_t i = 0; i < decl->npointers; i++) {
        // TODO: type qualifiers
        struct ty *inner = ty;

        ty = malloc(sizeof(struct ty));
        ty->kind = Ty_Pointer;
        ty->inner = inner;
    }

    struct ty_member *member = malloc(sizeof(struct ty));
    member->ty = ty;

    map_insert(members, decl->ident, member);
}

static struct ty *tycheck_type(struct tycheck *tyc, struct ast_type *ast_ty);

static void tycheck_struct_declaration(struct tycheck *tyc, struct map *members,
                                       struct vec *anonymous,
                                       struct ast_struct_declaration *decl) {
    struct ty *ty = tycheck_type(tyc, &decl->type);

    // If the type is a struct without a tag and is missing an identifier, treat
    // it as an anonymous member of the current struct.
    if (ty->kind == Ty_Struct && ty->tag == NULL && decl->ndeclarators == 0) {
        struct ty_member member = {.ty = ty};
        vec_append(anonymous, &member);
    }

    for (size_t i = 0; i < decl->ndeclarators; i++) {
        tycheck_struct_declarator(tyc, members, ty, &decl->declarators[i]);
    }
}

static struct ty *tycheck_type_struct(struct tycheck *tyc,
                                      struct ast_type *ast_ty) {
    struct vec *anonymous = vec_new(sizeof(struct ty_member));
    struct map *members = map_new(map_key_pointer);
    for (size_t i = 0; i < ast_ty->ndeclarations; i++) {
        tycheck_struct_declaration(tyc, members, anonymous,
                                   &ast_ty->declarations[i]);
    }

    struct ty *ty = malloc(sizeof(struct ty));
    ty->kind = Ty_Struct;
    ty->tag = ast_ty->ident;
    ty->members = members;
    ty->nanonymous = vec_into_raw(anonymous, (void **)&ty->anonymous);

    // If it's tagged, we declare a new type with the given tag. If it's
    // untagged, we still pass ownership of the struct ty* to the current
    // scope, but as it's not accessible it can't be used by anyone else.
    struct scope *scope = tyc->namespaces.tags;
    if (ty->tag == NULL) {
        scope_take_ownership(scope, ty);
    } else {
        scope_declare(scope, ty->tag, ty);
    }

    return ty;
}

static struct ty *tycheck_type(struct tycheck *tyc, struct ast_type *ast_ty) {

    switch (ast_ty->kind) {
    case Ast_Type_BasicType:
        return ty_from_ast_basic(ast_ty->basic);

    case Ast_Type_Struct:
        return tycheck_type_struct(tyc, ast_ty);
    }
}

void tycheck_declaration(struct tycheck *tyc, struct ast_declaration *decl) {
    UNUSED(tyc);

    struct ty *ty = tycheck_type(tyc, &decl->type);

    struct pprint *pp = pprint_new(stdout);
    pprintf(pp, "---\n");
    ast_pprint_declaration(pp, decl);
    pprintf(pp, "\n");
    ty_pprint(pp, ty);
    pprintf(pp, "\n");
    pprint_free(pp);
}

static void tycheck_statement(struct tycheck *tyc, ast_statement_t *stmt) {
    UNUSED(tyc);
    UNUSED(stmt);
}

static void tycheck_function(struct tycheck *tyc, ast_function_t *func) {
    // TODO: enter functon scope.

    for (size_t i = 0; i < func->block.nitems; i++) {
        struct ast_block_item *item = &func->block.items[i];

        switch (item->kind) {
        case Ast_BlockItem_Declaration:
            tycheck_declaration(tyc, &item->decl);
            break;
        case Ast_BlockItem_Statement:
            tycheck_statement(tyc, &item->stmt);
            break;
        }
    }
}

static bool _tycheck_program_iter(void *context, const void *key, void *value) {
    UNUSED(key);

    struct tycheck *tyc = (struct tycheck *)context;
    ast_function_t *func = (ast_function_t *)value;

    tycheck_function(tyc, func);

    return true;
}

void tycheck_check(struct tycheck *tyc, ast_program_t *prog) {
    map_iter(prog->functions, tyc, &_tycheck_program_iter);
}

