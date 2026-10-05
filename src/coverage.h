#ifndef MOLTEST_COVERAGE_INTERNAL_H
#define MOLTEST_COVERAGE_INTERNAL_H

/*
 * moltest-coverage's internals: one header for the units in src/ and for the
 * tests that exercise them. Nothing here is a public interface; a consumer
 * only links the package and the constructor in register.c does the rest.
 */

#include <stdbool.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Text                                                                 */
/* ------------------------------------------------------------------ */

/* A growable string. `data` is always NUL-terminated once anything was added. */
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    bool failed; /* an allocation failed; the content is incomplete */
} cov_text;

void cov_text_add(cov_text *text, const char *piece);
void cov_text_addf(cov_text *text, const char *format, ...) __attribute__((format(printf, 2, 3)));
/* Hand the string over (heap, caller frees), or NULL if building it failed. */
[[nodiscard]] char *cov_text_take(cov_text *text);

/* ------------------------------------------------------------------ */
/* Configuration: moltest-coverage.toml (ADR 0003)                      */
/* ------------------------------------------------------------------ */

#define COV_CONFIG_FILE "moltest-coverage.toml"
#define COV_PATTERNS_MAX 16
#define COV_PATH_MAX 256

typedef struct {
    char include[COV_PATTERNS_MAX][COV_PATH_MAX]; /* default: "src" */
    size_t include_count;
    char exclude[COV_PATTERNS_MAX][COV_PATH_MAX];
    size_t exclude_count;
    double fail_under;          /* lines, percent; < 0 when not set */
    double fail_under_branches; /* < 0 when not set */
    char lcov[COV_PATH_MAX];    /* "" when not written */
    char json[COV_PATH_MAX];
    char tool[COV_PATH_MAX]; /* "" = derived from the compiler */
} cov_config;

/* The configuration an absent file means. */
void cov_config_defaults(cov_config *config);

/* Parse `text`, named `source` in messages, over the defaults. False with
   "<source>:<line>: <what>" in `err` for anything outside the subset. */
[[nodiscard]] bool cov_config_parse(const char *text, const char *source, cov_config *config,
                                    char *err, size_t err_size);

/* Read COV_CONFIG_FILE from the current directory; defaults when absent. */
[[nodiscard]] bool cov_config_load(cov_config *config, char *err, size_t err_size);

/* Whether `path` (relative to the project root, '/' separated) is measured:
   under an include and under no exclude. */
[[nodiscard]] bool cov_config_measures(const cov_config *config, const char *path);

/* ------------------------------------------------------------------ */
/* Coverage data                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    int line;
    long long count;
} cov_line;

typedef struct {
    int line;
    int index; /* position among the branches gcov listed for that line */
    long long taken;
    bool executed; /* false: "never executed" */
} cov_branch;

typedef struct {
    char *name;
    int line;
    long long count;
} cov_function;

typedef struct {
    char *path;
    cov_line *lines;
    size_t line_count, line_capacity;
    cov_branch *branches;
    size_t branch_count, branch_capacity;
    cov_function *functions;
    size_t function_count, function_capacity;
} cov_file;

typedef struct {
    cov_file *files;
    size_t count, capacity;
} cov_report;

/* Covered and total, for one file or for the report. */
typedef struct {
    size_t lines_hit, lines_total;
    size_t branches_hit, branches_total;
    size_t functions_hit, functions_total;
} cov_totals;

void cov_report_free(cov_report *report);
[[nodiscard]] cov_totals cov_file_totals(const cov_file *file);
[[nodiscard]] cov_totals cov_report_totals(const cov_report *report);
/* Percent of hit over total; 100 when there is nothing to cover. */
[[nodiscard]] double cov_percent(size_t hit, size_t total);
/* Drop the files `config` does not measure. */
void cov_report_filter(cov_report *report, const cov_config *config);
/* Make every absolute path under `root` relative to it, so that include and
   exclude match however the build named its sources. `root` and the paths
   are compared with '/' separators, and a drive letter's case ignored. */
void cov_report_relativize(cov_report *report, const char *root);
/* Order the files worst line coverage first, then by path. */
void cov_report_sort(cov_report *report);

/* Parse the annotated text gcov or `llvm-cov gcov` prints with `-b -c -t`,
   any number of files one after another, into `report`; counts for a file
   already there are added (ADR 0002). */
[[nodiscard]] bool cov_parse_gcov(const char *text, cov_report *report, char *err, size_t err_size);

/* ------------------------------------------------------------------ */
/* Reports                                                              */
/* ------------------------------------------------------------------ */

/* The terminal table; `tool` names what produced the data. Heap string. */
[[nodiscard]] char *cov_render_text(const cov_report *report, const char *tool);
/* An lcov tracefile. Heap string. */
[[nodiscard]] char *cov_render_lcov(const cov_report *report);
/* The JSON summary. Heap string. */
[[nodiscard]] char *cov_render_json(const cov_report *report, const char *tool);
/* "12-14, 20": the executable lines of `file` that never ran. Heap string. */
[[nodiscard]] char *cov_missing_ranges(const cov_file *file);

/* Whether a configured output path stays inside the project: relative, and
   without a ".." component. */
[[nodiscard]] bool cov_output_path_is_safe(const char *path);

/* ------------------------------------------------------------------ */
/* Collection                                                           */
/* ------------------------------------------------------------------ */

/* Run `argv` (no shell), from the current directory, and return what it
   printed on stdout (heap), or NULL when it could not be run. `status` gets
   its exit status. */
[[nodiscard]] char *cov_run_capture(const char *const argv[], int *status);

/* The build directory of the running test binary's profile
   (`build/<profile>`), from its path `self`. */
[[nodiscard]] bool cov_profile_dir(const char *self, char *out, size_t size);

/* Flush the counters, find the data `config` measures under
   `<profile_dir>/obj`, run the gcov tool over it and parse what it prints.
   `instrumented` says whether the measured sources were compiled for coverage
   at all (their .gcno exist). `tool_used` names the tool. False with a reason
   when the tool fails; true with an empty report when there is no data. */
[[nodiscard]] bool cov_collect(const cov_config *config, const char *profile_dir,
                               cov_report *report, bool *instrumented, char *tool_used,
                               size_t tool_size, char *err, size_t err_size);

/* Delete every .gcda under `<profile_dir>/obj`: what a run reports is that
   run, not the sum of every run since the last clean build. */
void cov_erase(const char *profile_dir);

/* ------------------------------------------------------------------ */
/* Place in the run: molto RFC-0020 (ADR 0004)                          */
/* ------------------------------------------------------------------ */

#define COV_INDEX_VAR "MOLTO_TEST_INDEX"
#define COV_COUNT_VAR "MOLTO_TEST_COUNT"

/* Where this test executable stands in the run. `first` erases the counters
   of the last run; `last` reports and applies the floors. */
typedef struct {
    bool first;
    bool last;
    bool valid;   /* false: the variables were set and did not parse */
    size_t index; /* 0 when not told */
    size_t count;
} cov_position;

/* From the two variables' values, NULL when unset. Both unset, or anything
   that does not parse, is a run of its own: first and last at once. */
cov_position cov_position_parse(const char *index, const char *count);

#endif /* MOLTEST_COVERAGE_INTERNAL_H */
