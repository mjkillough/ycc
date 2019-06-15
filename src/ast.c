#include <stdio.h>

#include "ast.h"

static const char *ast_print_expr_binop(ast_expr_t *expr) {
    switch (expr->binop) {
    case Ast_BinOp_Addition:
        return "+";
    case Ast_BinOp_Subtraction:
        return "-";
    case Ast_BinOp_Multiplication:
        return "*";
    case Ast_BinOp_Division:
        return "/";
    default:
        return "UNKNOWN_OP";
    }
}

void ast_print_expr(ast_expr_t *expr) {
    switch (expr->discrim) {
    case Ast_Expr_Constant:
        printf("%s", expr->str);
        break;
    case Ast_Expr_BinOp:
        printf("Expr(");
        ast_print_expr(expr->lhs);
        printf(" %s ", ast_print_expr_binop(expr));
        ast_print_expr(expr->rhs);
        printf(")");
        break;
    case Ast_Expr_UnOp:
        printf("Expr(Neg(");
        ast_print_expr(expr->inner);
        printf("))");
        break;
    }
}

void ast_print_statement(ast_statement_t *stmt) {
    printf("Statement(");

    switch (stmt->kind) {
    case Ast_Statement_Return:
        printf("Return(");
        ast_print_expr(stmt->expr);
        printf(")");
        break;
    case Ast_Statement_Decl:
        printf("Decl(int, %s, ", stmt->identifier);
        ast_print_expr(stmt->expr);
        printf(")");
        break;
    }

    printf(")");
}

void ast_print_block(ast_block_t *block) {
    printf("Block(");
    for (size_t i = 0; i < block->count; i++) {
        ast_print_statement(&block->stmts[i]);

        if (i < block->count - 1) {
            printf(", ");
        }
    }
    printf(")");
}

void ast_print_function(ast_function_t *func) {
    printf("Function(name=%s, ", func->name);
    ast_print_block(&func->block);
    printf(")");
}

void ast_print_program(ast_program_t *prog) {
    printf("Program(");
    ast_print_function(&prog->function);
    printf(")\n");
}
