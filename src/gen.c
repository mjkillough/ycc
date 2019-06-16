#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "gen.h"
#include "map.h"

typedef struct {
    map_t *env;
    size_t stack_idx;
} state_t;

static size_t var_idx(state_t *state, const char *ident) {
    return (size_t)map_get(state->env, ident);
}

static bool gen_expr(FILE *f, state_t *state, ast_expr_t *expr) {
    switch (expr->discrim) {
    case Ast_Expr_Constant:
        fprintf(f, "mov $%s, %%eax\n", expr->str);
        break;
    case Ast_Expr_Var:
        fprintf(f, "mov -%zu(%%ebp), %%eax\n", var_idx(state, expr->str));
        break;
    case Ast_Expr_BinOp:
        gen_expr(f, state, expr->rhs);
        fprintf(f, "push %%eax\n");
        gen_expr(f, state, expr->lhs);
        fprintf(f, "pop %%ecx\n");

        switch (expr->binop) {
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
        case Ast_BinOp_Equal:
            fprintf(f, "cmp %%ecx, %%eax\n");
            fprintf(f, "mov $0, %%eax\n");
            fprintf(f, "sete %%al\n");
            break;
        case Ast_BinOp_NotEqual:
            fprintf(f, "cmp %%ecx, %%eax\n");
            fprintf(f, "mov $0, %%eax\n");
            fprintf(f, "setne %%al\n");
            break;
        case Ast_BinOp_LessThan:
            fprintf(f, "cmp %%ecx, %%eax\n");
            fprintf(f, "mov $0, %%eax\n");
            fprintf(f, "setl %%al\n");
            break;
        case Ast_BinOp_LessThanEqual:
            fprintf(f, "cmp %%ecx, %%eax\n");
            fprintf(f, "mov $0, %%eax\n");
            fprintf(f, "setle %%al\n");
            break;
        case Ast_BinOp_GreatherThan:
            fprintf(f, "cmp %%ecx, %%eax\n");
            fprintf(f, "mov $0, %%eax\n");
            fprintf(f, "setg %%al\n");
            break;
        case Ast_BinOp_GreaterThanEqual:
            fprintf(f, "cmp %%ecx, %%eax\n");
            fprintf(f, "mov $0, %%eax\n");
            fprintf(f, "setge %%al\n");
            break;
        }
        break;
    case Ast_Expr_UnOp:
        gen_expr(f, state, expr->inner);
        fprintf(f, "neg %%eax\n");
        break;
    }
    return true;
}

static bool gen_statement(FILE *f, state_t *state, ast_statement_t *stmt) {
    switch (stmt->kind) {
    case Ast_Statement_Return:
        gen_expr(f, state, stmt->expr);
        fprintf(f, "mov %%ebp, %%esp\n");
        fprintf(f, "pop %%ebp\n");
        fprintf(f, "ret\n");
        break;
    case Ast_Statement_Decl:
        gen_expr(f, state, stmt->expr);
        fprintf(f, "push %%eax\n");
        map_insert(state->env, stmt->identifier, (void *)state->stack_idx);
        state->stack_idx += 4;
        break;
    }
    return true;
}

static bool gen_block(FILE *f, state_t *state, ast_block_t *block) {
    for (size_t i = 0; i < block->count; i++) {
        gen_statement(f, state, &block->stmts[i]);
    }
    return true;
}

static bool gen_function(FILE *f, ast_function_t *func) {
    state_t state = {
        .env = map_new(),
        .stack_idx = 4,
    };
    fprintf(f, " .globl %s\n", func->name);
    fprintf(f, "%s:\n", func->name);
    fprintf(f, "push %%ebp\n");
    fprintf(f, "mov %%esp, %%ebp\n");
    return gen_block(f, &state, &func->block);
}

static bool gen_program(FILE *f, ast_program_t *prog) {
    return gen_function(f, &prog->function);
}

bool gen_generate(FILE *f, ast_program_t ast) { return gen_program(f, &ast); }
