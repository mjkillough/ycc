#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

static const struct {
    token_keyword_t keyword;
    const char *str;
} keywords[] = {
    {Keyword_int, "int"},
    {Keyword_return, "return"},
};

static bool lookup_keyword(const char *str, size_t len, token_keyword_t *out) {
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strncmp(str, keywords[i].str, len) == 0) {
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

static const char *punctuator_as_string(token_punctuator_t p) {
    switch (p) {
    case Punctuator_OpenBrace:
        return "{";
    case Punctuator_CloseBrace:
        return "}";
    case Punctuator_OpenParen:
        return "(";
    case Punctuator_CloseParen:
        return ")";
    case Punctuator_Semicolon:
        return ";";
    default:
        return "UNKNOWN";
    }
}

static bool iswhitespace(char c) { return c == ' ' || c == '\t'; }

static bool isnondigit(char c) { return c == '_' || isalpha(c); }

static bool ispunctuation(char c) {
    return c == '(' || c == ')' || c == '{' || c == '}' || c == ';';
}

typedef enum {
    State_Initial,
    // Parsing an identifier/keyword:
    State_Word,
    // Parsing an integer constant:
    State_Constant,
    // Parsing punctuation:
    State_Punctuation,
} state_t;

void lexer_print_token(token_t tok) {
    switch (tok.discrim) {
    case Token_Keyword:
        printf("Token_Keyword    { .keyword = %s }\n",
               keyword_as_string(tok.keyword));
        break;
    case Token_Identifier:
        printf("Token_Identifier { .str = \"%s\" }\n", tok.str);
        break;
    case Token_Constant:
        printf("Token_Constant   { .str = \"%s\" }\n", tok.str);
        break;
    case Token_Punctuator:
        printf("Token_Punctuator { .punctuator = %s }\n",
               punctuator_as_string(tok.punctuator));
        break;
    }
}

bool lexer_next_token(const char **prog, token_t *next) {
    const char *start = *prog;
    state_t state = State_Initial;

    while (true) {
        char c = (*prog)[0];

        if (c == '\0') {
            return false;
        }

        switch (state) {
        case State_Initial:
            if (iswhitespace(c)) {
                (*prog)++;
            } else if (isdigit(c)) {
                (*prog)++;
                state = State_Constant;
            } else if (isnondigit(c)) {
                (*prog)++;
                state = State_Word;
            } else if (ispunctuation(c)) {
                state = State_Punctuation;
            } else {
                printf("lexer: unexpected char: %c\n", c);
                exit(-1);
            }

            break;

        case State_Constant:
            if (isdigit(c)) {
                (*prog)++;
            } else {
                size_t len = *prog - start;
                char *str = (char *)malloc(len);
                strncpy(str, start, len);

                *next = (token_t){
                    .discrim = Token_Constant,
                    .str = str,
                };
                return true;
            }

            break;

        case State_Word:
            if (isnondigit(c) || isdigit(c)) {
                (*prog)++;
            } else {
                size_t len = *prog - start;

                token_keyword_t keyword;
                if (lookup_keyword(start, len, &keyword)) {
                    *next = (token_t){
                        .discrim = Token_Keyword,
                        .keyword = keyword,
                    };
                    return true;
                } else {
                    char *str = (char *)malloc(len);
                    strncpy(str, start, len);

                    *next = (token_t){
                        .discrim = Token_Identifier,
                        .str = str,
                    };
                    return true;
                }
            }
            break;

        case State_Punctuation:
            (*prog)++;

            switch (c) {
            case ';':
                *next = (token_t){
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_Semicolon,
                };
                return true;
            case '{':
                *next = (token_t){
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_OpenBrace,
                };
                return true;
            case '}':
                *next = (token_t){
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_CloseBrace,
                };
                return true;
            case '(':
                *next = (token_t){
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_OpenParen,
                };
                return true;
            case ')':
                *next = (token_t){
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_CloseParen,
                };
                return true;
            default:
                printf("lexer: unexpected punctuator: '%c'\n", c);
                exit(-1);
            }
        }
    }
}
