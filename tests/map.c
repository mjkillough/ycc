#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "framework.h"

TEST(should_fail) {
    printf("failed!\n");
    ASSERT(false);
}

TEST(should_pass) { printf("passed!\n"); }

TEST(should_crash) {
    printf("crashed!\n");
    raise(SIGSEGV);
}
