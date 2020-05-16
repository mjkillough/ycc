#include <stdio.h>

#include "ast.h"
#include "diag.h"
#include "gen.h"
#include "ident.h"
#include "lexer.h"
#include "parser.h"
#include "tycheck.h"

int main() {
    const char *prog = "int main() {\n"
                       "    int a = 1, b;\n"
                       "    b = 2;\n"
                       "    int c = a + b;\n"
                       "    short d = 0;\n"
                       "    struct { short a; int b; char c; };\n"
                       "    union { short a; int b; char c; }; \n"
                       "    return c;\n"
                       "}\n"
                       "\n";

    // lexer_state_t state = lexer_new(prog);
    // token_t token;
    // while (lexer_next_token(&state, &token)) {
    //     lexer_print_token(token);
    // }
    // return 0;

    struct ident_table *idents = ident_table_new();

    ast_program_t program;
    parse_result_t result = parser_parse(idents, prog, &program);
    if (result.kind == Parse_Result_Error) {
        diag_print(prog, &result.diag);
        return -1;
    }

    struct tycheck *tyc = tycheck_new();
    tycheck_check(tyc, &program);
    tycheck_free(tyc);

    printf("---\n\n");
    printf("Source:\n");
    printf("%s\n", prog);

    printf("---\n\n");
    printf("AST:\n");

    struct pprint *pp = pprint_new(stdout);
    ast_pprint_program(pp, &program);
    pprint_free(pp);

    /* printf("---\n\n"); */
    /* printf("ASM:\n"); */
    /* gen_generate(stdout, program); */

    FILE *f = fopen("out.s", "w");
    if (f == NULL) {
        printf("error: opening out.s\n");
        return -1;
    }
    gen_generate(f, program);
    fclose(f);

    return 0;
}
