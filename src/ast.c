#include <stdio.h>

#include "ast.h"

void ast_print_expr(ast_expr_t expr) {
    switch (expr.discrim) {
    case Ast_Expr_Constant:
        printf("Expr(%s)", expr.str);
        break;
    case Ast_Expr_Addition:
        printf("Expr(%s + ", expr.str);
        ast_print_expr(*expr.rhs);
        printf(")");
        break;
    }
}

void ast_print_statement(ast_statement_t statment) {
    printf("Statement(");
    ast_print_expr(statment.expr);
    printf(")");
}

void ast_print_function(ast_function_t func) {
    printf("Function(name=%s, ", func.name);
    ast_print_statement(func.statement);
    printf(")");
}

void ast_print_program(ast_program_t prog) {
    printf("Program(");
    ast_print_function(prog.function);
    printf(")\n");
}
