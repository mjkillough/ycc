#pragma once

typedef enum {
    Ast_BinOp_Addition,
    Ast_BinOp_Multiplication,
} ast_binop_t;

typedef struct ast_expr_t {
    enum {
        Ast_Expr_Constant,
        Ast_Expr_BinOp,
    } discrim;
    union {
        // Ast_Expr_Constant:
        const char *str;
        // Ast_Expr_Addition, Ast_Expr_Multiplication:
        struct {
            ast_binop_t binop;
            struct ast_expr_t *lhs;
            struct ast_expr_t *rhs;
        };
    };
} ast_expr_t;

typedef struct {
    ast_expr_t expr;
} ast_statement_t;

typedef struct {
    const char *name;
    ast_statement_t statement;
} ast_function_t;

typedef struct {
    ast_function_t function;
} ast_program_t;

void ast_print_expr(ast_expr_t expr);
void ast_print_statement(ast_statement_t statment);
void ast_print_function(ast_function_t func);
void ast_print_program(ast_program_t prog);
