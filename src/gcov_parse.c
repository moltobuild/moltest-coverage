#include "coverage.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The annotated text gcov and `llvm-cov gcov` print with `-b -c -t` (ADR 0002).
 *
 *         -:    0:Source:src/cov.c
 *   function cov_add called 2 returned 100% blocks executed 75%
 *         2:    2:int cov_add(int a, int b) {
 *         2:    3:    if (a < 0)
 *   branch  0 taken 2
 *   branch  1 never executed
 *     #####:    4:        return -1;
 *        1*:    5:    return a + b;
 *
 * A count of `-` is a line with no code, `#####` code that never ran, `=====`
 * code reached only by an exception, and `N*` a line that ran with some block
 * of it unexecuted. One run of the tool may print several files back to back;
 * a file printed twice (a header pulled in by two objects) is merged.
 */

#define TEXT_LINE_MAX 4096

static bool grow(void **items, size_t count, size_t *capacity, size_t size) {
    if (count < *capacity)
        return true;
    const size_t next = *capacity == 0 ? 32 : *capacity * 2;
    void *grown = realloc(*items, next * size);
    if (grown == NULL)
        return false;
    *items = grown;
    *capacity = next;
    return true;
}

static char *copy(const char *text) {
    const size_t size = strlen(text) + 1;
    char *out = malloc(size);
    if (out != NULL)
        memcpy(out, text, size);
    return out;
}

void cov_report_free(cov_report *report) {
    for (size_t i = 0; i < report->count; i++) {
        cov_file *file = &report->files[i];
        for (size_t f = 0; f < file->function_count; f++)
            free(file->functions[f].name);
        free(file->path);
        free(file->lines);
        free(file->branches);
        free(file->functions);
    }
    free(report->files);
    *report = (cov_report){0};
}

/* Forward slashes, no leading "./": the form include/exclude are written in. */
static void normalize(char *path) {
    for (char *c = path; *c != '\0'; c++) {
        if (*c == '\\')
            *c = '/';
    }
    while (path[0] == '.' && path[1] == '/')
        memmove(path, path + 2, strlen(path + 2) + 1);
}

/* The file named `path`, added when new. `existed` says which. */
static cov_file *file_named(cov_report *report, const char *path, bool *existed) {
    for (size_t i = 0; i < report->count; i++) {
        if (strcmp(report->files[i].path, path) == 0) {
            *existed = true;
            return &report->files[i];
        }
    }
    *existed = false;
    if (!grow((void **)&report->files, report->count, &report->capacity, sizeof *report->files))
        return NULL;
    cov_file *file = &report->files[report->count];
    *file = (cov_file){.path = copy(path)};
    if (file->path == NULL)
        return NULL;
    report->count++;
    return file;
}

static bool add_line(cov_file *file, bool merging, int number, long long count) {
    if (merging) {
        for (size_t i = 0; i < file->line_count; i++) {
            if (file->lines[i].line == number) {
                file->lines[i].count += count;
                return true;
            }
        }
    }
    if (!grow((void **)&file->lines, file->line_count, &file->line_capacity, sizeof *file->lines))
        return false;
    file->lines[file->line_count++] = (cov_line){.line = number, .count = count};
    return true;
}

static bool add_branch(cov_file *file, bool merging, int line, int index, bool executed,
                       long long taken) {
    if (merging) {
        for (size_t i = 0; i < file->branch_count; i++) {
            cov_branch *b = &file->branches[i];
            if (b->line == line && b->index == index) {
                b->executed = b->executed || executed;
                b->taken += taken;
                return true;
            }
        }
    }
    if (!grow((void **)&file->branches, file->branch_count, &file->branch_capacity,
              sizeof *file->branches))
        return false;
    file->branches[file->branch_count++] =
        (cov_branch){.line = line, .index = index, .taken = taken, .executed = executed};
    return true;
}

static bool add_function(cov_file *file, const char *name, int line, long long count) {
    for (size_t i = 0; i < file->function_count; i++) {
        if (strcmp(file->functions[i].name, name) == 0) {
            file->functions[i].count += count;
            if (file->functions[i].line == 0)
                file->functions[i].line = line;
            return true;
        }
    }
    if (!grow((void **)&file->functions, file->function_count, &file->function_capacity,
              sizeof *file->functions))
        return false;
    char *owned = copy(name);
    if (owned == NULL)
        return false;
    file->functions[file->function_count++] =
        (cov_function){.name = owned, .line = line, .count = count};
    return true;
}

/* A count field: "-" (no code, false), "#####"/"=====" (0), "12" or "12*". */
static bool read_count(const char *field, long long *count) {
    while (*field == ' ' || *field == '\t')
        field++;
    if (strncmp(field, "#####", 5) == 0 || strncmp(field, "=====", 5) == 0) {
        *count = 0;
        return true;
    }
    if (!isdigit((unsigned char)*field))
        return false;
    char *end = NULL;
    *count = strtoll(field, &end, 10);
    return true;
}

static bool fail(char *err, size_t err_size, int number, const char *what) {
    if (err != NULL && err_size > 0)
        snprintf(err, err_size, "gcov output line %d: %s", number, what);
    return false;
}

bool cov_parse_gcov(const char *text, cov_report *report, char *err, size_t err_size) {
    cov_file *file = NULL;
    bool merging = false;
    int current_line = 0;      /* the source line branches belong to */
    int branch_index = 0;      /* position among that line's branches */
    char pending_function[512] = "";
    long long pending_count = 0;
    bool have_pending = false;
    int number = 0;

    const char *p = text;
    while (*p != '\0') {
        number++;
        const char *eol = strchr(p, '\n');
        size_t length = eol == NULL ? strlen(p) : (size_t)(eol - p);
        if (length >= TEXT_LINE_MAX)
            return fail(err, err_size, number, "line too long");
        char line[TEXT_LINE_MAX];
        memcpy(line, p, length);
        line[length] = '\0';
        if (length > 0 && line[length - 1] == '\r')
            line[--length] = '\0';
        p = eol == NULL ? p + length : eol + 1;

        if (strncmp(line, "function ", 9) == 0) {
            /* "function NAME called N returned ..." — its line is the next one. */
            char name[512];
            long long called = 0;
            if (sscanf(line + 9, "%511s called %lld", name, &called) != 2)
                return fail(err, err_size, number, "unreadable function record");
            snprintf(pending_function, sizeof pending_function, "%s", name);
            pending_count = called;
            have_pending = true;
            continue;
        }
        if (strncmp(line, "branch ", 7) == 0) {
            if (file == NULL)
                return fail(err, err_size, number, "branch before any source file");
            const bool never = strstr(line, "never executed") != NULL;
            long long taken = 0;
            const char *at = strstr(line, "taken ");
            if (!never) {
                if (at == NULL)
                    return fail(err, err_size, number, "unreadable branch record");
                taken = strtoll(at + 6, NULL, 10);
            }
            if (!add_branch(file, merging, current_line, branch_index++, !never, taken))
                return fail(err, err_size, number, "out of memory");
            continue;
        }

        /* "<count>:<line>:<source>" */
        char *first = strchr(line, ':');
        if (first == NULL)
            continue; /* "call ...", "unconditional ...", anything else */
        char *second = strchr(first + 1, ':');
        if (second == NULL)
            continue;
        *first = '\0';
        *second = '\0';
        const int source_line = atoi(first + 1);
        const char *rest = second + 1;

        if (source_line == 0) {
            if (strncmp(rest, "Source:", 7) == 0) {
                char path[1024];
                snprintf(path, sizeof path, "%s", rest + 7);
                normalize(path);
                file = file_named(report, path, &merging);
                if (file == NULL)
                    return fail(err, err_size, number, "out of memory");
                have_pending = false;
            }
            continue;
        }
        if (file == NULL)
            return fail(err, err_size, number, "source line before any source file");

        current_line = source_line;
        branch_index = 0;
        if (have_pending) {
            if (!add_function(file, pending_function, source_line, pending_count))
                return fail(err, err_size, number, "out of memory");
            have_pending = false;
        }
        long long count = 0;
        if (!read_count(line, &count))
            continue; /* "-": no code on this line */
        if (!add_line(file, merging, source_line, count))
            return fail(err, err_size, number, "out of memory");
    }
    return true;
}

double cov_percent(size_t hit, size_t total) {
    return total == 0 ? 100.0 : (double)hit * 100.0 / (double)total;
}

cov_totals cov_file_totals(const cov_file *file) {
    cov_totals t = {0};
    t.lines_total = file->line_count;
    for (size_t i = 0; i < file->line_count; i++)
        t.lines_hit += file->lines[i].count > 0 ? 1 : 0;
    t.branches_total = file->branch_count;
    for (size_t i = 0; i < file->branch_count; i++)
        t.branches_hit += file->branches[i].executed && file->branches[i].taken > 0 ? 1 : 0;
    t.functions_total = file->function_count;
    for (size_t i = 0; i < file->function_count; i++)
        t.functions_hit += file->functions[i].count > 0 ? 1 : 0;
    return t;
}

cov_totals cov_report_totals(const cov_report *report) {
    cov_totals sum = {0};
    for (size_t i = 0; i < report->count; i++) {
        const cov_totals t = cov_file_totals(&report->files[i]);
        sum.lines_hit += t.lines_hit;
        sum.lines_total += t.lines_total;
        sum.branches_hit += t.branches_hit;
        sum.branches_total += t.branches_total;
        sum.functions_hit += t.functions_hit;
        sum.functions_total += t.functions_total;
    }
    return sum;
}

void cov_report_filter(cov_report *report, const cov_config *config) {
    size_t kept = 0;
    for (size_t i = 0; i < report->count; i++) {
        if (cov_config_measures(config, report->files[i].path)) {
            report->files[kept++] = report->files[i];
            continue;
        }
        cov_file *dropped = &report->files[i];
        for (size_t f = 0; f < dropped->function_count; f++)
            free(dropped->functions[f].name);
        free(dropped->path);
        free(dropped->lines);
        free(dropped->branches);
        free(dropped->functions);
    }
    report->count = kept;
}

static int by_line(const void *a, const void *b) {
    const int left = ((const cov_line *)a)->line;
    const int right = ((const cov_line *)b)->line;
    return (left > right) - (left < right);
}

static int worst_first(const void *a, const void *b) {
    const cov_totals left = cov_file_totals(a);
    const cov_totals right = cov_file_totals(b);
    const double lp = cov_percent(left.lines_hit, left.lines_total);
    const double rp = cov_percent(right.lines_hit, right.lines_total);
    if (lp != rp)
        return lp < rp ? -1 : 1;
    return strcmp(((const cov_file *)a)->path, ((const cov_file *)b)->path);
}

void cov_report_sort(cov_report *report) {
    for (size_t i = 0; i < report->count; i++)
        qsort(report->files[i].lines, report->files[i].line_count, sizeof(cov_line), by_line);
    qsort(report->files, report->count, sizeof *report->files, worst_first);
}
