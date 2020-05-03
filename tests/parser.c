#include <stdio.h>

#include "ast.h"
#include "parser.h"

#include "common.h"
#include "framework.h"
#include "snapshot.h"

static void parser_snapshotter(FILE *f, void *data) {
    const char *prog = (const char *)data;

    ast_program_t program;
    parse_result_t result = parser_parse(prog, &program);
    if (result.kind == Parse_Result_Error) {
        diag_print(prog, &result.diag);
        FAIL("FAILED TO PARSE", "");
    }

    struct pprint *pp = pprint_new(f);
    ast_pprint_program(pp, &program);
    pprint_free(pp);
}

#define PARSER_TEST(name, prog)                                                \
    TEST(name) { SNAPSHOT(&parser_snapshotter, prog); }

PARSER_TEST(main, "int main() {}")

PARSER_TEST(binops, "int main() {\n"
                    "int a = 1 + 2;\n"
                    "int b = 3 - 4;\n"
                    "int c = 5 * 6;\n"
                    "int d = 7 / 8;\n"
                    "int e = 1 == 2;\n"
                    "int f = 3 != 4;\n"
                    "int g = 5 <  6;\n"
                    "int h = 7 <= 8;\n"
                    "int i = 9 >  0;\n"
                    "int j = 1 >= 2;\n"
                    "}")

PARSER_TEST(decl, "int main() {\n"
                  "int a = 0;\n"
                  "}")

PARSER_TEST(assignops, "int main() {\n"
                       "a  = 0;\n"
                       "b += 1;\n"
                       "c -= 2;\n"
                       "d *= 3;\n"
                       "e /= 4;\n"
                       "}")

PARSER_TEST(expr_variables, "int main() {\n"
                            "int a = a + b;\n"
                            "}")

PARSER_TEST(stmt_return, "int main() {\n"
                         "return 0;\n"
                         "}")

PARSER_TEST(stmt_if, "int main() {\n"
                     "if (1 == 0) {\n"
                     //"    int i = 0;\n"
                     "} else {\n"
                     //"    int j = 1;\n"
                     "}\n"
                     "}")

PARSER_TEST(multiple_functions, "int main() {}\n"
                                "int other() {}\n")

// TODO: "void no_return_type() {}\n"
// TODO: "int arguments(int i) {}\n"
