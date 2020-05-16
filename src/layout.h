#pragma once

#include <stdlib.h>

#include "pprint.h"
#include "ty.h"

size_t alignment_padding(size_t base, size_t alignment);

struct layout;

struct layout *layout_ty(struct ty *ty);

void layout_pprint(struct pprint *pp, struct layout *layout);

