#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ident.h"
#include "lexer.h"

static const struct {
    token_keyword_t keyword;
    const char *str;
} keywords[] = {
    {Keyword_char, "char"},     {Keyword_short, "short"},
    {Keyword_int, "int"},       {Keyword_long, "long"},
    {Keyword_return, "return"}, {Keyword_if, "if"},
    {Keyword_else, "else"},     {Keyword_struct, "struct"},
    {Keyword_union, "union"},   {Keyword_const, "const"},
};

static bool lookup_keyword(const char *str, size_t len, token_keyword_t *out) {
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strlen(keywords[i].str) == len &&
            strncmp(str, keywords[i].str, len) == 0) {
            *out = keywords[i].keyword;
            return true;
        }
    }
    return false;
}

static const char *keyword_as_string(token_keyword_t keyword) {
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (keywords[i].keyword == keyword) {
            return keywords[i].str;
        }
    }
    return "UNKNOWN_KEYWORD";
}

static bool iswhitespace(char c) { return c == ' ' || c == '\t' || c == '\n'; }

static bool isnondigit(char c) { return c == '_' || isalpha(c); }

static void advance(lexer_state_t *state) {
    char prev = state->unlexed[0];

    state->unlexed++;
    state->character++;

    if (prev == '\n') {
        state->line++;
        state->character = 0;
    }
}

static char peek(lexer_state_t *state, size_t i) { return state->unlexed[i]; }

static token_span_t span(lexer_state_t *state, const char *start,
                         const char *end) {
    return (token_span_t){
        .start = start,
        .end = end,
        .line = state->line,
        .character = state->character,
    };
}

static bool lexer_constant(lexer_state_t *state, token_t *next) {
    const char *start = state->unlexed;
    advance(state);

    while (true) {
        char c = state->unlexed[0];

        if (c == '\0') {
            return false;
        }

        if (isdigit(c)) {
            advance(state);
        } else {
            size_t len = state->unlexed - start;
            char *str = (char *)malloc(len + 1);
            strlcpy(str, start, len + 1);

            *next = (token_t){.discrim = Token_Constant,
                              .str = str,
                              .span = span(state, start, state->unlexed - 1)};
            return true;
        }
    }
}

static bool lexer_identifier_or_keyword(lexer_state_t *state, token_t *next) {
    const char *start = state->unlexed;
    advance(state);

    while (true) {
        char c = state->unlexed[0];

        if (c == '\0') {
            return false;
        }

        if (isnondigit(c) || isdigit(c)) {
            advance(state);
        } else {
            size_t len = state->unlexed - start;

            token_keyword_t keyword;
            if (lookup_keyword(start, len, &keyword)) {
                *next = (token_t){
                    .discrim = Token_Keyword,
                    .keyword = keyword,
                    .span = span(state, start, state->unlexed - 1),
                };
                return true;
            } else {
                char *str = (char *)malloc(len + 1);
                strlcpy(str, start, len + 1);

                // TODO: ident_from_str does strdup(). Does it need to?
                struct ident *ident = ident_from_str(state->idents, str);

                *next =
                    (token_t){.discrim = Token_Identifier,
                              .ident = ident,
                              .span = span(state, start, state->unlexed - 1)};
                return true;
            }
        }
    }
}

static bool ispunctuation(char c) {
    return c == '(' || c == ')' || c == '{' || c == '}' || c == '[' ||
           c == ']' || c == ';' || c == '+' || c == '-' || c == '*' ||
           c == '/' || c == '=' || c == '>' || c == '<' || c == '!' ||
           c == '&' || c == ',' || c == '.';
}

static struct {
    token_punctuator_t punctuator;
    const char *str;
    size_t len;
} punctuators[] = {
    {Punctuator_Equal, "==", 2},
    {Punctuator_NotEqual, "!=", 2},
    {Punctuator_GreaterThanEqual, ">=", 2},
    {Punctuator_LessThanEqual, "<=", 2},
    {Punctuator_PlusAssign, "+=", 2},
    {Punctuator_MinusAssign, "-=", 2},
    {Punctuator_AsteriskAssign, "*=", 2},
    {Punctuator_ForwardSlashAssign, "/=", 2},
    {Punctuator_Arrow, "->", 2},

    {Punctuator_Assign, "=", 1},
    {Punctuator_Ampersand, "&", 1},
    {Punctuator_GreaterThan, ">", 1},
    {Punctuator_LessThan, "<", 1},
    {Punctuator_OpenBrace, "{", 1},
    {Punctuator_CloseBrace, "}", 1},
    {Punctuator_OpenParen, "(", 1},
    {Punctuator_CloseParen, ")", 1},
    {Punctuator_OpenBracket, "[", 1},
    {Punctuator_CloseBracket, "]", 1},
    {Punctuator_Semicolon, ";", 1},
    {Punctuator_Comma, ",", 1},
    {Punctuator_Plus, "+", 1},
    {Punctuator_Minus, "-", 1},
    {Punctuator_Asterisk, "*", 1},
    {Punctuator_ForwardSlash, "/", 1},
    {Punctuator_Period, ".", 1},
};

static const char *punctuator_as_string(token_punctuator_t p) {
    for (size_t i = 0; i < sizeof(punctuators) / sizeof(punctuators[0]); i++) {
        if (punctuators[i].punctuator == p) {
            return punctuators[i].str;
        }
    }
    return "UNKNOWN_PUNCTUATOR";
}

static bool lexer_punctuation(lexer_state_t *state, token_t *next) {
    const char *start = state->unlexed;

    char c[2] = {peek(state, 0), peek(state, 1)};
    for (size_t i = 0; i < sizeof(punctuators) / sizeof(punctuators[0]); i++) {
        bool match = true;
        for (size_t j = 0; j < punctuators[i].len; j++) {
            if (c[j] != punctuators[i].str[j]) {
                match = false;
                break;
            }
        }

        if (!match) {
            continue;
        }

        *next =
            (token_t){.discrim = Token_Punctuator,
                      .punctuator = punctuators[i].punctuator,
                      .span = span(state, start, start + punctuators[i].len)};

        for (size_t j = 0; j < punctuators[i].len; j++) {
            advance(state);
        }

        return true;
    }
    printf("lexer: unexpected punctuator\n");
    exit(-1);
}

void lexer_print_token(FILE *f, token_t tok) {
    switch (tok.discrim) {
    case Token_Keyword:
        fprintf(f, "Token_Keyword    { .keyword = %s }\n",
                keyword_as_string(tok.keyword));
        break;
    case Token_Identifier:
        fprintf(f, "Token_Identifier { .str = \"%s\" }\n",
                ident_to_str(tok.ident));
        break;
    case Token_Constant:
        fprintf(f, "Token_Constant   { .str = \"%s\" }\n", tok.str);
        break;
    case Token_Punctuator:
        fprintf(f, "Token_Punctuator { .punctuator = %s }\n",
                punctuator_as_string(tok.punctuator));
        break;
    }
}

lexer_state_t lexer_new(struct ident_table *idents, const char *prog) {
    return (lexer_state_t){
        .idents = idents,

        .prog = prog,
        .unlexed = prog,

        .line = 0,
        .character = 0,
    };
}

bool lexer_next_token(lexer_state_t *state, token_t *next) {
    while (true) {
        char c = state->unlexed[0];

        if (c == '\0') {
            return false;
        }

        if (iswhitespace(c)) {
            advance(state);
        } else if (isdigit(c)) {
            return lexer_constant(state, next);
        } else if (isnondigit(c)) {
            return lexer_identifier_or_keyword(state, next);
        } else if (ispunctuation(c)) {
            return lexer_punctuation(state, next);
        } else {
            printf("lexer: unexpected char: %c\n", c);
            return false;
        }
    }
}
