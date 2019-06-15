#pragma once

#include <stdbool.h>

typedef struct {
    unsigned int line;
    unsigned int character;

    const char *start;
    const char *end;
} token_span_t;

typedef enum {
    Token_Keyword,
    Token_Identifier,
    Token_Constant,
    // Token_StringLiteral,
    Token_Punctuator
} token_discrim_t;

typedef enum {
    Keyword_int,
    Keyword_return,
} token_keyword_t;

typedef enum {
    Punctuator_Semicolon,

    Punctuator_Equals,

    Punctuator_Plus,
    Punctuator_Minus,
    Punctuator_Asterisk,
    Punctuator_ForwardSlash,

    Punctuator_OpenParen,
    Punctuator_CloseParen,
    Punctuator_OpenBrace,
    Punctuator_CloseBrace,
} token_punctuator_t;

typedef struct {
    token_discrim_t discrim;
    union {
        // Token_Keyword
        token_keyword_t keyword;
        // Token_Identifier, Token_Constant
        const char *str;
        // Token_Punctuator
        token_punctuator_t punctuator;
    };
    token_span_t span;
} token_t;

// TODO: Make this private
typedef struct {
    const char *prog;
    const char *unlexed;

    unsigned int line;
    unsigned int character;
} lexer_state_t;

lexer_state_t lexer_new(const char *prog);
bool lexer_next_token(lexer_state_t *state, token_t *next);

void lexer_print_token(token_t tok);
