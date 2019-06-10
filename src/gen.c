#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "gen.h"

// static bool gen_expr(FILE *f, ast_expr_t expr) {}

static bool gen_statement(FILE *f, ast_statement_t stmt) {
    fprintf(f, "movl $%s, %%eax\n", stmt.expr.str);
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
