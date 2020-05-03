#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "gen.h"
#include "map.h"

typedef struct {
    map *env;
    size_t stack_idx;
    uint64_t label_idx;
} state_t;

static size_t var_idx(state_t *state, const char *ident) {
    return (size_t)map_get(state->env, ident);
}

static bool gen_expr(FILE *f, state_t *state, ast_expr_t *expr) {
    switch (expr->discrim) {
    case Ast_Expr_Constant:
        fprintf(f, "mov $%s, %%rax\n", expr->str);
        break;
    case Ast_Expr_Var:
        fprintf(f, "mov -%zu(%%rbp), %%rax\n", var_idx(state, expr->str));
        break;
    case Ast_Expr_BinOp:
        gen_expr(f, state, expr->rhs);
        fprintf(f, "pushq %%rax\n");
        gen_expr(f, state, expr->lhs);
        fprintf(f, "popq %%rcx\n");

        switch (expr->binop) {
        case Ast_BinOp_Addition:
            fprintf(f, "add %%rcx, %%rax\n");
            break;
        case Ast_BinOp_Subtraction:
            fprintf(f, "sub %%rcx, %%rax\n");
            break;
        case Ast_BinOp_Multiplication:
            fprintf(f, "imul %%rcx, %%rax\n");
            break;
        case Ast_BinOp_Division:
            fprintf(f, "mov $0, %%rdx\n");
            fprintf(f, "idiv %%rcx\n");
            break;
        case Ast_BinOp_Equal:
            fprintf(f, "cmp %%rcx, %%rax\n");
            fprintf(f, "mov $0, %%rax\n");
            fprintf(f, "sete %%al\n");
            break;
        case Ast_BinOp_NotEqual:
            fprintf(f, "cmp %%rcx, %%rax\n");
            fprintf(f, "mov $0, %%rax\n");
            fprintf(f, "setne %%al\n");
            break;
        case Ast_BinOp_LessThan:
            fprintf(f, "cmp %%rcx, %%rax\n");
            fprintf(f, "mov $0, %%rax\n");
            fprintf(f, "setl %%al\n");
            break;
        case Ast_BinOp_LessThanEqual:
            fprintf(f, "cmp %%rcx, %%rax\n");
            fprintf(f, "mov $0, %%rax\n");
            fprintf(f, "setle %%al\n");
            break;
        case Ast_BinOp_GreaterThan:
            fprintf(f, "cmp %%rcx, %%rax\n");
            fprintf(f, "mov $0, %%rax\n");
            fprintf(f, "setg %%al\n");
            break;
        case Ast_BinOp_GreaterThanEqual:
            fprintf(f, "cmp %%rcx, %%rax\n");
            fprintf(f, "mov $0, %%rax\n");
            fprintf(f, "setge %%al\n");
            break;
        }
        break;
    case Ast_Expr_UnOp:
        gen_expr(f, state, expr->lhs);
        fprintf(f, "neg %%rax\n");
        break;
    case Ast_Expr_AssignOp:
        if (expr->lhs->discrim != Ast_Expr_Var) {
            printf("error: can only assign to variables");
            exit(-1);
        }

        gen_expr(f, state, expr->rhs);

        switch (expr->assignop) {
        case Ast_AssignOp_Assign:
            fprintf(f, "mov %%rax, ");
            break;
        case Ast_AssignOp_Addition:
            fprintf(f, "add %%rax, ");
            break;
        case Ast_AssignOp_Subtraction:
            fprintf(f, "sub %%rax, ");
            break;
        case Ast_AssignOp_Multiplication:
            fprintf(f, "mov %%rax, %%rcx\n");
            fprintf(f, "mov -%zu(%%rbp), %%rax\n",
                    var_idx(state, expr->lhs->str));
            fprintf(f, "imul %%rcx, %%rax\n");
            fprintf(f, "mov %%rax, ");
            break;
        case Ast_AssignOp_Division:
            fprintf(f, "mov %%rax, %%rcx\n");
            fprintf(f, "mov $0, %%rdx\n");
            fprintf(f, "mov -%zu(%%rbp), %%rax\n",
                    var_idx(state, expr->lhs->str));
            fprintf(f, "idiv %%rcx\n");
            fprintf(f, "mov %%rax, ");
            break;
        }

        fprintf(f, "-%zu(%%rbp)\n", var_idx(state, expr->lhs->str));
    }
    return true;
}

static bool gen_block(FILE *f, state_t *state, ast_block_t *block);

static bool gen_statement(FILE *f, state_t *state, ast_statement_t *stmt) {
    switch (stmt->kind) {
    case Ast_Statement_Return:
        gen_expr(f, state, stmt->expr);
        fprintf(f, "mov %%rbp, %%rsp\n");
        fprintf(f, "popq %%rbp\n");
        fprintf(f, "ret\n");
        break;
    case Ast_Statement_Decl:
        gen_expr(f, state, stmt->expr);
        fprintf(f, "pushq %%rax\n");
        map_insert(state->env, stmt->identifier, (void *)state->stack_idx);
        state->stack_idx += 8;
        break;
    case Ast_Statement_If:
        gen_expr(f, state, stmt->expr);
        fprintf(f, "cmp $0, %%rax\n");
        size_t end_label = state->label_idx++;
        if (stmt->arm2 != NULL) {
            size_t else_label = state->label_idx++;
            fprintf(f, "je if_%zu\n", else_label);
            gen_statement(f, state, stmt->arm1);
            fprintf(f, "je if_%zu\n", end_label);
            fprintf(f, "if_%zu:\n", else_label);
            gen_statement(f, state, stmt->arm2);
        } else {
            fprintf(f, "je if_%zu\n", end_label);
            gen_statement(f, state, stmt->arm1);
        }
        fprintf(f, "if_%zu:\n", end_label);
        break;
    case Ast_Statement_Block:
        gen_block(f, state, stmt->block);
        break;
    case Ast_Statement_Expr:
        gen_expr(f, state, stmt->expr);
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
        .stack_idx = 8,
    };
    fprintf(f, " .globl %s\n", func->name);
    fprintf(f, "%s:\n", func->name);
    fprintf(f, "pushq %%rbp\n");
    fprintf(f, "mov %%rsp, %%rbp\n");
    return gen_block(f, &state, &func->block);
}

static bool gen_function_iter(void *context, const char *key, void *value) {
    FILE *f = context;
    UNUSED(key);
    return gen_function(f, value);
}

static bool gen_program(FILE *f, ast_program_t *prog) {
    return map_iter(prog->functions, f, &gen_function_iter);
}

bool gen_generate(FILE *f, ast_program_t ast) { return gen_program(f, &ast); }
