#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "gen.h"

static bool gen_expr(FILE *f, ast_expr_t expr) {
    switch (expr.discrim) {
    case Ast_Expr_Constant:
        fprintf(f, "mov $%s, %%eax\n", expr.str);
        break;
    case Ast_Expr_BinOp:
        gen_expr(f, *expr.rhs);
        fprintf(f, "push %%eax\n");
        gen_expr(f, *expr.lhs);
        fprintf(f, "pop %%ecx\n");

        switch (expr.binop) {
        case Ast_BinOp_Addition:
            fprintf(f, "add %%ecx, %%eax\n");
            break;
        case Ast_BinOp_Subtraction:
            fprintf(f, "sub %%ecx, %%eax\n");
            break;
        case Ast_BinOp_Multiplication:
            fprintf(f, "imul %%ecx, %%eax\n");
            break;
        case Ast_BinOp_Division:
            fprintf(f, "mov $0, %%edx\n");
            fprintf(f, "idiv %%ecx\n");
            break;
        }
        break;
    }
    return true;
}

static bool gen_statement(FILE *f, ast_statement_t stmt) {
    gen_expr(f, stmt.expr);
    fprintf(f, "ret\n");
    return true;
}

static bool gen_function(FILE *f, ast_function_t func) {
    fprintf(f, " .globl %s\n", func.name);
    fprintf(f, "%s:\n", func.name);
    return gen_statement(f, func.statement);
}

static bool gen_program(FILE *f, ast_program_t prog) {
    return gen_function(f, prog.function);
}

bool gen_generate(FILE *f, ast_program_t ast) { return gen_program(f, ast); }
