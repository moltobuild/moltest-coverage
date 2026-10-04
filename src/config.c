#include "coverage.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * moltest-coverage.toml: a TOML subset read without a TOML library (ADR 0003).
 *
 * Comments, blank lines and `key = value`, where a value is a string, a number,
 * a boolean or a one-line array of strings. No tables. Anything else, and any
 * key this file does not know, is an error naming the line: a misspelt
 * `fail_under` must stop the run rather than quietly measure nothing.
 */

#define LINE_MAX_LENGTH 1024

void cov_config_defaults(cov_config *config) {
    memset(config, 0, sizeof *config);
    snprintf(config->include[0], sizeof config->include[0], "src");
    config->include_count = 1;
    config->fail_under = -1.0;
    config->fail_under_branches = -1.0;
}

static bool fail(char *err, size_t err_size, const char *source, int line, const char *format,
                 ...) __attribute__((format(printf, 5, 6)));

static bool fail(char *err, size_t err_size, const char *source, int line, const char *format,
                 ...) {
    if (err == NULL || err_size == 0)
        return false;
    int written = snprintf(err, err_size, "%s:%d: ", source, line);
    if (written < 0 || (size_t)written >= err_size)
        return false;
    va_list args;
    va_start(args, format);
    (void)vsnprintf(err + written, err_size - (size_t)written, format, args);
    va_end(args);
    return false;
}

static const char *skip_spaces(const char *p) {
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

/* After a value: only spaces and an optional comment may follow. */
static bool only_comment_left(const char *p) {
    p = skip_spaces(p);
    return *p == '\0' || *p == '#';
}

/* A basic string starting at the opening quote; `*end` is set after the
   closing one. Handles \" and \\, which is all a path needs. */
static bool read_string(const char *p, char *out, size_t size, const char **end) {
    if (*p != '"')
        return false;
    p++;
    size_t length = 0;
    while (*p != '"') {
        if (*p == '\0')
            return false;
        char c = *p++;
        if (c == '\\') {
            if (*p != '"' && *p != '\\')
                return false;
            c = *p++;
        }
        if (length + 1 >= size)
            return false;
        out[length++] = c;
    }
    out[length] = '\0';
    *end = p + 1;
    return true;
}

/* `[ "a", "b" ]` on one line into `items`. */
static bool read_array(const char *p, char items[][COV_PATH_MAX], size_t max, size_t *count,
                       const char **end) {
    if (*p != '[')
        return false;
    p = skip_spaces(p + 1);
    *count = 0;
    if (*p == ']') {
        *end = p + 1;
        return true;
    }
    for (;;) {
        if (*count == max)
            return false;
        if (!read_string(p, items[*count], COV_PATH_MAX, &p))
            return false;
        (*count)++;
        p = skip_spaces(p);
        if (*p == ']') {
            *end = p + 1;
            return true;
        }
        if (*p != ',')
            return false;
        p = skip_spaces(p + 1);
        if (*p == ']') { /* a trailing comma, which TOML allows */
            *end = p + 1;
            return true;
        }
    }
}

static bool read_percent(const char *p, double *out, const char **end) {
    char *stop = NULL;
    errno = 0;
    const double value = strtod(p, &stop);
    if (stop == p || errno != 0 || value < 0.0 || value > 100.0)
        return false;
    *out = value;
    *end = stop;
    return true;
}

/* Paths in the config use '/', relative to the project root. */
static void trim_trailing_slashes(char *path) {
    size_t length = strlen(path);
    while (length > 1 && path[length - 1] == '/')
        path[--length] = '\0';
}

static bool apply(cov_config *config, const char *key, const char *value, const char *source,
                  int line, char *err, size_t err_size) {
    const char *end = value;
    if (strcmp(key, "include") == 0 || strcmp(key, "exclude") == 0) {
        const bool include = key[0] == 'i';
        char (*items)[COV_PATH_MAX] = include ? config->include : config->exclude;
        size_t *count = include ? &config->include_count : &config->exclude_count;
        if (!read_array(value, items, COV_PATTERNS_MAX, count, &end))
            return fail(err, err_size, source, line,
                        "'%s' must be an array of at most %d strings", key, COV_PATTERNS_MAX);
        for (size_t i = 0; i < *count; i++)
            trim_trailing_slashes(items[i]);
    } else if (strcmp(key, "fail_under") == 0 || strcmp(key, "fail_under_branches") == 0) {
        double *target =
            strcmp(key, "fail_under") == 0 ? &config->fail_under : &config->fail_under_branches;
        if (!read_percent(value, target, &end))
            return fail(err, err_size, source, line, "'%s' must be a number from 0 to 100", key);
    } else if (strcmp(key, "lcov") == 0 || strcmp(key, "json") == 0 ||
               strcmp(key, "tool") == 0) {
        char *target = key[0] == 'l' ? config->lcov : key[0] == 'j' ? config->json : config->tool;
        if (!read_string(value, target, COV_PATH_MAX, &end))
            return fail(err, err_size, source, line, "'%s' must be a string", key);
        if (key[0] != 't' && !cov_output_path_is_safe(target))
            return fail(err, err_size, source, line,
                        "'%s' must be a relative path inside the project, without '..'", key);
    } else {
        return fail(err, err_size, source, line, "unknown key '%s'", key);
    }
    if (!only_comment_left(end))
        return fail(err, err_size, source, line, "unexpected text after the value of '%s'", key);
    return true;
}

bool cov_config_parse(const char *text, const char *source, cov_config *config, char *err,
                      size_t err_size) {
    int number = 0;
    const char *p = text;
    while (*p != '\0') {
        number++;
        const char *eol = strchr(p, '\n');
        const size_t length = eol == NULL ? strlen(p) : (size_t)(eol - p);
        if (length >= LINE_MAX_LENGTH)
            return fail(err, err_size, source, number, "line longer than %d characters",
                        LINE_MAX_LENGTH - 1);
        char line[LINE_MAX_LENGTH];
        memcpy(line, p, length);
        line[length] = '\0';
        if (length > 0 && line[length - 1] == '\r')
            line[length - 1] = '\0';
        p = eol == NULL ? p + length : eol + 1;

        const char *at = skip_spaces(line);
        if (*at == '\0' || *at == '#')
            continue;
        if (*at == '[')
            return fail(err, err_size, source, number, "tables are not part of this file");

        char key[64];
        size_t key_length = 0;
        while (isalnum((unsigned char)*at) || *at == '_') {
            if (key_length + 1 >= sizeof key)
                return fail(err, err_size, source, number, "key too long");
            key[key_length++] = *at++;
        }
        key[key_length] = '\0';
        at = skip_spaces(at);
        if (key_length == 0 || *at != '=')
            return fail(err, err_size, source, number, "expected 'key = value'");
        if (!apply(config, key, skip_spaces(at + 1), source, number, err, err_size))
            return false;
    }
    return true;
}

bool cov_config_load(cov_config *config, char *err, size_t err_size) {
    cov_config_defaults(config);
    FILE *file = fopen(COV_CONFIG_FILE, "rb");
    if (file == NULL)
        return true; /* absent: the defaults are the configuration */

    cov_text text = {0};
    char chunk[4096];
    size_t read;
    while ((read = fread(chunk, 1, sizeof chunk - 1, file)) > 0) {
        chunk[read] = '\0';
        cov_text_add(&text, chunk);
    }
    const bool read_failed = ferror(file) != 0;
    fclose(file);
    char *content = cov_text_take(&text);
    if (read_failed || content == NULL) {
        free(content);
        snprintf(err, err_size, "%s: could not be read", COV_CONFIG_FILE);
        return false;
    }
    const bool ok = cov_config_parse(content, COV_CONFIG_FILE, config, err, err_size);
    free(content);
    return ok;
}

/* Whether `path` is `prefix` or lies under it. */
static bool under(const char *path, const char *prefix) {
    const size_t length = strlen(prefix);
    if (strcmp(prefix, ".") == 0)
        return true;
    return strncmp(path, prefix, length) == 0 && (path[length] == '\0' || path[length] == '/');
}

bool cov_config_measures(const cov_config *config, const char *path) {
    bool included = false;
    for (size_t i = 0; i < config->include_count && !included; i++)
        included = under(path, config->include[i]);
    if (!included)
        return false;
    for (size_t i = 0; i < config->exclude_count; i++) {
        if (under(path, config->exclude[i]))
            return false;
    }
    return true;
}

bool cov_output_path_is_safe(const char *path) {
    if (path[0] == '\0' || path[0] == '/' || path[0] == '\\')
        return false;
    if (isalpha((unsigned char)path[0]) && path[1] == ':') /* C:\... */
        return false;
    /* No ".." as a whole component, whichever separator delimits it. */
    const char *p = path;
    while (*p != '\0') {
        const size_t length = strcspn(p, "/\\");
        if (length == 2 && p[0] == '.' && p[1] == '.')
            return false;
        p += length;
        if (*p != '\0')
            p++;
    }
    return true;
}
