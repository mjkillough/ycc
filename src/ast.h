#pragma once

#include <stddef.h>
#include <stdio.h>

#include "map.h"
#include "pprint.h"
#include "ty.h"

typedef enum {
    Ast_BinOp_Addition,
    Ast_BinOp_Subtraction,
    Ast_BinOp_Multiplication,
    Ast_BinOp_Division,

    Ast_BinOp_Equal,
    Ast_BinOp_NotEqual,
    Ast_BinOp_LessThan,
    Ast_BinOp_LessThanEqual,
    Ast_BinOp_GreaterThan,
    Ast_BinOp_GreaterThanEqual,
} ast_binop_t;

typedef enum {
    Ast_UnOp_Negation,
    Ast_UnOp_AddressOf,
    Ast_UnOp_Deref,
} ast_unop_t;

typedef enum {
    Ast_AssignOp_Assign,
    Ast_AssignOp_Addition,
    Ast_AssignOp_Subtraction,
    Ast_AssignOp_Multiplication,
    Ast_AssignOp_Division,
} ast_assignop_t;

typedef struct ast_expr_t {
    enum {
        Ast_Expr_Constant,
        Ast_Expr_Var,
        Ast_Expr_UnOp,
        Ast_Expr_BinOp,
        Ast_Expr_AssignOp,
    } discrim;
    union {
        // Ast_Expr_Constant, Ast_Expr_Var:
        const char *str;
        // Ast_Expr_UnOp:
        ast_unop_t unop;
        // Ast_Expr_Binop:
        ast_binop_t binop;
        // Ast_Expr_AssignOp:
        ast_assignop_t assignop;
    };
    // Ast_Expr_UnOp, Ast_Expr_Binop, Ast_Expr_Assign:
    struct ast_expr_t *lhs, *rhs;
} ast_expr_t;

struct ast_block_t;

typedef struct ast_statement_t {
    enum {
        Ast_Statement_Return,
        Ast_Statement_If,
        Ast_Statement_Block,
        Ast_Statement_Expr,

    } kind;
    // Ast_Statement_Return, Ast_Statement_Decl, Ast_Statement_Expr,
    // Ast_Statement_If:
    ast_expr_t *expr;
    // Ast_Statement_If:
    struct ast_statement_t *arm1, *arm2;
    // Ast_Statement_Block:
    struct ast_block_t *block;
} ast_statement_t;

typedef struct ast_block_t {
    struct ast_block_item *items;
    size_t nitems;
} ast_block_t;

typedef struct {
    const char *name;
    ast_block_t block;
} ast_function_t;

typedef struct {
    map *functions; // ast_function_t
} ast_program_t;

void ast_pprint_expr(struct pprint *pp, ast_expr_t *expr);
void ast_pprint_statement(struct pprint *pp, ast_statement_t *statment);
void ast_pprint_block(struct pprint *pp, ast_block_t *block);
void ast_pprint_function(struct pprint *pp, ast_function_t *func);
void ast_pprint_program(struct pprint *pp, ast_program_t *prog);

enum ast_basic_type {
    Ast_BasicType_Int,
};

struct ast_declarator {
    enum {
        Ast_Declarator_Pointer,
        Ast_Declarator_Ident,
    } kind;
    union {
        // Ast_Declarator_Ident:
        const char *ident;
        // Ast_Declarator_Pointer:
        struct ast_declarator *next;
    };
};

struct ast_struct_declaration;

// This is more like `declaration-specifiers` in the grammar.
struct ast_type {
    enum {
        Ast_Type_BasicType,
        Ast_Type_Struct,
    } kind;
    union {
        // Ast_DeclSpecifier_BasicType:
        enum ast_basic_type basic;
        // Ast_DeclSpecifier_Struct:
        struct {
            // Identifier, optional:
            const char *ident;
            struct ast_struct_declaration *declarations;
            size_t ndeclarations;
        };
    };
};

// aka. a struct "field"
struct ast_struct_declaration {
    struct ast_type type;
    // Optional:
    struct ast_declarator *declarators;
    size_t ndeclarators;
};

struct ast_declaration {
    struct ast_type type;
    struct ast_declarator declarator;
    // Optional:
    ast_expr_t *expr;
};

void ast_pprint_declarator(struct pprint *pp, struct ast_declarator *decl);
void ast_pprint_struct_declaration(struct pprint *pp,
                                   struct ast_struct_declaration *decl);
void ast_pprint_type(struct pprint *pp, struct ast_type *ty);
void ast_pprint_declaration(struct pprint *pp, struct ast_declaration *decl);

struct ast_block_item {
    enum {
        Ast_BlockItem_Declaration,
        Ast_BlockItem_Statement,
    } kind;
    union {
        ast_statement_t stmt;
        struct ast_declaration decl;
    };
};

