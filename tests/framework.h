#pragma once

#include <errno.h>
#include <stdlib.h>

// Some inspiration from https://github.com/Snaipe/Criterion

// Set to the current test name before the execution of each test:
extern const char *__test_name;

#define HANDLE_ERROR(msg)                                                      \
    do {                                                                       \
        printf("%s (%s:%i): %s\n", msg, __FILE__, __LINE__, strerror(errno));  \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

#define FAIL(msg1, msg2)                                                       \
    do {                                                                       \
        printf("\033[1;31m" msg1 " (%s:%i): \033[0m", __FILE__, __LINE__);     \
        printf("%s\n", msg2);                                                  \
        exit(1);                                                               \
    } while (0)

#define ASSERT(condition)                                                      \
    do {                                                                       \
        if (!(condition)) {                                                    \
            FAIL("assertion failed", #condition);                              \
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
