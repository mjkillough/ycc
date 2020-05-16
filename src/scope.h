#pragma once

#include "ident.h"
#include "map.h"
#include "vec.h"

struct scope;

struct scope *scope_new();
struct scope *scope_new_child(struct scope *s);

void scope_declare(struct scope *s, struct ident *ident, void *value);
void *scope_get(struct scope *s, struct ident *ident);
void scope_take_ownership(struct scope *s, void *ptr);

