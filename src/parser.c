#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

typedef struct {
    const char *prog;
    lexer_state_t lexer;
    token_t token;
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
    // XXX error
    lexer_next_token(&state->lexer, &state->token);
}

static bool keyword(state_t *state, token_keyword_t keyword) {
    return state->token.discrim == Token_Keyword &&
           state->token.keyword == keyword;
}

static bool punctuator(state_t *state, token_punctuator_t punctuator) {
    return state->token.discrim == Token_Punctuator &&
           state->token.punctuator == punctuator;
}

static bool identifier(state_t *state, const char **str) {
    if (state->token.discrim == Token_Identifier) {
        *str = state->token.str;
        return true;
    }
    return false;
}

// <expr-primary> = <constant>
parse_result_t parse_expr_primary(state_t *state, ast_expr_t *expr) {
    if (state->token.discrim != Token_Constant) {
        return error(state, "expected integer");
    }

    *expr = (ast_expr_t){
        .discrim = Ast_Expr_Constant,
        .str = state->token.str,
    };

    advance(state);

    return ok();
}

// <expr-unary> = - <expr-primary>
parse_result_t parse_expr_unary(state_t *state, ast_expr_t *expr) {
    // TODO
    return parse_expr_primary(state, expr);
}

// <expr-multiplicative> =   <expr-unary>
//                         | <expr-unary> { + <expr-unary> }
parse_result_t parse_expr_multiplicative(state_t *state, ast_expr_t *expr) {
    parse_result_t result;
    if (iserror(result = parse_expr_unary(state, expr))) {
        return result;
    }

    while (punctuator(state, Punctuator_Asterisk)) {
        advance(state);

        ast_expr_t *lhs = malloc(sizeof(ast_expr_t));
        *lhs = *expr;

        ast_expr_t *rhs = malloc(sizeof(ast_expr_t));
        if (iserror(result = parse_expr_unary(state, rhs))) {
            return result;
        }

        *expr = (ast_expr_t){
            .discrim = Ast_Expr_Multiplication,
            .lhs = lhs,
            .rhs = rhs,
        };
    }

    return ok();
}

// <expr-additive> =   <expr-multiplicative>
//                   | <expr-multiplicative> { + <expr-multiplicative> }
parse_result_t parse_expr_additive(state_t *state, ast_expr_t *expr) {
    parse_result_t result;
    if (iserror(result = parse_expr_multiplicative(state, expr))) {
        return result;
    }

    while (punctuator(state, Punctuator_Plus)) {
        advance(state);

        ast_expr_t *lhs = malloc(sizeof(ast_expr_t));
        *lhs = *expr;

        ast_expr_t *rhs = malloc(sizeof(ast_expr_t));
        if (iserror(result = parse_expr_multiplicative(state, rhs))) {
            return result;
        }

        *expr = (ast_expr_t){
            .discrim = Ast_Expr_Addition,
            .lhs = lhs,
            .rhs = rhs,
        };
    }

    return ok();
}

// <expr> ::= <expr-multiplicative>
parse_result_t parse_expr(state_t *state, ast_expr_t *expr) {
    return parse_expr_additive(state, expr);
}

// <statement> ::= "return" <expr> ";"
parse_result_t parse_statement(state_t *state, ast_statement_t *statement) {
    if (!keyword(state, Keyword_return)) {
        return error(state, "expected keyword return");
    }
    advance(state);

    ast_expr_t expr;
    parse_result_t result;
    if (iserror(result = parse_expr(state, &expr))) {
        return result;
    }

    if (!punctuator(state, Punctuator_Semicolon)) {
        return error(state, "expected semicolon");
    }
    advance(state);

    *statement = (ast_statement_t){.expr = expr};

    return ok();
}

// <function> ::= "int" <id> "(" ")" "{" <statement> "}"
parse_result_t parse_function(state_t *state, ast_function_t *function) {
    if (!keyword(state, Keyword_int)) {
        return error(state, "expected keyword int");
    }
    advance(state);

    const char *name;
    if (!identifier(state, &name)) {
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

    if (!punctuator(state, Punctuator_OpenBrace)) {
        return error(state, "expected opening brace");
    }
    advance(state);

    ast_statement_t statement;
    parse_result_t result;
    if (iserror(result = parse_statement(state, &statement))) {
        return result;
    }

    if (!punctuator(state, Punctuator_CloseBrace)) {
        return error(state, "expected closing brace");
    }
    advance(state);

    *function = (ast_function_t){.name = name, .statement = statement};

    return ok();
}

// <program> ::= <function>
parse_result_t parse_program(state_t *state, ast_program_t *program) {
    ast_function_t function;
    parse_result_t result;
    if (iserror(result = parse_function(state, &function))) {
        return result;
    }

    *program = (ast_program_t){.function = function};

    return ok();
}

parse_result_t parser_parse(const char *prog, ast_program_t *program) {
    lexer_state_t lexer = lexer_new(prog);
    token_t token;
    if (!lexer_next_token(&lexer, &token)) {
        printf("fatal: couldn't lex\n");
        exit(-1);
    }

    state_t state = {
        .prog = prog,
        .lexer = lexer,
        .token = token,
    };
    return parse_program(&state, program);
}
