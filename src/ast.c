#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "map.h"

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
    case Ast_BinOp_Equal:
        return "==";
    case Ast_BinOp_NotEqual:
        return "!=";
    case Ast_BinOp_LessThan:
        return "<";
    case Ast_BinOp_LessThanEqual:
        return "<=";
    case Ast_BinOp_GreatherThan:
        return ">";
    case Ast_BinOp_GreaterThanEqual:
        return ">=";
    default:
        return "UNKNOWN_OP";
    }
}

static const char *ast_print_expr_assignop(ast_expr_t *expr) {
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

void ast_print_expr(FILE *f, ast_expr_t *expr) {
    switch (expr->discrim) {
    case Ast_Expr_Constant:
        fprintf(f, "%s", expr->str);
        break;
    case Ast_Expr_Var:
        fprintf(f, "Var(%s)", expr->str);
        break;
    case Ast_Expr_BinOp:
        fprintf(f, "Expr(");
        ast_print_expr(f, expr->lhs);
        fprintf(f, " %s ", ast_print_expr_binop(expr));
        ast_print_expr(f, expr->rhs);
        fprintf(f, ")");
        break;
    case Ast_Expr_UnOp:
        fprintf(f, "Expr(Neg(");
        ast_print_expr(f, expr->lhs);
        printf("))");
        break;
    case Ast_Expr_AssignOp:
        fprintf(f, "Assign(");
        ast_print_expr(f, expr->lhs);
        fprintf(f, " %s ", ast_print_expr_assignop(expr));
        ast_print_expr(f, expr->rhs);
        fprintf(f, ")");
        break;
    }
}

void ast_print_block(FILE *f, ast_block_t *block);

void ast_print_statement(FILE *f, ast_statement_t *stmt) {
    fprintf(f, "Statement(");

    switch (stmt->kind) {
    case Ast_Statement_Return:
        fprintf(f, "Return(");
        ast_print_expr(f, stmt->expr);
        fprintf(f, ")");
        break;
    case Ast_Statement_Decl:
        fprintf(f, "Decl(int, %s, ", stmt->identifier);
        ast_print_expr(f, stmt->expr);
        fprintf(f, ")");
        break;
    case Ast_Statement_If:
        fprintf(f, "If(");
        ast_print_expr(f, stmt->expr);
        fprintf(f, ", ");
        ast_print_statement(f, stmt->arm1);
        if (stmt->arm2 != NULL) {
            fprintf(f, ", ");
            ast_print_statement(f, stmt->arm2);
        }
        fprintf(f, ")");
        break;
    case Ast_Statement_Block:
        ast_print_block(f, stmt->block);
        break;
    case Ast_Statement_Expr:
        ast_print_expr(f, stmt->expr);
        break;
    }

    fprintf(f, ")");
}

void ast_print_block(FILE *f, ast_block_t *block) {
    fprintf(f, "Block(");
    for (size_t i = 0; i < block->count; i++) {
        ast_print_statement(f, &block->stmts[i]);

        if (i < block->count - 1) {
            fprintf(f, ", ");
        }
    }
    fprintf(f, ")");
}

void ast_print_function(FILE *f, ast_function_t *func) {
    fprintf(f, "Function(name=%s, ", func->name);
    ast_print_block(f, &func->block);
    fprintf(f, ")");
    fprintf(f, "\n");
}

static bool ast_print_function_iter(void *context, const char *key,
                                    void *value) {
    UNUSED(context);
    UNUSED(key);

    FILE *f = (FILE *)context;
    ast_print_function(f, value);
    return true;
}

void ast_print_program(FILE *f, ast_program_t *prog) {
    map_iter(prog->functions, f, &ast_print_function_iter);
}
