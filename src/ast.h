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

struct decl {
    struct ty ty;
    const char *identifier;
    struct ast_expr_t *expr;
};

typedef struct ast_statement_t {
    enum {
        Ast_Statement_Return,
        Ast_Statement_Decl,
        Ast_Statement_If,
        Ast_Statement_Block,
        Ast_Statement_Expr,
    } kind;
    // Ast_Statement_Decl:
    struct decl *decl;
    // Ast_Statement_Return, Ast_Statement_Decl, Ast_Statement_Expr,
    // Ast_Statement_If:
    ast_expr_t *expr;
    // Ast_Statement_If:
    struct ast_statement_t *arm1, *arm2;
    // Ast_Statement_Block:
    struct ast_block_t *block;
} ast_statement_t;

typedef struct ast_block_t {
    ast_statement_t *stmts;
    size_t count;
    size_t capacity;
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

