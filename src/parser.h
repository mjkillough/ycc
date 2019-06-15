#pragma once

#include <stdbool.h>

#include "diag.h"

typedef struct {
    enum {
        Parse_Result_Ok,
        Parse_Result_Error,
    } kind;
    // Parse_Result_Error:
    diag_t diag;
} parse_result_t;

parse_result_t parser_parse(const char *prog, ast_program_t *program);
