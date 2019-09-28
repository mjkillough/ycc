#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "framework.h"

#define OUTPUT_BUFFER_SIZE 512
#define RESULT_PADDING 15

#define FOR_TEST(var, body)                                                    \
    do {                                                                       \
        for (struct test *var = &__start_tests; test < &__stop_tests;          \
             test++) {                                                         \
            body                                                               \
        }                                                                      \
    } while (0)

extern struct test __start_tests;
extern struct test __stop_tests;

struct output {
    char *data;
    size_t length;
};

static void free_output(struct output output) { free(output.data); }

// Reads fd until error or EOF, returning the output.
// The caller is responsible for calling free_output() on the output.
static struct output read_output(int fd) {
    size_t total = OUTPUT_BUFFER_SIZE;
    size_t remaining = total;
    char *output = malloc(total);

    while (true) {
        ssize_t bytes = read(fd, output, remaining);
        if (bytes == 0) {
            break; // EOF
        } else if (bytes < 0) {
            // EINTR?
            perror("read");
            exit(-1);
        }

        remaining -= bytes;
        if (!remaining) {
            remaining = total;
            total *= 2;
            output = realloc(output, total);
        }
    }

    total -= remaining;
    output = realloc(output, total);
    return (struct output){output, total};
}

enum status {
    kStatusOk,
    kStatusFail,
    kStatusCrash,
};

static enum status check_status(pid_t pid) {
    int wstatus = 0;
    waitpid(pid, &wstatus, 0);
    if (WIFEXITED(wstatus)) {
        if (WEXITSTATUS(wstatus) == 0) {
            return kStatusOk;
        } else {
            return kStatusFail;
        }
    } else {
        return kStatusCrash;
    }
}

struct result {
    struct test *test;
    struct output output;
    enum status status;
};

static struct result *new_result() {
    return (struct result *)malloc(sizeof(struct result));
}

static void free_result(struct result *result) {
    free_output(result->output);
    free(result);
}

static void redirect_io(int fd) {
    while (dup2(fd, STDOUT_FILENO) == -1 && errno == EINTR) {
    }
    while (dup2(fd, STDERR_FILENO) == -1 && errno == EINTR) {
    }
}

// Runs provided test in a subprocess, capturing output and returning the
// result.
static void run_test(struct test *test, struct result *result) {
    result->test = test;

    int pipefd[2] = {0};
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(-1);
    }

    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(-1);
    } else if (pid == 0) {
        close(pipefd[0]);
        redirect_io(pipefd[1]);

        test->fn();

        exit(0);
    }

    close(pipefd[1]);

    result->output = read_output(pipefd[0]);
    result->status = check_status(pid);

    close(pipefd[0]);
}

static void print_padding(const char *str, size_t padding) {
    size_t len = strlen(str);
    for (size_t i = len; i < padding; i++) {
        printf(" ");
    }
}

static void print_status(enum status status) {
    switch (status) {
    case kStatusOk:
        printf("\033[1;32mPASS\033[0m");
        break;
    case kStatusFail:
        printf("\033[1;31mFAIL\033[0m");
        break;
    case kStatusCrash:
        printf("\033[1;35mCRASH\033[0m");
        break;
    }
}

static void print_output(struct test *test, struct result *result) {
    printf("--- ");
    print_status(result->status);
    printf(" %s: %s ---\n", test->file, test->name);
    printf("%.*s\n", (int)result->output.length, result->output.data);
}

// Runs all tests linked into executable. Returns true if all tests pass, false
// otherwise.
static bool run_all_tests() {
    // Run tests, outputting result:
    const char *file = NULL;
    FOR_TEST(test, {
        // Assumes tests for the same file will be placed next to each other in
        // memory.
        if (file == NULL || strcmp(file, test->file)) {
            file = test->file;
            printf("running %s:\n", file);
        }

        printf("\t%s ", test->name);

        struct result *result = new_result();
        run_test(test, result);

        print_padding(test->name, RESULT_PADDING);
        print_status(result->status);
        printf("\n");

        test->data = (void *)result;
    });

    printf("\n");

    // Display any errors and free results:
    size_t passes = 0, fails = 0, crashes = 0;
    FOR_TEST(test, {
        struct result *result = (struct result *)test->data;

        switch (result->status) {
        case kStatusOk:
            passes++;
            break;
        case kStatusFail:
            fails++;
            print_output(test, result);
            break;
        case kStatusCrash:
            crashes++;
            print_output(test, result);
            break;
        }

        free_result(result);
    });

    printf("\033[1;32mPASSES\033[0m: %li ", passes);
    printf("\033[1;31mFAILS\033[0m: %li ", fails);
    printf("\033[1;35mCRASHES\033[0m: %li\n", crashes);

    return !(fails > 0 || crashes > 0);
}

int main() { return run_all_tests() ? 0 : -1; }
