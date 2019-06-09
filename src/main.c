#include <stdio.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

int main() {
    const char *prog = "int main() { return 0; }";

    // token_t token;
    // while (lexer_next_token(&prog, &token)) {
    //     lexer_print_token(token);
    // }

    ast_program_t program;
    if (!parser_parse(prog, &program)) {
        printf("error parsing\n");
        return -1;
    }

    ast_print_program(program);

    return 0;
}
