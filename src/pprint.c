#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct pprint {
    FILE *f;
    bool newline;
    size_t indent;
};

struct pprint *pprint_new(FILE *f) {
    struct pprint *pp = calloc(1, sizeof(struct pprint));
    pp->f = f;
    return pp;
}

void pprint_free(struct pprint *pp) { free(pp); }

void pprintf(struct pprint *pp, const char *fmt, ...) {
    if (pp->newline) {
        for (size_t i = 0; i < pp->indent; i++) {
            fprintf(pp->f, "    ");
        }
    }
    pp->newline = false;

    va_list args;
    va_start(args, fmt);
    vfprintf(pp->f, fmt, args);
    va_end(args);
}

void pprint_newline(struct pprint *pp) {
    fprintf(pp->f, "\n");
    pp->newline = true;
}

void pprint_indent(struct pprint *pp) { pp->indent++; }
void pprint_unindent(struct pprint *pp) { pp->indent--; }

