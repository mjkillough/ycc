#pragma once

#include <stdio.h>

struct pprint;

struct pprint *pprint_new(FILE *f);
void pprint_free(struct pprint *pp);

void pprintf(struct pprint *pp, const char *fmt, ...);
void pprint_newline(struct pprint *pp);
void pprint_indent(struct pprint *pp);
void pprint_unindent(struct pprint *pp);

