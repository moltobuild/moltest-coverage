#include "coverage.h"
#include "moltest_api.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The plugin: a moltest reporter registered when the package is linked
 * (ADR 0001). Nothing else in a consumer has to mention it.
 *
 * on_run_start reads the configuration and erases the last run's data;
 * on_run_end flushes, collects, prints, writes the files and applies the
 * floors. A problem of the plugin's own (a bad config, no gcov) fails the run
 * through moltest_fail_run: a floor that silently stops being checked is worse
 * than a red build that says why.
 */

#define ERR_SIZE 512

static cov_config config;
static bool config_ok;
static char config_err[ERR_SIZE];
static char profile_dir[1024];
static bool have_profile_dir;

static void on_run_start(size_t files, size_t tests, void *ctx) {
    (void)files, (void)tests, (void)ctx;
    config_ok = cov_config_load(&config, config_err, sizeof config_err);
    have_profile_dir = cov_profile_dir(moltest_self_path(), profile_dir, sizeof profile_dir);
    if(have_profile_dir)
        cov_erase(profile_dir);
}

static void fail_runf(const char *format, ...) __attribute__((format(printf, 1, 2)));

static void fail_runf(const char *format, ...) {
    char reason[ERR_SIZE + 128];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(reason, sizeof reason, format, args);
    va_end(args);
    moltest_fail_run(reason);
}

static void write_output(const char *path, char *content, const char *what) {
    if(content == NULL) {
        fail_runf("moltest_coverage: out of memory rendering the %s report", what);
        return;
    }
    FILE *file = fopen(path, "wb");
    const bool written = file != NULL && fputs(content, file) >= 0;
    if(file != NULL && fclose(file) != 0)
        fail_runf("moltest_coverage: could not write %s", path);
    else if(!written)
        fail_runf("moltest_coverage: could not write %s (does its directory exist?)", path);
    free(content);
}

/* Fail the run when `percent` is under `floor` (a floor < 0 is unset). */
static void apply_floor(const char *what, const char *key, double percent, double floor) {
    if(floor < 0 || percent >= floor)
        return;
    fail_runf("moltest_coverage: %s coverage %.1f%% is under %s = %.1f by %.1f points", what,
              percent, key, floor, floor - percent);
}

static void on_run_end(const cov_moltest_summary *summary, void *ctx) {
    (void)summary, (void)ctx;
    if(!config_ok) {
        fail_runf("moltest_coverage: %s", config_err);
        return;
    }
    if(!have_profile_dir) {
        fail_runf("moltest_coverage: cannot tell where this test binary's build lives");
        return;
    }

    cov_report report = {0};
    char tool[COV_PATH_MAX] = "";
    char err[ERR_SIZE] = "";
    if(!cov_collect(&config, profile_dir, &report, tool, sizeof tool, err, sizeof err)) {
        fail_runf("moltest_coverage: %s", err);
        return;
    }

    if(report.count == 0) {
        printf("\nmoltest_coverage: no coverage data for %s%s; build the tests with "
               "--coverage (a profile with flags = [\"--coverage\"])\n",
               config.include[0], config.include_count > 1 ? " and the other includes" : "");
        if(config.fail_under >= 0 || config.fail_under_branches >= 0)
            fail_runf("moltest_coverage: a floor is set and nothing was measured");
        cov_report_free(&report);
        return;
    }

    char *text = cov_render_text(&report, tool);
    if(text != NULL)
        fputs(text, stdout);
    free(text);
    if(config.lcov[0] != '\0')
        write_output(config.lcov, cov_render_lcov(&report), "lcov");
    if(config.json[0] != '\0')
        write_output(config.json, cov_render_json(&report, tool), "JSON");

    const cov_totals t = cov_report_totals(&report);
    apply_floor("line", "fail_under", cov_percent(t.lines_hit, t.lines_total), config.fail_under);
    apply_floor("branch", "fail_under_branches", cov_percent(t.branches_hit, t.branches_total),
                config.fail_under_branches);
    cov_report_free(&report);
}

static const cov_moltest_reporter plugin = {
    .api_version = COV_MOLTEST_REPORTER_API,
    .name = "moltest_coverage",
    .on_run_start = on_run_start,
    .on_run_end = on_run_end,
};

__attribute__((constructor)) static void moltest_coverage_register(void) {
    moltest_add_reporter(&plugin);
}
