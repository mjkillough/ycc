#include <stdio.h>

#include "ast.h"
#include "gen.h"
#include "lexer.h"
#include "parser.h"

int main() {
    const char *prog = "int main() { return 4 + 5; }";

    // token_t token;
    // while (lexer_next_token(&prog, &token)) {
    //     lexer_print_token(token);
    // }

    ast_program_t program;
    if (!parser_parse(prog, &program)) {
        printf("error parsing\n");
        return -1;
    }

    printf("---\n\n");
    printf("Source:\n");
    printf("%s\n", prog);

    printf("---\n\n");
    printf("AST:\n");
    ast_print_program(program);

    printf("---\n\n");
    printf("ASM:\n");
    gen_generate(stdout, program);

    FILE *f = fopen("out.s", "w");
    if (f == NULL) {
        printf("error: opening out.s\n");
        return -1;
    }
    gen_generate(f, program);
    fclose(f);

    return 0;
}
