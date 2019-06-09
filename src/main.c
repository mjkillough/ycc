#include "lexer.h"

int main() {
    const char *prog = "int main() { return 0; }";

    token_t token;
    while (lexer_next_token(&prog, &token)) {
        lexer_print_token(token);
    }

    return 0;
}
