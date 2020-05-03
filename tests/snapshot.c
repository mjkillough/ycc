#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ftw.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "framework.h"
#include "snapshot.h"

static char *snapshot_path(const char *file, const char *name) {
    char *buffer = NULL;
    size_t size = 0;

    FILE *stream = open_memstream(&buffer, &size);
    if (stream == NULL) {
        HANDLE_ERROR("open_memstream");
    }

    fprintf(stream, "snapshots/%s:%s.snapshot", file, name);
    fclose(stream);

    return buffer;
}

static char *snapshot_path_new(const char *path) {
    size_t len = strlen(path);
    size_t newlen = len + 4;
    char *new = malloc(newlen + 1);
    memcpy(new, path, len);
    memcpy(new + len, ".new", 4);
    new[newlen] = '\0';
    return new;
}

// Outputs the diff between file1 and file2. Outputs nothing if the two files
// are identical. Returns true if the files are different.
static bool diff(const char *file1, const char *file2) {
    char *buffer = NULL;
    size_t size = 0;

    FILE *stream = open_memstream(&buffer, &size);
    if (stream == NULL) {
        HANDLE_ERROR("open_memstream");
    }
    fprintf(stream, "diff %s %s", file1, file2);
    fclose(stream);

    int status = system(buffer);
    free(buffer);

    return status != 0;
}

// Creates an empty snapshot file if one does not exist.
static void ensure_snapshot(const char *path) {
    FILE *f = fopen(path, "a");
    if (f == NULL) {
        HANDLE_ERROR("fopen");
    }
    fclose(f);
}

static bool ensure_directory(const char *path) {
    char *buffer = NULL;
    size_t size = 0;

    FILE *stream = open_memstream(&buffer, &size);
    if (stream == NULL) {
        HANDLE_ERROR("open_memstream");
    }
    fprintf(stream, "mkdir -p %s", path);
    fclose(stream);

    int status = system(buffer);
    free(buffer);

    return status != 0;
}

void snapshot_test(const char *file, const char *name,
                   void (*printer)(FILE *f, void *data), void *data) {
    char *path_old = snapshot_path(file, name);
    char *path_new = snapshot_path_new(path_old);

    char *directory = dirname(path_old);
    if (directory == NULL) {
        HANDLE_ERROR("dirname");
    }
    if (ensure_directory(directory)) {
        HANDLE_ERROR("ensure_directory");
    }

    ensure_snapshot(path_old);

    FILE *fnew = fopen(path_new, "w");
    if (fnew == NULL) {
        HANDLE_ERROR("fopen");
    }
    printer(fnew, data);
    fclose(fnew);

    if (diff(path_old, path_new)) {
        FAIL("snapshots differ", name);
    } else {
        remove(path_new);
    }

    free(path_old);
    free(path_new);
}

static void review(const char *path_old, const char *path_new) {
    printf("--- \033[1;35mREVIEW:\033[0m %s ---\n\n", path_old);

    diff(path_old, path_new);

    printf("\n");
    while (true) {
        printf("\t\033[1;32ma\033[0m accept\n");
        printf("\t\033[1;31mr\033[0m reject\n");
        printf("\t\033[1;33ms\033[0m skip\n");
        printf("\n");

        int c = getchar();
        switch (c) {
        case 'a':
            rename(path_new, path_old);
            return;
        case 'r':
            remove(path_new);
            return;
        case 's':
            return;
        default:
            printf("\nunknown option: %c\n", c);
        }
    }
}

static bool ends_with(const char *str, const char *suffix) {
    size_t len = strlen(str);
    size_t slen = strlen(suffix);
    if (slen > len) {
        return false;
    }
    return strncmp(str + len - slen, ".snapshot", slen) == 0;
}

static int directory_walk(const char *path, const struct stat *sb, int tflag) {
    UNUSED(sb);

    // Ignore directories or files we can't read.
    if (tflag != FTW_F) {
        return 0;
    }

    // Ignore files that don't end in .snapshot (i.e. ignore .new!)
    if (!ends_with(path, ".snapshot")) {
        return 0;
    }

    // Ignore snapshots that don't have an equivalent .new.
    char *path_new = snapshot_path_new(path);
    if (access(path_new, F_OK)) {
        free(path_new);
        return 0;
    }

    review(path, path_new);
    free(path_new);

    return 0;
}

void snapshot_review(void) {
    if (ftw("snapshots", &directory_walk, 3)) {
        HANDLE_ERROR("ftw");
    }
}
