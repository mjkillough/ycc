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

static void _expr_unop(struct pprint *pp, ast_expr_t *expr) {
    switch (expr->unop) {
    case Ast_UnOp_Negation:
        pprintf(pp, "Neg(");
        ast_pprint_expr(pp, expr->lhs);
        pprintf(pp, ")");
        break;

    case Ast_UnOp_AddressOf:
        pprintf(pp, "AddrOf(");
        ast_pprint_expr(pp, expr->lhs);
        pprintf(pp, ")");
        break;

    case Ast_UnOp_Deref:
        pprintf(pp, "Deref(");
        ast_pprint_expr(pp, expr->lhs);
        pprintf(pp, ")");
        break;
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
        pprintf(pp, "Expr(");
        _expr_unop(pp, expr);
        pprintf(pp, ")");
        break;

    case Ast_Expr_AssignOp:
        pprintf(pp, "Assign(");
        ast_pprint_expr(pp, expr->lhs);
        pprintf(pp, " %s ", _expr_assignop(expr));
        ast_pprint_expr(pp, expr->rhs);
        pprintf(pp, ")");
        break;

    case Ast_Expr_MemberOf:
        pprintf(pp, "MemberOf(");
        ast_pprint_expr(pp, expr->member.lhs);
        pprintf(pp, ", ");
        pprintf(pp, "%s", expr->member.ident);
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
    if (block->nitems > 0) {
        pprint_newline(pp);
    }

    for (size_t i = 0; i < block->nitems; i++) {
        switch (block->items[i].kind) {
        case Ast_BlockItem_Statement:
            ast_pprint_statement(pp, &block->items[i].stmt);
            break;
        case Ast_BlockItem_Declaration:
            ast_pprint_declaration(pp, &block->items[i].decl);
            break;
        }

        if (i < block->nitems - 1) {
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

void ast_pprint_declarator(struct pprint *pp, struct ast_declarator *decl) {
    switch (decl->kind) {
    case Ast_Declarator_Pointer:
        pprintf(pp, " *");
        ast_pprint_declarator(pp, decl->next);
        break;
    case Ast_Declarator_Ident:
        pprintf(pp, " %s", decl->ident);
        break;
    }
}

void ast_pprint_struct_declaration(struct pprint *pp,
                                   struct ast_struct_declaration *decl) {
    pprintf(pp, "Decl(");
    ast_pprint_type(pp, &decl->type);
    pprintf(pp, ",");

    if (decl->ndeclarators > 1) {
        pprintf(pp, " [");
        pprint_newline(pp);
        pprint_indent(pp);
    }

    for (size_t i = 0; i < decl->ndeclarators; i++) {
        ast_pprint_declarator(pp, &decl->declarators[i]);

        if (decl->ndeclarators > 1) {
            pprintf(pp, ",");
            pprint_newline(pp);
        }
    }

    if (decl->ndeclarators > 1) {
        pprint_unindent(pp);
        pprintf(pp, "]");
    }

    pprintf(pp, ")");
}

static const char *_basic_type(enum ast_basic_type ty) {
    switch (ty) {
    case Ast_BasicType_Int:
        return "int";
    }
}

static void ast_pprint_struct(struct pprint *pp, struct ast_type *ty) {
    // TODO: Support identifier.

    pprintf(pp, "Struct(_, [");
    pprint_newline(pp);
    pprint_indent(pp);

    for (size_t i = 0; i < ty->ndeclarations; i++) {
        ast_pprint_struct_declaration(pp, &ty->declarations[i]);

        pprintf(pp, ",");
        pprint_newline(pp);
    }

    pprint_unindent(pp);
}

void ast_pprint_type(struct pprint *pp, struct ast_type *ty) {
    switch (ty->kind) {
    case Ast_Type_BasicType:
        pprintf(pp, _basic_type(ty->basic));
        break;

    case Ast_Type_Struct:
        ast_pprint_struct(pp, ty);
        break;
    }
}

void ast_pprint_declaration(struct pprint *pp, struct ast_declaration *decl) {
    pprintf(pp, "Decl(");
    ast_pprint_type(pp, &decl->type);
    pprintf(pp, ",");
    ast_pprint_declarator(pp, &decl->declarator);

    if (decl->expr != NULL) {
        pprintf(pp, ", = ");
        ast_pprint_expr(pp, decl->expr);
    }

    pprintf(pp, ")");
}

