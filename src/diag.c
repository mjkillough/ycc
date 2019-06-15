#include <stdio.h>
#include <string.h>

#include "diag.h"
#include "lexer.h"

void diag_print(const char *prog, diag_t *diag) {
    printf("---\n");

    printf("%.*s", (int)(diag->span.start - prog), prog);
    printf("\033[1;31m%.*s\033[0m",
           (int)(diag->span.end - diag->span.start) + 1, diag->span.start);
    printf("%s\n\n", diag->span.end + 1);

    printf("Line %d, Column %d\n", diag->span.line, diag->span.character);
    printf("Error: %s\n", diag->msg);

    printf("---\n");
}
