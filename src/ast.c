#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "map.h"
#include "pprint.h"
#include "ty.h"

static const char *_expr_binop(ast_expr_t *expr);
static const char *_expr_assignop(ast_expr_t *expr);

static const char *_expr_binop(ast_expr_t *expr) {
    switch (expr->binop) {
    case Ast_BinOp_Addition:
        return "+";
    case Ast_BinOp_Subtraction:
        return "-";
    case Ast_BinOp_Multiplication:
        return "*";
    case Ast_BinOp_Division:
        return "/";
    case Ast_BinOp_Equal:
        return "==";
    case Ast_BinOp_NotEqual:
        return "!=";
    case Ast_BinOp_LessThan:
        return "<";
    case Ast_BinOp_LessThanEqual:
        return "<=";
    case Ast_BinOp_GreaterThan:
        return ">";
    case Ast_BinOp_GreaterThanEqual:
        return ">=";
    default:
        return "UNKNOWN_OP";
    }
}

static const char *_expr_assignop(ast_expr_t *expr) {
    switch (expr->assignop) {
    case Ast_AssignOp_Assign:
        return "=";
    case Ast_AssignOp_Addition:
        return "+=";
    case Ast_AssignOp_Subtraction:
        return "-=";
    case Ast_AssignOp_Multiplication:
        return "*=";
    case Ast_AssignOp_Division:
        return "/=";
    default:
        return "UNKNOWN_OP";
    }
}

void ast_pprint_expr(struct pprint *pp, ast_expr_t *expr) {
    switch (expr->discrim) {
    case Ast_Expr_Constant:
        pprintf(pp, "%s", expr->str);
        break;

    case Ast_Expr_Var:
        pprintf(pp, "Var(%s)", expr->str);
        break;

    case Ast_Expr_BinOp:
        pprintf(pp, "Expr(");
        ast_pprint_expr(pp, expr->lhs);
        pprintf(pp, " %s ", _expr_binop(expr));
        ast_pprint_expr(pp, expr->rhs);
        pprintf(pp, ")");
        break;

    case Ast_Expr_UnOp:
        pprintf(pp, "Expr(Neg(");
        ast_pprint_expr(pp, expr->lhs);
        pprintf(pp, "))");
        break;

    case Ast_Expr_AssignOp:
        pprintf(pp, "Assign(");
        ast_pprint_expr(pp, expr->lhs);
        pprintf(pp, " %s ", _expr_assignop(expr));
        ast_pprint_expr(pp, expr->rhs);
        pprintf(pp, ")");
        break;
    }
}

void ast_pprint_statement(struct pprint *pp, ast_statement_t *stmt) {
    pprintf(pp, "Statement(");

    switch (stmt->kind) {
    case Ast_Statement_Return:
        pprintf(pp, "Return(");
        ast_pprint_expr(pp, stmt->expr);
        pprintf(pp, ")");
        break;

    case Ast_Statement_Decl:
        pprintf(pp, "Decl(");
        ty_pprint(pp, &stmt->decl->ty);
        pprintf(pp, ", %s, ", stmt->decl->identifier);
        ast_pprint_expr(pp, stmt->decl->expr);
        pprintf(pp, ")");
        break;

    case Ast_Statement_If:
        pprintf(pp, "If(");
        ast_pprint_expr(pp, stmt->expr);
        pprintf(pp, ", ");
        ast_pprint_statement(pp, stmt->arm1);
        if (stmt->arm2 != NULL) {
            pprintf(pp, ", ");
            ast_pprint_statement(pp, stmt->arm2);
        }
        pprintf(pp, ")");
        break;

    case Ast_Statement_Block:
        ast_pprint_block(pp, stmt->block);
        break;

    case Ast_Statement_Expr:
        ast_pprint_expr(pp, stmt->expr);
        break;
    }

    pprintf(pp, ")");
}

void ast_pprint_block(struct pprint *pp, ast_block_t *block) {
    pprintf(pp, "Block(");

    pprint_indent(pp);
    if (block->count > 0) {
        pprint_newline(pp);
    }

    for (size_t i = 0; i < block->count; i++) {
        ast_pprint_statement(pp, &block->stmts[i]);

        if (i < block->count - 1) {
            pprintf(pp, ", ");
        }
        pprint_newline(pp);
    }

    pprint_unindent(pp);

    pprintf(pp, ")");
}

void ast_pprint_function(struct pprint *pp, ast_function_t *func) {
    pprintf(pp, "Function(name=%s, ", func->name);
    ast_pprint_block(pp, &func->block);
    pprintf(pp, ")");

    pprint_newline(pp);
}

static bool ast_pprint_function_iter(void *context, const char *key,
                                     void *value) {
    UNUSED(key);
    ast_pprint_function((struct pprint *)context, value);
    return true;
}

void ast_pprint_program(struct pprint *pp, ast_program_t *prog) {
    map_iter(prog->functions, pp, &ast_pprint_function_iter);
}
