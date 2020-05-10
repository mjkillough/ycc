#include <stdio.h>

#include "ast.h"
#include "ident.h"
#include "parser.h"

#include "common.h"
#include "framework.h"
#include "snapshot.h"

static void lsnapshotter(FILE *f, void *data) {
    const char *prog = (const char *)data;

    struct ident_table *idents = ident_table_new();

    lexer_state_t state = lexer_new(idents, prog);
    token_t token;
    while (lexer_next_token(&state, &token)) {
        lexer_print_token(f, token);
    }

    ident_table_free(idents);
}

#define LEXER_TEST(name, prog)                                                 \
    TEST(name) { SNAPSHOT(&lsnapshotter, prog); }

LEXER_TEST(punctuator, ". ->\n"
                       "; ,\n"
                       "&\n"
                       "= += -= *= /=\n"
                       "== != > >= < <=\n"
                       "+ - * /\n"
                       "( ) { }\n")

LEXER_TEST(keywords, "int return if else struct\n")

LEXER_TEST(identifiers_not_keywords, "e i\n")

// TODO: Why is 0 skipped?
LEXER_TEST(constants, "1 123 0")

