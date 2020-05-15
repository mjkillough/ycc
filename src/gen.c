#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "gen.h"
#include "ident.h"
#include "map.h"

struct state {
    FILE *f;
    struct map *env;
    size_t stack_idx;
    uint64_t label_idx;
};

static size_t
var_idx(struct state *s, const struct ident *ident) {
    return (size_t)map_get(s->env, ident);
}

static bool gen_expr(struct state *s, ast_expr_t *expr) {
    switch (expr->discrim) {
    case Ast_Expr_Constant:
        fprintf(s->f, "mov $%s, %%rax\n", expr->str);
        break;
    case Ast_Expr_Var:
        fprintf(s->f, "mov -%zu(%%rbp), %%rax\n", var_idx(s, expr->ident));
        break;
    case Ast_Expr_BinOp:
        gen_expr(s, expr->rhs);
        fprintf(s->f, "pushq %%rax\n");
        gen_expr(s, expr->lhs);
        fprintf(s->f, "popq %%rcx\n");

        switch (expr->binop) {
        case Ast_BinOp_Addition:
            fprintf(s->f, "add %%rcx, %%rax\n");
            break;
        case Ast_BinOp_Subtraction:
            fprintf(s->f, "sub %%rcx, %%rax\n");
            break;
        case Ast_BinOp_Multiplication:
            fprintf(s->f, "imul %%rcx, %%rax\n");
            break;
        case Ast_BinOp_Division:
            fprintf(s->f, "mov $0, %%rdx\n");
            fprintf(s->f, "idiv %%rcx\n");
            break;
        case Ast_BinOp_Equal:
            fprintf(s->f, "cmp %%rcx, %%rax\n");
            fprintf(s->f, "mov $0, %%rax\n");
            fprintf(s->f, "sete %%al\n");
            break;
        case Ast_BinOp_NotEqual:
            fprintf(s->f, "cmp %%rcx, %%rax\n");
            fprintf(s->f, "mov $0, %%rax\n");
            fprintf(s->f, "setne %%al\n");
            break;
        case Ast_BinOp_LessThan:
            fprintf(s->f, "cmp %%rcx, %%rax\n");
            fprintf(s->f, "mov $0, %%rax\n");
            fprintf(s->f, "setl %%al\n");
            break;
        case Ast_BinOp_LessThanEqual:
            fprintf(s->f, "cmp %%rcx, %%rax\n");
            fprintf(s->f, "mov $0, %%rax\n");
            fprintf(s->f, "setle %%al\n");
            break;
        case Ast_BinOp_GreaterThan:
            fprintf(s->f, "cmp %%rcx, %%rax\n");
            fprintf(s->f, "mov $0, %%rax\n");
            fprintf(s->f, "setg %%al\n");
            break;
        case Ast_BinOp_GreaterThanEqual:
            fprintf(s->f, "cmp %%rcx, %%rax\n");
            fprintf(s->f, "mov $0, %%rax\n");
            fprintf(s->f, "setge %%al\n");
            break;
        }
        break;
    case Ast_Expr_UnOp:
        switch (expr->unop) {
        case Ast_UnOp_Negation:
            gen_expr(s, expr->lhs);
            fprintf(s->f, "neg %%rax\n");
            break;
        case Ast_UnOp_AddressOf:
            if (expr->lhs->discrim != Ast_Expr_Var) {
                printf("error: can only take address of variables\n");
                exit(-1);
            }
            fprintf(s->f, "mov %%rbp, %%rax\n");
            fprintf(s->f, "sub $%zu, %%rax\n", var_idx(s, expr->lhs->ident));
            break;
        case Ast_UnOp_Deref:
            if (expr->lhs->discrim != Ast_Expr_Var) {
                printf("error: can only take address of variables\n");
                exit(-1);
            }
            fprintf(s->f, "mov -%zu(%%rbp), %%rax\n",
                    var_idx(s, expr->lhs->ident));
            fprintf(s->f, "mov (%%rax), %%rax\n");
            break;
        }
        break;
    case Ast_Expr_AssignOp:
        if (expr->lhs->discrim != Ast_Expr_Var) {
            printf("error: can only assign to variables\n");
            exit(-1);
        }

        gen_expr(s, expr->rhs);

        switch (expr->assignop) {
        case Ast_AssignOp_Assign:
            fprintf(s->f, "mov %%rax, ");
            break;
        case Ast_AssignOp_Addition:
            fprintf(s->f, "add %%rax, ");
            break;
        case Ast_AssignOp_Subtraction:
            fprintf(s->f, "sub %%rax, ");
            break;
        case Ast_AssignOp_Multiplication:
            fprintf(s->f, "mov %%rax, %%rcx\n");
            fprintf(s->f, "mov -%zu(%%rbp), %%rax\n",
                    var_idx(s, expr->lhs->ident));
            fprintf(s->f, "imul %%rcx, %%rax\n");
            fprintf(s->f, "mov %%rax, ");
            break;
        case Ast_AssignOp_Division:
            fprintf(s->f, "mov %%rax, %%rcx\n");
            fprintf(s->f, "mov $0, %%rdx\n");
            fprintf(s->f, "mov -%zu(%%rbp), %%rax\n",
                    var_idx(s, expr->lhs->ident));
            fprintf(s->f, "idiv %%rcx\n");
            fprintf(s->f, "mov %%rax, ");
            break;
        }

        fprintf(s->f, "-%zu(%%rbp)\n", var_idx(s, expr->lhs->ident));
    }
    return true;
}

static bool gen_block(struct state *s, ast_block_t *block);

static struct ident *_declarator_ident(struct ast_declarator *declarator) {
    switch (declarator->kind) {
    case Ast_Declarator_Ident:
        return declarator->ident;
    }
}

static bool gen_statement(struct state *s, ast_statement_t *stmt) {
    switch (stmt->kind) {
    case Ast_Statement_Return:
        gen_expr(s, stmt->expr);
        fprintf(s->f, "mov %%rbp, %%rsp\n");
        fprintf(s->f, "popq %%rbp\n");
        fprintf(s->f, "ret\n");
        break;
    case Ast_Statement_If:
        gen_expr(s, stmt->expr);
        fprintf(s->f, "cmp $0, %%rax\n");
        size_t end_label = s->label_idx++;
        if (stmt->arm2 != NULL) {
            size_t else_label = s->label_idx++;
            fprintf(s->f, "je if_%zu\n", else_label);
            gen_statement(s, stmt->arm1);
            fprintf(s->f, "je if_%zu\n", end_label);
            fprintf(s->f, "if_%zu:\n", else_label);
            gen_statement(s, stmt->arm2);
        } else {
            fprintf(s->f, "je if_%zu\n", end_label);
            gen_statement(s, stmt->arm1);
        }
        fprintf(s->f, "if_%zu:\n", end_label);
        break;
    case Ast_Statement_Block:
        gen_block(s, stmt->block);
        break;
    case Ast_Statement_Expr:
        gen_expr(s, stmt->expr);
        break;
    }

    fprintf(s->f, "\n");

    return true;
}

static bool gen_declaration(struct state *s, struct ast_declaration *decl) {
    for (size_t i = 0; i < decl->ndeclarators; i++) {
        if (decl->exprs[i] != NULL) {
            gen_expr(s, decl->exprs[i]);
        }
        // TODO: We can avoid the push and just decrement the stack pointer if
        // we don't have an expr.
        fprintf(s->f, "pushq %%rax\n");
        map_insert(s->env, _declarator_ident(&decl->declarators[i]),
                   (void *)s->stack_idx);
        s->stack_idx += 8;
    }

    return true;
}

static bool gen_block(struct state *s, ast_block_t *block) {
    for (size_t i = 0; i < block->nitems; i++) {
        switch (block->items[i].kind) {
        case Ast_BlockItem_Statement:
            gen_statement(s, &block->items[i].stmt);
            break;
        case Ast_BlockItem_Declaration:
            gen_declaration(s, &block->items[i].decl);
            break;
        }
    }

    return true;
}

static bool gen_function(FILE *f, ast_function_t *func) {
    struct state s = {
        .f = f,
        .env = map_new(map_key_pointer),
        .stack_idx = 8,
    };
    fprintf(s.f, " .globl %s\n", ident_to_str(func->ident));
    fprintf(s.f, "%s:\n", ident_to_str(func->ident));
    fprintf(s.f, "pushq %%rbp\n");
    fprintf(s.f, "mov %%rsp, %%rbp\n");
    return gen_block(&s, &func->block);
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
