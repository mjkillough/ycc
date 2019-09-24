#pragma once

#include "map.h"

typedef enum {
    Ast_BinOp_Addition,
    Ast_BinOp_Subtraction,
    Ast_BinOp_Multiplication,
    Ast_BinOp_Division,

    Ast_BinOp_Equal,
    Ast_BinOp_NotEqual,
    Ast_BinOp_LessThan,
    Ast_BinOp_LessThanEqual,
    Ast_BinOp_GreatherThan,
    Ast_BinOp_GreaterThanEqual,
} ast_binop_t;

typedef enum {
    Ast_UnOp_Negation,
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
        Ast_Statement_Decl,
        Ast_Statement_If,
        Ast_Statement_Block,
        Ast_Statement_Expr,
    } kind;
    // Ast_Statement_Decl:
    const char *identifier;
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

void ast_print_expr(ast_expr_t *expr);
void ast_print_statement(ast_statement_t *statment);
void ast_print_function(ast_function_t *func);
void ast_print_program(ast_program_t *prog);
