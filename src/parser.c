#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "ident.h"
#include "lexer.h"
#include "map.h"
#include "parser.h"
#include "ty.h"
#include "vec.h"

typedef struct {
    const char *prog;
    struct ident_table *idents;
    lexer_state_t lexer;
    token_t token;
    bool eof;
} state_t;

static parse_result_t ok() { return (parse_result_t){.kind = Parse_Result_Ok}; }

static parse_result_t error(state_t *state, const char *msg) {
    return (parse_result_t){.kind = Parse_Result_Error,
                            .diag = {.span = state->token.span, .msg = msg}};
}

static bool iserror(parse_result_t result) {
    return result.kind == Parse_Result_Error;
}

static void advance(state_t *state) {
    state->eof |= !lexer_next_token(&state->lexer, &state->token);
}

static bool eof(state_t *state) { return state->eof; }

static bool keyword(state_t *state, token_keyword_t keyword) {
    return state->token.discrim == Token_Keyword &&
           state->token.keyword == keyword;
}

static bool punctuator(state_t *state, token_punctuator_t punctuator) {
    return state->token.discrim == Token_Punctuator &&
           state->token.punctuator == punctuator;
}

static bool identifier(state_t *state, struct ident **ident) {
    if (state->token.discrim == Token_Identifier) {
        *ident = state->token.ident;
        return true;
    }
    return false;
}

// <expr-primary> = <constant>
parse_result_t parse_expr_primary(state_t *state, ast_expr_t *expr) {
    if (state->token.discrim == Token_Constant) {
        *expr = (ast_expr_t){
            .discrim = Ast_Expr_Constant,
            .str = state->token.str,
        };
        advance(state);
    } else if (state->token.discrim == Token_Identifier) {
        *expr = (ast_expr_t){
            .discrim = Ast_Expr_Var,
            .ident = state->token.ident,
        };
        advance(state);
    } else {
        return error(state, "expected primary expression");
    }

    return ok();
}

parse_result_t parse_expr_postfix(state_t *state, ast_expr_t *expr) {
    parse_result_t result = {0};
    if (iserror(result = parse_expr_primary(state, expr))) {
        return result;
    }

    while (true) {
        if (state->token.discrim != Token_Punctuator) {
            return ok();
        }

        switch (state->token.punctuator) {
        case Punctuator_Period: {
            advance(state);

            struct ident *ident;
            if (!identifier(state, &ident)) {
                return error(state, "expected identifier");
            }
            advance(state);

            ast_expr_t *lhs = malloc(sizeof(ast_expr_t));
            *lhs = *expr;

            *expr = (ast_expr_t){.discrim = Ast_Expr_MemberOf,
                                 .member = {
                                     .deref = false,
                                     .lhs = lhs,
                                     .ident = ident,
                                 }};
            break;
        }

        case Punctuator_Arrow: {
            advance(state);

            struct ident *ident;
            if (!identifier(state, &ident)) {
                return error(state, "expected identifier");
            }
            advance(state);

            ast_expr_t *lhs = malloc(sizeof(ast_expr_t));
            *lhs = *expr;

            *expr = (ast_expr_t){.discrim = Ast_Expr_MemberOf,
                                 .member = {
                                     .deref = true,
                                     .lhs = lhs,
                                     .ident = ident,
                                 }};
            break;
        }

        default:
            return ok();
        }
    }
}

// <expr-unary> =   - <expr-primary>
//                | & <expr-primary>
//                |   <expr-primary>
parse_result_t parse_expr_unary(state_t *state, ast_expr_t *expr) {
    struct unop {
        token_punctuator_t punctuator;
        ast_unop_t unop;
    };

    struct unop ops[] = {
        {Punctuator_Minus, Ast_UnOp_Negation},
        {Punctuator_Ampersand, Ast_UnOp_AddressOf},
        {Punctuator_Asterisk, Ast_UnOp_Deref},
    };
    size_t num_ops = sizeof(ops) / sizeof(ops[0]);

    struct unop *op = NULL;
    for (size_t i = 0; i < num_ops; i++) {
        if (punctuator(state, ops[i].punctuator)) {
            advance(state);
            op = &ops[i];
        }
    }

    if (op == NULL) {
        return parse_expr_postfix(state, expr);
    }

    parse_result_t result;
    // XXX: This should be cast-expression, but at least it should be
    // unary-expression to allow !!
    if (iserror(result = parse_expr_primary(state, expr))) {
        return result;
    }

    ast_expr_t *inner = malloc(sizeof(ast_expr_t));
    *inner = *expr;

    *expr = (ast_expr_t){
        .discrim = Ast_Expr_UnOp,
        .unop = op->unop,
        .lhs = inner,
    };

    return ok();
}

struct binop {
    token_punctuator_t punctuator;
    ast_binop_t op;
};

parse_result_t parse_expr_binop(state_t *state, ast_expr_t *expr,
                                struct binop ops[], size_t num_ops,
                                parse_result_t (*parse_next)(state_t *,
                                                             ast_expr_t *)) {
    parse_result_t result;
    if (iserror(result = parse_next(state, expr))) {
        return result;
    }

    while (true) {
        struct binop *op = NULL;
        for (size_t i = 0; i < num_ops; i++) {
            if (punctuator(state, ops[i].punctuator)) {
                advance(state);
                op = &ops[i];
            }
        }

        if (op == NULL) {
            return ok();
        }

        ast_expr_t *lhs = malloc(sizeof(ast_expr_t));
        *lhs = *expr;

        ast_expr_t *rhs = malloc(sizeof(ast_expr_t));
        if (iserror(result = parse_next(state, rhs))) {
            return result;
        }

        *expr = (ast_expr_t){
            .discrim = Ast_Expr_BinOp,
            .binop = op->op,
            .lhs = lhs,
            .rhs = rhs,
        };
    }

    return ok();
}

// <expr-multiplicative> =   <expr-unary>
//                         | <expr-unary> { + <expr-unary> }
parse_result_t parse_expr_multiplicative(state_t *state, ast_expr_t *expr) {
    struct binop ops[] = {
        {Punctuator_Asterisk, Ast_BinOp_Multiplication},
        {Punctuator_ForwardSlash, Ast_BinOp_Division},
    };
    return parse_expr_binop(state, expr, ops, sizeof(ops) / sizeof(ops[0]),
                            parse_expr_unary);
}

// <expr-additive> =   <expr-multiplicative>
//                   | <expr-multiplicative> { + <expr-multiplicative> }
parse_result_t parse_expr_additive(state_t *state, ast_expr_t *expr) {
    struct binop ops[] = {
        {Punctuator_Plus, Ast_BinOp_Addition},
        {Punctuator_Minus, Ast_BinOp_Subtraction},
    };
    return parse_expr_binop(state, expr, ops, sizeof(ops) / sizeof(ops[0]),
                            parse_expr_multiplicative);
}

parse_result_t parse_expr_relational(state_t *state, ast_expr_t *expr) {
    struct binop ops[] = {
        {Punctuator_LessThan, Ast_BinOp_LessThan},
        {Punctuator_LessThanEqual, Ast_BinOp_LessThanEqual},
        {Punctuator_GreaterThan, Ast_BinOp_GreaterThan},
        {Punctuator_GreaterThanEqual, Ast_BinOp_GreaterThanEqual},
    };
    return parse_expr_binop(state, expr, ops, sizeof(ops) / sizeof(ops[0]),
                            parse_expr_additive);
}

parse_result_t parse_expr_equality(state_t *state, ast_expr_t *expr) {
    struct binop ops[] = {
        {Punctuator_Equal, Ast_BinOp_Equal},
        {Punctuator_NotEqual, Ast_BinOp_NotEqual},
    };
    return parse_expr_binop(state, expr, ops, sizeof(ops) / sizeof(ops[0]),
                            parse_expr_relational);
}

parse_result_t parse_expr_assignment(state_t *state, ast_expr_t *expr) {
    struct assignop {
        token_punctuator_t punctuator;
        ast_assignop_t assignop;
    } ops[] = {
        {Punctuator_Assign, Ast_AssignOp_Assign},
        {Punctuator_PlusAssign, Ast_AssignOp_Addition},
        {Punctuator_MinusAssign, Ast_AssignOp_Subtraction},
        {Punctuator_AsteriskAssign, Ast_AssignOp_Multiplication},
        {Punctuator_ForwardSlashAssign, Ast_AssignOp_Division},
    };

    parse_result_t result;
    if (iserror(result = parse_expr_equality(state, expr))) {
        return result;
    }

    while (true) {
        struct assignop *op = NULL;
        for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
            if (punctuator(state, ops[i].punctuator)) {
                advance(state);
                op = &ops[i];
            }
        }

        if (op == NULL) {
            return ok();
        }

        ast_expr_t *lhs = malloc(sizeof(ast_expr_t));
        *lhs = *expr;

        ast_expr_t *rhs = malloc(sizeof(ast_expr_t));
        if (iserror(result = parse_expr_equality(state, rhs))) {
            return result;
        }

        *expr = (ast_expr_t){
            .discrim = Ast_Expr_AssignOp,
            .assignop = op->assignop,
            .lhs = lhs,
            .rhs = rhs,
        };
    }

    return ok();
}

parse_result_t parse_expr(state_t *state, ast_expr_t *expr) {
    return parse_expr_assignment(state, expr);
}

parse_result_t parse_block(state_t *state, ast_block_t *block);

// <statement> ::= "return" <expr> ";"
parse_result_t parse_statement(state_t *state, ast_statement_t *statement) {
    if (keyword(state, Keyword_return)) {
        advance(state);

        ast_expr_t *expr = (ast_expr_t *)malloc(sizeof(ast_expr_t));
        parse_result_t result;
        if (iserror(result = parse_expr(state, expr))) {
            return result;
        }

        *statement = (ast_statement_t){
            .kind = Ast_Statement_Return,
            .expr = expr,
        };

        if (!punctuator(state, Punctuator_Semicolon)) {
            return error(state, "expected semicolon");
        }
        advance(state);
    } else if (keyword(state, Keyword_if)) {
        advance(state);

        if (!punctuator(state, Punctuator_OpenParen)) {
            return error(state, "expected opening parenthesis");
        }
        advance(state);

        ast_expr_t *expr = malloc(sizeof(ast_expr_t));
        parse_result_t result = {0};
        if (iserror(result = parse_expr(state, expr))) {
            return result;
        }

        if (!punctuator(state, Punctuator_CloseParen)) {
            return error(state, "expected closing parenthesis");
        }
        advance(state);

        ast_statement_t *stmt1 = malloc(sizeof(ast_statement_t));
        if (iserror(result = parse_statement(state, stmt1))) {
            return result;
        }

        ast_statement_t *stmt2 = NULL;
        if (keyword(state, Keyword_else)) {
            advance(state);

            stmt2 = malloc(sizeof(ast_statement_t));
            if (iserror(result = parse_statement(state, stmt2))) {
                return result;
            }
        }

        *statement = (ast_statement_t){
            .kind = Ast_Statement_If,
            .expr = expr,
            .arm1 = stmt1,
            .arm2 = stmt2,
        };
    } else if (punctuator(state, Punctuator_OpenBrace)) {
        ast_block_t *block = malloc(sizeof(ast_block_t));
        parse_result_t result = {0};
        if (iserror(result = parse_block(state, block))) {
            return result;
        }

        *statement = (ast_statement_t){
            .kind = Ast_Statement_Block,
            .block = block,
        };
    } else {
        ast_expr_t *expr = malloc(sizeof(ast_expr_t));
        parse_result_t result = {0};
        if (iserror(result = parse_expr(state, expr))) {
            return result;
        }

        if (!punctuator(state, Punctuator_Semicolon)) {
            return error(state, "expected semicolon");
        }
        advance(state);

        *statement = (ast_statement_t){
            .kind = Ast_Statement_Expr,
            .expr = expr,
        };
    }

    return ok();
}

parse_result_t parse_type(state_t *state, struct ast_type *type);

// TODO: SHould this be shared between parse_declaration and
// parse_struct_declaration, given that the former allows an expr?
parse_result_t parse_declarator(state_t *state, struct ast_declarator *decl) {
    bool ptr = false;
    if (punctuator(state, Punctuator_Asterisk)) {
        advance(state);
        ptr = true;
    }

    struct ident *ident;
    if (!identifier(state, &ident)) {
        return error(state, "expected identifier");
    }
    advance(state);

    if (!ptr) {
        decl->kind = Ast_Declarator_Ident;
        decl->ident = ident;
    } else {
        decl->kind = Ast_Declarator_Pointer;
        decl->next = malloc(sizeof(struct ast_declarator));
        decl->next->kind = Ast_Declarator_Ident;
        decl->next->ident = ident;
    }

    return ok();
}

// XXX: There's a lot of similarity with parse_declaration, although structs
// will eventually allow bitfields. Considers collapsing.
parse_result_t parse_struct_declaration(state_t *state,
                                        struct ast_struct_declaration *decl) {
    parse_result_t result = {0};
    if (iserror(result = parse_type(state, &decl->type))) {
        return result;
    }

    // TODO: pointer declarators.
    struct vec *declarators = vec_new(sizeof(struct ast_declarator));

    // NOTE: We will never enter this loop if there is no declarator.
    while (!punctuator(state, Punctuator_Semicolon)) {
        struct ast_declarator decl = {0};
        if (iserror(result = parse_declarator(state, &decl))) {
            return result;
        }

        vec_append(declarators, &decl);

        if (punctuator(state, Punctuator_Comma)) {
            advance(state);
        }
    }

    if (!punctuator(state, Punctuator_Semicolon)) {
        return error(state, "expected semicolon");
    }
    advance(state);

    decl->ndeclarators = vec_into_raw(declarators, (void **)&decl->declarators);

    return ok();
}

parse_result_t parse_type_struct(state_t *state, struct ast_type *type) {
    advance(state);

    // Identifier (aka. tag) is optional.
    struct ident *ident = NULL;
    if (identifier(state, &ident)) {
        advance(state);
    }

    if (!punctuator(state, Punctuator_OpenBrace)) {
        return error(state, "expected opening brace");
    }
    advance(state);

    struct vec *declarations = vec_new(sizeof(struct ast_struct_declaration));

    while (!punctuator(state, Punctuator_CloseBrace)) {
        struct ast_struct_declaration decl = {0};
        parse_result_t result = {0};
        if (iserror(result = parse_struct_declaration(state, &decl))) {
            return result;
        }

        vec_append(declarations, &decl);
    }
    advance(state);

    type->kind = Ast_Type_Struct;
    type->ident = ident;
    type->ndeclarations =
        vec_into_raw(declarations, (void **)&type->declarations);

    return ok();
}

parse_result_t parse_type(state_t *state, struct ast_type *type) {
    if (keyword(state, Keyword_int)) {
        advance(state);

        type->kind = Ast_Type_BasicType;
        type->basic = Ast_BasicType_Int;

        return ok();
    }

    if (keyword(state, Keyword_struct)) {
        return parse_type_struct(state, type);
    }

    return error(state, "expected int or struct");
}

parse_result_t parse_declaration(state_t *state, struct ast_declaration *decl) {
    parse_result_t result = {0};
    if (iserror(result = parse_type(state, &decl->type))) {
        return result;
    }

    bool ptr = false;
    if (punctuator(state, Punctuator_Asterisk)) {
        advance(state);
        ptr = true;
    }

    struct ident *ident;
    if (!identifier(state, &ident)) {
        return error(state, "expected identifier");
    }
    advance(state);

    if (!punctuator(state, Punctuator_Assign)) {
        return error(state, "expected =");
    }
    advance(state);

    ast_expr_t *expr = (ast_expr_t *)malloc(sizeof(ast_expr_t));
    if (iserror(result = parse_expr(state, expr))) {
        return result;
    }

    if (!ptr) {
        decl->declarator.kind = Ast_Declarator_Ident;
        decl->declarator.ident = ident;
    } else {
        decl->declarator.kind = Ast_Declarator_Pointer;
        decl->declarator.next = malloc(sizeof(struct ast_declarator));
        decl->declarator.next->kind = Ast_Declarator_Ident;
        decl->declarator.next->ident = ident;
    }

    decl->expr = expr;

    if (!punctuator(state, Punctuator_Semicolon)) {
        return error(state, "expected semicolon");
    }
    advance(state);

    return ok();
}

parse_result_t parse_block_item(state_t *state, struct ast_block_item *item){
    parse_result_t result = {0};

    bool is_type =
        keyword(state, Keyword_int) || keyword(state, Keyword_struct);
    if (is_type) {
        item->kind = Ast_BlockItem_Declaration;
        if (iserror(result = parse_declaration(state, &item->decl))) {
            return result;
        }
        return ok();
    }

    item->kind = Ast_BlockItem_Statement;
    if (iserror(result = parse_statement(state, &item->stmt))) {
        return result;
    }

    return ok();
}

// <block> ::= { <statement> }
parse_result_t parse_block(state_t *state, ast_block_t *block) {
    if (!punctuator(state, Punctuator_OpenBrace)) {
        return error(state, "expected opening brace");
    }
    advance(state);

    struct vec *items = vec_new(sizeof(struct ast_block_item));

    while (!punctuator(state, Punctuator_CloseBrace)) {
        struct ast_block_item item = {0};
        parse_result_t result = {0};
        if (iserror(result = parse_block_item(state, &item))) {
            return result;
        }
        vec_append(items, &item);
    }
    advance(state);

    block->nitems = vec_into_raw(items, (void **)&block->items);

    return ok();
}

// <function> ::= "int" <id> "(" ")" "{" <block> "}"
parse_result_t parse_function(state_t *state, ast_function_t *function) {
    if (!keyword(state, Keyword_int)) {
        return error(state, "expected keyword int");
    }
    advance(state);

    struct ident *ident;
    if (!identifier(state, &ident)) {
        printf("error: expected identifier\n");
        return error(state, "expected identifier");
    }
    advance(state);

    if (!punctuator(state, Punctuator_OpenParen)) {
        return error(state, "expected opening parenthesis");
    }
    advance(state);
    if (!punctuator(state, Punctuator_CloseParen)) {
        return error(state, "expected closing parenthesis");
    }
    advance(state);

    ast_block_t block = {0};
    parse_result_t result = {0};
    if (iserror(result = parse_block(state, &block))) {
        return result;
    }

    *function = (ast_function_t){.ident = ident, .block = block};

    return ok();
}

// <program> ::= <function>
parse_result_t parse_program(state_t *state, ast_program_t *program) {
    struct map *functions = map_new(map_key_string);

    while (!eof(state)) {
        ast_function_t *function = calloc(1, sizeof(ast_function_t));
        parse_result_t result;
        if (iserror(result = parse_function(state, function))) {
            return result;
        }

        // TODO: Avoid ident_to_str if/when map can have arbitrary keys
        map_insert(functions, ident_to_str(function->ident), function);
    }

    *program = (ast_program_t){.functions = functions};

    // TODO: free functions

    return ok();
}

parse_result_t parser_parse(struct ident_table *idents, const char *prog,
                            ast_program_t *program) {
    lexer_state_t lexer = lexer_new(idents, prog);
    token_t token;
    if (!lexer_next_token(&lexer, &token)) {
        printf("fatal: couldn't lex\n");
        exit(-1);
    }

    state_t state = {
        .prog = prog,
        .idents = idents,
        .lexer = lexer,
        .token = token,
        .eof = false,
    };
    return parse_program(&state, program);
}
