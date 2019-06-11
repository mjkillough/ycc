#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

typedef struct {
    const char *prog;
    token_t token;
} state_t;

static void advance(state_t *state) {
    // XXX error
    lexer_next_token(&state->prog, &state->token);
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

// <expr> ::= <int> | <int> + <expr>
static bool parse_expr(state_t *state, ast_expr_t *expr) {
    if (state->token.discrim != Token_Constant) {
        printf("error: expected integer\n");
        return false;
    }

    const char *constant = state->token.str;
    advance(state);

    if (!punctuator(state, Punctuator_Plus)) {
        *expr = (ast_expr_t){.discrim = Ast_Expr_Constant, .str = constant};
        return true;
    }
    advance(state);

    ast_expr_t *rhs = malloc(sizeof(ast_expr_t));
    if (!parse_expr(state, rhs)) {
        printf("error: expected expr\n");
        return false;
    }

    *expr =
        (ast_expr_t){.discrim = Ast_Expr_Addition, .str = constant, .rhs = rhs};

    return true;
}

// <statement> ::= "return" <expr> ";"
static bool parse_statement(state_t *state, ast_statement_t *statement) {
    if (!keyword(state, Keyword_return)) {
        printf("error: expected keyword return\n");
        return false;
    }
    advance(state);

    ast_expr_t expr;
    if (!parse_expr(state, &expr)) {
        printf("error: expected expr\n");
        return false;
    }

    if (!punctuator(state, Punctuator_Semicolon)) {
        printf("error: expected semicolon\n");
        return false;
    }
    advance(state);

    *statement = (ast_statement_t){.expr = expr};

    return true;
}

// <function> ::= "int" <id> "(" ")" "{" <statement> "}"
static bool parse_function(state_t *state, ast_function_t *function) {
    if (!keyword(state, Keyword_int)) {
        printf("error: expected keyword int\n");
        return false;
    }
    advance(state);

    const char *name;
    if (!identifier(state, &name)) {
        printf("error: expected identifier\n");
        return false;
    }
    advance(state);

    if (!punctuator(state, Punctuator_OpenParen)) {
        printf("error: expected opening parenthesis\n");
        return false;
    }
    advance(state);
    if (!punctuator(state, Punctuator_CloseParen)) {
        printf("error: expected closing parenthesis\n");
        return false;
    }
    advance(state);

    if (!punctuator(state, Punctuator_OpenBrace)) {
        printf("error: expected opening brace\n");
        return false;
    }
    advance(state);

    ast_statement_t statement;
    if (!parse_statement(state, &statement)) {
        printf("error: expected statement\n");
        return false;
    }

    if (!punctuator(state, Punctuator_CloseBrace)) {
        printf("error: expected closing brace\n");
        return false;
    }
    advance(state);

    *function = (ast_function_t){.name = name, .statement = statement};

    return true;
}

// <program> ::= <function>
static bool parse_program(state_t *state, ast_program_t *program) {
    ast_function_t function;
    if (!parse_function(state, &function)) {
        return false;
    }

    *program = (ast_program_t){.function = function};

    return true;
}

bool parser_parse(const char *prog, ast_program_t *program) {
    token_t token;
    if (!lexer_next_token(&prog, &token)) {
        return false;
    }

    state_t state = {
        .prog = prog,
        .token = token,
    };
    if (!parse_program(&state, program)) {
        return false;
    }

    return true;
}
