#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    Punctuator_OpenParen,
    Punctuator_CloseParen,
    Punctuator_OpenBrace,
    Punctuator_CloseBrace,
} token_punctuator_t;

const char* punctuator_as_string(token_punctuator_t p) {
    switch (p) {
    case Punctuator_OpenBrace:
        return "\"{\"";
    case Punctuator_CloseBrace:
        return "\"}\"";
    case Punctuator_OpenParen:
        return "\"(\"";
    case Punctuator_CloseParen:
        return "\")\"";
    case Punctuator_Semicolon:
        return "\";\"";
    default:
        return "UNKNOWN";
    }
}

typedef struct {
    token_discrim_t discrim;

    union {
        // Token_Keyword
        token_keyword_t keyword;
        // Token_Identifier, Token_Constant
        const char* str;
        // Token_Punctuator
        token_punctuator_t punctuator;
    };
} token_t;

bool iswhitespace(char c) {
    return c == ' ' || c == '\t';
}

bool isnondigit(char c) {
    return c == '_' || isalpha(c);
}

bool ispunctuation(char c) {
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

void emit(token_t tok) {
    switch (tok.discrim) {
        // case Token_Keyword:
        //     printf("Token_Keyword    { .str = %s }\n");
        //     break;
        case Token_Identifier:
            printf("Token_Identifier { .str = \"%s\" }\n", tok.str);
            break;
        case Token_Constant:
            printf("Token_Constant   { .str = \"%s\" }\n", tok.str);
            break;
        case Token_Punctuator:
            printf("Token_Punctuator { .punctuator = %s }\n", punctuator_as_string(tok.punctuator));
            break;
    }
}

int main() {
    const char* prog = "int main() { return 0; }";
    size_t len = strlen(prog);
    int start = 0;
    int idx = 0;
    state_t state = State_Initial;

    while (true) {
        if (idx == len) {
            break;
        }

        char c = prog[idx];

        switch (state) {
        case State_Initial:
            start = idx;

            if (iswhitespace(c)) {
                idx++;
            } else if (isdigit(c)) {
                idx++;
                state = State_Constant;
            } else if (isnondigit(c)) {
                idx++;
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
                idx++;
            } else {
                size_t len = idx - start;
                char* str = (char*)malloc(len);
                strncpy(str, &prog[start], len);

                emit(token_t{
                    .discrim = Token_Constant,
                    .str = str,
                });

                state = State_Initial;
            }

            break;

        case State_Word:
            if (isnondigit(c) || isdigit(c)) {
                idx++;
            } else {
                size_t len = idx - start;
                char* str = (char*)malloc(len);
                strncpy(str, &prog[start], len);

                emit(token_t{
                    .discrim = Token_Identifier,
                    .str = str,
                });

                state = State_Initial;
            }

            break;

        case State_Punctuation:
            switch (c) {
            case ';':
                emit(token_t{
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_Semicolon,
                });

                idx++;
                state = State_Initial;
                break;
            case '{':
                emit(token_t{
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_OpenBrace,
                });

                idx++;
                state = State_Initial;
                break;
            case '}':
                emit(token_t{
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_CloseBrace,
                });

                idx++;
                state = State_Initial;
                break;
            case '(':
                emit(token_t{
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_OpenParen,
                });

                idx++;
                state = State_Initial;
                break;
            case ')':
                emit(token_t{
                    .discrim = Token_Punctuator,
                    .punctuator = Punctuator_CloseParen,
                });

                idx++;
                state = State_Initial;
                break;
            default:
                printf("lexer: unexpected punctuator: '%c'\n", c);
                exit(-1);
            }
        }
    }
}