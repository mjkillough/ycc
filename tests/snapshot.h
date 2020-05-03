#pragma once

#include <stdio.h>

// Some inspiration from https://github.com/mitsuhiko/insta

// Snapshot test. One per TEST function.
#define SNAPSHOT(printer, data)                                                \
    snapshot_test(__FILE__, __test_name, printer, data)

void snapshot_test(const char *file, const char *name,
                   void (*printer)(FILE *f, void *data), void *data);

void snapshot_review(void);
