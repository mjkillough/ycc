#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "ast.h"

bool gen_generate(FILE *f, ast_program_t ast);
