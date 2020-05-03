#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "framework.h"
#include "snapshot.h"

TEST(should_fail) {
    printf("failed!\n");
    ASSERT(false);
}

TEST(should_pass) { printf("passed!\n"); }

TEST(should_crash) {
    printf("crashed!\n");
    raise(SIGSEGV);
}

static void snapshotter(FILE *f, void *data) {
    UNUSED(data);
    fprintf(f, "hello, world\n");
}

TEST(snapshot_test) { SNAPSHOT(&snapshotter, NULL); }
