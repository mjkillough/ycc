#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "diag.h"
#include "ident.h"
#include "map.h"
#include "scope.h"
#include "ty.h"
#include "vec.h"

struct tycheck {
    struct {
        struct scope *tags;
        struct scope *ordinary;
    } namespaces;
};

struct tycheck *tycheck_new() {
    struct tycheck *tyc = malloc(sizeof(struct tycheck));
    tyc->namespaces.tags = scope_new();
    tyc->namespaces.ordinary = scope_new();
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
    case Ast_BasicType_Char:
        basic = BasicTy_Char;
        break;
    case Ast_BasicType_Short:
        basic = BasicTy_ShortInt;
        break;
    case Ast_BasicType_Int:
        basic = BasicTy_Int;
        break;
    case Ast_BasicType_Long:
        basic = BasicTy_LongInt;
        break;
    }

    struct ty *ty = malloc(sizeof(struct ty));
    ty->kind = Ty_Basic;
    ty->basic = basic;

    return ty;
}

static void tycheck_struct_declarator(struct tycheck *tyc, struct vec *members,
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

    struct ty_member member = {
        .anonymous = false,
        .ident = decl->ident,
        .ty = ty,
    };
    vec_append(members, &member);
}

static struct ty *tycheck_type(struct tycheck *tyc, struct ast_type *ast_ty);

static void tycheck_struct_declaration(struct tycheck *tyc, struct vec *members,
                                       struct ast_struct_declaration *decl) {
    struct ty *ty = tycheck_type(tyc, &decl->type);

    // If the type is a struct without a tag and is missing an identifier, treat
    // it as an anonymous member of the current struct.
    if (ty->kind == Ty_Struct && ty->tag == NULL && decl->ndeclarators == 0) {
        struct ty_member anonymous = {
            .anonymous = true,
            .ident = NULL,
            .ty = ty,
        };
        vec_append(members, &anonymous);
    }

    for (size_t i = 0; i < decl->ndeclarators; i++) {
        tycheck_struct_declarator(tyc, members, ty, &decl->declarators[i]);
    }
}

static void tycheck_struct_construct_lookup(struct tycheck *tyc,
                                            struct map *lookup, struct ty *ty) {
    // Construct a look-up from each member's ident to its type, including the
    // members of any anonymous members (or anonymous members' members, etc.)
    for (size_t i = 0; i < ty->nmembers; i++) {
        struct ty_member *member = &ty->members[i];

        if (member->anonymous) {
            tycheck_struct_construct_lookup(tyc, lookup, member->ty);
        } else if (member->ident != NULL) {
            // A tagged member without a declarator (i.e. with ident == NULL) is
            // just a declaration and shouldn't be included in the lookup.

            // TODO: Check that the member doesn't already exist and error.
            map_insert(lookup, member->ident, member);
        }
    }
}

static size_t alignment_padding(size_t base, size_t alignment) {
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

static void layout_pprint(struct pprint *pp, struct layout *layout);

static void layout_pprint_member(struct pprint *pp,
                                 struct layout_member *member) {
    pprintf(pp, "(%s, %zu, ", ident_to_str(member->ident), member->offset);
    layout_pprint(pp, member->layout);
    pprintf(pp, ")");
}

static void layout_pprint(struct pprint *pp, struct layout *layout) {
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

static struct layout *layout_ty(struct ty *ty);

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

static struct layout *layout_ty(struct ty *ty) {
    switch (ty->kind) {
    case Ty_Basic:
        return layout_basic_ty(ty->basic);

    case Ty_Struct:
        return layout_ty_struct(ty);
    }
}

static struct ty *
tycheck_type_struct(struct tycheck *tyc, struct ast_type *ast_ty) {
    struct vec *members = vec_new(sizeof(struct ty_member));
    for (size_t i = 0; i < ast_ty->ndeclarations; i++) {
        tycheck_struct_declaration(tyc, members, &ast_ty->declarations[i]);
    }

    struct ty *ty = malloc(sizeof(struct ty));
    ty->kind = Ty_Struct;
    ty->tag = ast_ty->ident;
    ty->nmembers = vec_into_raw(members, (void **)&ty->members);
    ty->lookup = map_new(map_key_pointer);

    tycheck_struct_construct_lookup(tyc, ty->lookup, ty);

    // If it's tagged, we declare a new type with the given tag. If it's
    // untagged, we still pass ownership of the struct ty* to the current
    // scope, but as it's not accessible it can't be used by anyone else.
    struct scope *scope = tyc->namespaces.tags;
    if (ty->tag == NULL) {
        scope_take_ownership(scope, ty);
    } else {
        scope_declare(scope, ty->tag, ty);
    }

    struct layout *layout = layout_ty(ty);
    struct pprint *pp = pprint_new(stdout);
    layout_pprint(pp, layout);

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

static void tycheck_declarator(struct tycheck *tyc, struct ty *ty,
                               struct ast_declarator *decl) {
    for (size_t i = 0; i < decl->npointers; i++) {
        // TODO: type qualifiers
        struct ty *inner = ty;

        ty = malloc(sizeof(struct ty));
        ty->kind = Ty_Pointer;
        ty->inner = inner;
    }

    scope_declare(tyc->namespaces.ordinary, decl->ident, ty);
}

void tycheck_declaration(struct tycheck *tyc, struct ast_declaration *decl) {
    struct ty *ty = tycheck_type(tyc, &decl->type);

    for (size_t i = 0; i < decl->ndeclarators; i++) {
        tycheck_declarator(tyc, ty, &decl->declarators[i]);

        // TODO: type check expr if it exists
    }

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

