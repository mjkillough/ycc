#pragma once

#include "lexer.h"

typedef struct {
    token_span_t span;
    const char *msg;
} diag_t;

void diag_print(const char *prog, diag_t *diag);
