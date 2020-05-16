#pragma once

#include <stdbool.h>
#include <stdio.h>

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
    Keyword_char,
    Keyword_short,
    Keyword_int,
    Keyword_long,
    Keyword_return,
    Keyword_if,
    Keyword_else,
    Keyword_struct,
    Keyword_const,
} token_keyword_t;

typedef enum {
    Punctuator_Period,
    Punctuator_Arrow,

    Punctuator_Semicolon,
    Punctuator_Comma,

    Punctuator_Ampersand,

    Punctuator_Assign,
    Punctuator_PlusAssign,
    Punctuator_MinusAssign,
    Punctuator_AsteriskAssign,
    Punctuator_ForwardSlashAssign,

    Punctuator_Equal,
    Punctuator_NotEqual,
    Punctuator_GreaterThan,
    Punctuator_GreaterThanEqual,
    Punctuator_LessThan,
    Punctuator_LessThanEqual,

    Punctuator_Plus,
    Punctuator_Minus,
    Punctuator_Asterisk,
    Punctuator_ForwardSlash,

    Punctuator_OpenParen,
    Punctuator_CloseParen,
    Punctuator_OpenBrace,
    Punctuator_CloseBrace,
    Punctuator_OpenBracket,
    Punctuator_CloseBracket,
} token_punctuator_t;

typedef struct {
    token_discrim_t discrim;
    union {
        // Token_Keyword
        token_keyword_t keyword;
        // Token_Identifier
        struct ident *ident;
        // Token_Constant:
        const char *str;
        // Token_Punctuator
        token_punctuator_t punctuator;
    };
    token_span_t span;
} token_t;

// TODO: Make this private
typedef struct {
    struct ident_table *idents;

    const char *prog;
    const char *unlexed;

    unsigned int line;
    unsigned int character;
} lexer_state_t;

lexer_state_t lexer_new(struct ident_table *idents, const char *prog);
bool lexer_next_token(lexer_state_t *state, token_t *next);

void lexer_print_token(FILE *f, token_t tok);
