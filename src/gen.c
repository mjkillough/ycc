#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "gen.h"
#include "ident.h"
#include "map.h"

typedef struct {
    struct map *env;
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
        switch (expr->unop) {
        case Ast_UnOp_Negation:
            gen_expr(f, state, expr->lhs);
            fprintf(f, "neg %%rax\n");
            break;
        case Ast_UnOp_AddressOf:
            if (expr->lhs->discrim != Ast_Expr_Var) {
                printf("error: can only take address of variables\n");
                exit(-1);
            }
            fprintf(f, "mov %%rbp, %%rax\n");
            fprintf(f, "sub $%zu, %%rax\n", var_idx(state, expr->lhs->str));
            break;
        case Ast_UnOp_Deref:
            if (expr->lhs->discrim != Ast_Expr_Var) {
                printf("error: can only take address of variables\n");
                exit(-1);
            }
            fprintf(f, "mov -%zu(%%rbp), %%rax\n",
                    var_idx(state, expr->lhs->str));
            fprintf(f, "mov (%%rax), %%rax\n");
            break;
        }
        break;
    case Ast_Expr_AssignOp:
        if (expr->lhs->discrim != Ast_Expr_Var) {
            printf("error: can only assign to variables\n");
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

static const char *_declarator_ident(struct ast_declarator *declarator) {
    switch (declarator->kind) {
    case Ast_Declarator_Pointer:
        return _declarator_ident(declarator->next);
    case Ast_Declarator_Ident:
        return ident_to_str(declarator->ident);
    }
}

static bool gen_statement(FILE *f, state_t *state, ast_statement_t *stmt) {
    switch (stmt->kind) {
    case Ast_Statement_Return:
        gen_expr(f, state, stmt->expr);
        fprintf(f, "mov %%rbp, %%rsp\n");
        fprintf(f, "popq %%rbp\n");
        fprintf(f, "ret\n");
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

    fprintf(f, "\n");

    return true;
}

static bool gen_declaration(FILE *f, state_t *state,
                            struct ast_declaration *decl) {
    gen_expr(f, state, decl->expr);
    fprintf(f, "pushq %%rax\n");
    map_insert(state->env, _declarator_ident(&decl->declarator),
               (void *)state->stack_idx);
    state->stack_idx += 8;
    return true;
}

static bool gen_block(FILE *f, state_t *state, ast_block_t *block) {
    for (size_t i = 0; i < block->nitems; i++) {
        switch (block->items[i].kind) {
        case Ast_BlockItem_Statement:
            gen_statement(f, state, &block->items[i].stmt);
            break;
        case Ast_BlockItem_Declaration:
            gen_declaration(f, state, &block->items[i].decl);
            break;
        }
    }

    return true;
}

static bool gen_function(FILE *f, ast_function_t *func) {
    state_t state = {
        .env = map_new(map_key_string),
        .stack_idx = 8,
    };
    fprintf(f, " .globl %s\n", ident_to_str(func->ident));
    fprintf(f, "%s:\n", ident_to_str(func->ident));
    fprintf(f, "pushq %%rbp\n");
    fprintf(f, "mov %%rsp, %%rbp\n");
    return gen_block(f, &state, &func->block);
}

static bool gen_function_iter(void *context, const void *key, void *value) {
    FILE *f = context;
    UNUSED(key);
    return gen_function(f, value);
}

static bool gen_program(FILE *f, ast_program_t *prog) {
    return map_iter(prog->functions, f, &gen_function_iter);
}

bool gen_generate(FILE *f, ast_program_t ast) { return gen_program(f, &ast); }
