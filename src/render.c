#include "coverage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The three outputs: the terminal table, an lcov tracefile, a JSON summary. */

static int by_line(const void *a, const void *b) {
    const int left = ((const cov_line *)a)->line;
    const int right = ((const cov_line *)b)->line;
    return (left > right) - (left < right);
}

/* The file's executable lines in source order, as a copy the caller frees. */
static cov_line *sorted_lines(const cov_file *file) {
    cov_line *lines = malloc((file->line_count > 0 ? file->line_count : 1) * sizeof *lines);
    if (lines == NULL)
        return NULL;
    if (file->line_count > 0)
        memcpy(lines, file->lines, file->line_count * sizeof *lines);
    qsort(lines, file->line_count, sizeof *lines, by_line);
    return lines;
}

/*
 * Runs of unexecuted lines, the way coverage.py prints them: lines with no
 * code between two missing ones do not split a run, a line that ran does.
 */
char *cov_missing_ranges(const cov_file *file) {
    cov_text text = {0};
    cov_text_add(&text, "");
    cov_line *lines = sorted_lines(file);
    if (lines == NULL) {
        free(cov_text_take(&text));
        return NULL;
    }
    size_t i = 0;
    while (i < file->line_count) {
        if (lines[i].count > 0) {
            i++;
            continue;
        }
        const int start = lines[i].line;
        int end = start;
        while (i < file->line_count && lines[i].count == 0)
            end = lines[i++].line;
        if (text.length > 0)
            cov_text_add(&text, ", ");
        if (start == end)
            cov_text_addf(&text, "%d", start);
        else
            cov_text_addf(&text, "%d-%d", start, end);
    }
    free(lines);
    return cov_text_take(&text);
}

/* "112/140  80.0%", or "-" when there is nothing of that kind to cover. */
static void cell(char *out, size_t size, size_t hit, size_t total) {
    if (total == 0)
        snprintf(out, size, "-");
    else
        snprintf(out, size, "%zu/%zu %5.1f%%", hit, total, cov_percent(hit, total));
}

typedef struct {
    char lines[48], branches[48], functions[48];
} row_cells;

static row_cells cells_of(cov_totals t) {
    row_cells row;
    cell(row.lines, sizeof row.lines, t.lines_hit, t.lines_total);
    cell(row.branches, sizeof row.branches, t.branches_hit, t.branches_total);
    cell(row.functions, sizeof row.functions, t.functions_hit, t.functions_total);
    return row;
}

static size_t widest(size_t current, const char *text) {
    const size_t length = strlen(text);
    return length > current ? length : current;
}

char *cov_render_text(const cov_report *report, const char *tool) {
    cov_text text = {0};
    cov_text_addf(&text, "\nmoltest_coverage (%s)\n\n", tool);
    if (report->count == 0) {
        cov_text_add(&text, "no measured file has coverage data\n");
        return cov_text_take(&text);
    }

    const cov_totals total = cov_report_totals(report);
    const row_cells total_cells = cells_of(total);
    size_t w_file = widest(strlen("File"), "TOTAL");
    size_t w_lines = widest(strlen("Lines"), total_cells.lines);
    size_t w_branches = widest(strlen("Branches"), total_cells.branches);
    size_t w_functions = widest(strlen("Functions"), total_cells.functions);
    for (size_t i = 0; i < report->count; i++) {
        const row_cells c = cells_of(cov_file_totals(&report->files[i]));
        w_file = widest(w_file, report->files[i].path);
        w_lines = widest(w_lines, c.lines);
        w_branches = widest(w_branches, c.branches);
        w_functions = widest(w_functions, c.functions);
    }

    const int f = (int)w_file, l = (int)w_lines, b = (int)w_branches, fn = (int)w_functions;
    cov_text_addf(&text, "%-*s  %*s  %*s  %*s  %s\n", f, "File", l, "Lines", b, "Branches", fn,
                  "Functions", "Missing");
    for (size_t i = 0; i < report->count; i++) {
        const cov_file *file = &report->files[i];
        const row_cells c = cells_of(cov_file_totals(file));
        char *missing = cov_missing_ranges(file);
        cov_text_addf(&text, "%-*s  %*s  %*s  %*s  %s\n", f, file->path, l, c.lines, b,
                      c.branches, fn, c.functions, missing != NULL ? missing : "?");
        free(missing);
    }
    cov_text_addf(&text, "%-*s  %*s  %*s  %*s\n", f, "TOTAL", l, total_cells.lines, b,
                  total_cells.branches, fn, total_cells.functions);
    return cov_text_take(&text);
}

/*
 * An lcov tracefile, the format genhtml, Codecov and Coveralls read: one
 * record per file, functions, branches and lines, each followed by its
 * found/hit counts. gcov's text names no basic blocks, so every branch is in
 * block 0 and numbered by its position on the line, as lcov's own geninfo
 * does when it reads the same text.
 */
char *cov_render_lcov(const cov_report *report) {
    cov_text text = {0};
    cov_text_add(&text, "");
    for (size_t i = 0; i < report->count; i++) {
        const cov_file *file = &report->files[i];
        const cov_totals t = cov_file_totals(file);
        cov_text_addf(&text, "TN:\nSF:%s\n", file->path);
        for (size_t k = 0; k < file->function_count; k++)
            cov_text_addf(&text, "FN:%d,%s\n", file->functions[k].line, file->functions[k].name);
        for (size_t k = 0; k < file->function_count; k++)
            cov_text_addf(&text, "FNDA:%lld,%s\n", file->functions[k].count,
                          file->functions[k].name);
        cov_text_addf(&text, "FNF:%zu\nFNH:%zu\n", t.functions_total, t.functions_hit);
        for (size_t k = 0; k < file->branch_count; k++) {
            const cov_branch *br = &file->branches[k];
            if (br->executed)
                cov_text_addf(&text, "BRDA:%d,0,%d,%lld\n", br->line, br->index, br->taken);
            else
                cov_text_addf(&text, "BRDA:%d,0,%d,-\n", br->line, br->index);
        }
        cov_text_addf(&text, "BRF:%zu\nBRH:%zu\n", t.branches_total, t.branches_hit);
        cov_line *lines = sorted_lines(file);
        for (size_t k = 0; lines != NULL && k < file->line_count; k++)
            cov_text_addf(&text, "DA:%d,%lld\n", lines[k].line, lines[k].count);
        free(lines);
        cov_text_addf(&text, "LF:%zu\nLH:%zu\nend_of_record\n", t.lines_total, t.lines_hit);
    }
    return cov_text_take(&text);
}

/* A JSON string: quotes, backslashes and control characters escaped. */
static void add_json_string(cov_text *text, const char *value) {
    cov_text_add(text, "\"");
    for (const unsigned char *c = (const unsigned char *)value; *c != '\0'; c++) {
        if (*c == '"' || *c == '\\')
            cov_text_addf(text, "\\%c", *c);
        else if (*c < 0x20)
            cov_text_addf(text, "\\u%04x", *c);
        else
            cov_text_addf(text, "%c", *c);
    }
    cov_text_add(text, "\"");
}

static void add_figure(cov_text *text, const char *name, size_t hit, size_t total,
                       bool comma) {
    cov_text_addf(text, "\"%s\": {\"covered\": %zu, \"total\": %zu, \"percent\": %.2f}%s", name,
                  hit, total, cov_percent(hit, total), comma ? ", " : "");
}

static void add_figures(cov_text *text, cov_totals t) {
    add_figure(text, "lines", t.lines_hit, t.lines_total, true);
    add_figure(text, "branches", t.branches_hit, t.branches_total, true);
    add_figure(text, "functions", t.functions_hit, t.functions_total, false);
}

char *cov_render_json(const cov_report *report, const char *tool) {
    cov_text text = {0};
    cov_text_add(&text, "{\n  \"tool\": ");
    add_json_string(&text, tool);
    cov_text_add(&text, ",\n  \"totals\": {");
    add_figures(&text, cov_report_totals(report));
    cov_text_add(&text, "},\n  \"files\": [");
    for (size_t i = 0; i < report->count; i++) {
        const cov_file *file = &report->files[i];
        cov_text_add(&text, i == 0 ? "\n    {\"path\": " : ",\n    {\"path\": ");
        add_json_string(&text, file->path);
        cov_text_add(&text, ", ");
        add_figures(&text, cov_file_totals(file));
        cov_text_add(&text, ", \"missing_lines\": [");
        cov_line *lines = sorted_lines(file);
        bool first = true;
        for (size_t k = 0; lines != NULL && k < file->line_count; k++) {
            if (lines[k].count != 0)
                continue;
            cov_text_addf(&text, "%s%d", first ? "" : ", ", lines[k].line);
            first = false;
        }
        free(lines);
        cov_text_add(&text, "]}");
    }
    cov_text_add(&text, report->count > 0 ? "\n  ]\n}\n" : "]\n}\n");
    return cov_text_take(&text);
}
