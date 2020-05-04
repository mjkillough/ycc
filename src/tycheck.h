#pragma once

#include "ty.h"
#include "ast.h"

struct tycheck;

struct tycheck *tycheck_new();
void tycheck_free(struct tycheck *tyc);

void tycheck_check(struct tycheck *tyc, ast_program_t *prog);

