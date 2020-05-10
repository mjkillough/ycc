#pragma once

#include <stdlib.h>

struct ident;
struct ident_table;

struct ident_table *ident_table_new();
void ident_table_free(struct ident_table *t);

size_t ident_table_len(struct ident_table *t);

struct ident *ident_from_str(struct ident_table *t, const char *str);
const char *ident_to_str(struct ident *ident);

