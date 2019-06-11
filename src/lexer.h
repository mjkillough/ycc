#pragma once

#include <stdbool.h>

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
    Punctuator_Plus,
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
} token_t;

void lexer_print_token(token_t tok);
bool lexer_next_token(const char **prog, token_t *next);
