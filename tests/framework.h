#pragma once

#include <stdlib.h>

// Some inspiration from https://github.com/Snaipe/Criterion

#define ASSERT(condition)                                                      \
    do {                                                                       \
        if (!(condition)) {                                                    \
            printf("\033[1;31massertion failed: \033[0m");                     \
            printf(#condition);                                                \
            printf(" (%s:%i)\n", __FILE__, __LINE__);                          \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

struct test {
    const char *file;
    const char *name;
    void (*fn)(void);
    // Used by test runner to store context:
    void *data;
};

#define TEST(identifier)                                                       \
    void test_fn_##identifier(void);                                           \
    struct test test_##identifier __attribute__((__section__("tests"))) = {    \
        .file = __FILE__,                                                      \
        .name = #identifier,                                                   \
        .fn = &test_fn_##identifier,                                           \
        .data = NULL,                                                          \
    };                                                                         \
    void test_fn_##identifier(void)
