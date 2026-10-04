#include "coverage.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define getcwd _getcwd
#else
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/*
 * From counters in memory to a parsed report: flush, find, run the tool.
 *
 * The plugin itself is compiled and linked with --coverage (recipe.toml), so
 * the coverage runtime is in every test binary that links it and __gcov_dump
 * can be called directly. A weak reference would not do: on ELF it does not
 * pull __gcov_dump out of libgcov.a, and on Mach-O an undefined weak symbol
 * fails the link.
 */

void __gcov_dump(void); // NOLINT(bugprone-reserved-identifier): the runtime's own name

#define BATCH 32 /* .gcda files per run of the tool */
#define PATH_LIMIT 1024

/* ------------------------------------------------------------------ */
/* Running a tool                                                       */
/* ------------------------------------------------------------------ */

#ifdef _WIN32
/* One argument quoted the way the C runtime splits a command line back. */
static void quote_argument(cov_text *line, const char *arg) {
    cov_text_add(line, "\"");
    size_t backslashes = 0;
    for(const char *c = arg; *c != '\0'; c++) {
        if(*c == '\\') {
            backslashes++;
            continue;
        }
        for(size_t i = 0; i < backslashes * (*c == '"' ? 2 : 1); i++)
            cov_text_add(line, "\\");
        backslashes = 0;
        if(*c == '"')
            cov_text_add(line, "\\\"");
        else
            cov_text_addf(line, "%c", *c);
    }
    for(size_t i = 0; i < backslashes * 2; i++)
        cov_text_add(line, "\\");
    cov_text_add(line, "\" ");
}

char *cov_run_capture(const char *const argv[], int *status) {
    *status = -1;
    cov_text line = {0};
    for(size_t i = 0; argv[i] != NULL; i++)
        quote_argument(&line, argv[i]);
    char *command = cov_text_take(&line);
    if(command == NULL)
        return NULL;

    SECURITY_ATTRIBUTES inherit = {sizeof inherit, NULL, TRUE};
    HANDLE read_end = NULL, write_end = NULL;
    if(!CreatePipe(&read_end, &write_end, &inherit, 0)) {
        free(command);
        return NULL;
    }
    SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0);
    HANDLE null_device =
        CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &inherit, OPEN_EXISTING, 0, NULL);

    STARTUPINFOA start = {.cb = sizeof start, .dwFlags = STARTF_USESTDHANDLES};
    start.hStdOutput = write_end;
    start.hStdError = null_device;
    start.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process = {0};
    const BOOL started =
        CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &start, &process);
    free(command);
    CloseHandle(write_end);
    if(null_device != INVALID_HANDLE_VALUE)
        CloseHandle(null_device);
    if(!started) {
        CloseHandle(read_end);
        return NULL;
    }

    cov_text out = {0};
    cov_text_add(&out, "");
    char chunk[4096];
    DWORD read = 0;
    while(ReadFile(read_end, chunk, sizeof chunk - 1, &read, NULL) && read > 0) {
        chunk[read] = '\0';
        cov_text_add(&out, chunk);
    }
    CloseHandle(read_end);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);
    *status = (int)code;
    return cov_text_take(&out);
}
#else
char *cov_run_capture(const char *const argv[], int *status) {
    *status = -1;
    int pipe_ends[2];
    if(pipe(pipe_ends) != 0)
        return NULL;
    fflush(stdout);
    fflush(stderr);
    const pid_t child = fork();
    if(child < 0) {
        close(pipe_ends[0]);
        close(pipe_ends[1]);
        return NULL;
    }
    if(child == 0) {
        dup2(pipe_ends[1], STDOUT_FILENO);
        const int null_device = open("/dev/null", O_WRONLY);
        if(null_device >= 0)
            dup2(null_device, STDERR_FILENO);
        close(pipe_ends[0]);
        close(pipe_ends[1]);
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    close(pipe_ends[1]);
    cov_text out = {0};
    cov_text_add(&out, "");
    char chunk[4096];
    ssize_t read_count;
    while((read_count = read(pipe_ends[0], chunk, sizeof chunk - 1)) > 0) {
        chunk[read_count] = '\0';
        cov_text_add(&out, chunk);
    }
    close(pipe_ends[0]);
    int raw = 0;
    while(waitpid(child, &raw, 0) < 0) {
    }
    *status = WIFEXITED(raw) ? WEXITSTATUS(raw) : -1;
    return cov_text_take(&out);
}
#endif

/* ------------------------------------------------------------------ */
/* Finding the data                                                     */
/* ------------------------------------------------------------------ */

static bool is_separator(char c) { return c == '/' || c == '\\'; }

bool cov_profile_dir(const char *self, char *out, size_t size) {
    /* <...>/build/<profile>/tests/<binary> → <...>/build/<profile> */
    if(self == NULL || strlen(self) >= size)
        return false;
    snprintf(out, size, "%s", self);
    for(int cut = 0; cut < 2; cut++) {
        char *last = NULL;
        for(char *c = out; *c != '\0'; c++) {
            if(is_separator(*c))
                last = c;
        }
        if(last == NULL)
            return false;
        *last = '\0';
    }
    return out[0] != '\0';
}

static bool ends_with(const char *text, const char *suffix) {
    const size_t length = strlen(text), suffix_length = strlen(suffix);
    return length >= suffix_length && strcmp(text + length - suffix_length, suffix) == 0;
}

typedef struct {
    char **paths;
    size_t count, capacity;
} path_list;

static void path_list_free(path_list *list) {
    for(size_t i = 0; i < list->count; i++)
        free(list->paths[i]);
    free((void *)list->paths);
    *list = (path_list){0};
}

static bool path_list_add(path_list *list, const char *path) {
    if(list->count == list->capacity) {
        const size_t next = list->capacity == 0 ? 32 : list->capacity * 2;
        char **grown = (char **)realloc((void *)list->paths, next * sizeof *grown);
        if(grown == NULL)
            return false;
        list->paths = grown;
        list->capacity = next;
    }
    const size_t size = strlen(path) + 1;
    list->paths[list->count] = malloc(size);
    if(list->paths[list->count] == NULL)
        return false;
    memcpy(list->paths[list->count++], path, size);
    return true;
}

/* Every file under `dir` whose name ends in `suffix`, recursively. */
static void find_suffixed(const char *dir, const char *suffix, path_list *found) {
    DIR *handle = opendir(dir);
    if(handle == NULL)
        return;
    struct dirent *entry;
    while((entry = readdir(handle)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char path[PATH_LIMIT];
        const int written = snprintf(path, sizeof path, "%s/%s", dir, entry->d_name);
        if(written < 0 || (size_t)written >= sizeof path)
            continue;
        struct stat info;
        if(stat(path, &info) != 0)
            continue;
        if(S_ISDIR(info.st_mode))
            find_suffixed(path, suffix, found);
        else if(ends_with(path, suffix))
            (void)path_list_add(found, path);
    }
    closedir(handle);
}

void cov_erase(const char *profile_dir) {
    char obj[PATH_LIMIT];
    snprintf(obj, sizeof obj, "%s/obj", profile_dir);
    path_list found = {0};
    find_suffixed(obj, ".gcda", &found);
    for(size_t i = 0; i < found.count; i++)
        (void)remove(found.paths[i]);
    path_list_free(&found);
}

/* ------------------------------------------------------------------ */
/* Choosing and running the tool                                        */
/* ------------------------------------------------------------------ */

#define TOOL_WORDS 4

typedef struct {
    char words[TOOL_WORDS][COV_PATH_MAX];
    size_t count;
} tool_command;

/* "llvm-cov gcov" → {"llvm-cov", "gcov"}. */
static void tool_split(const char *spec, tool_command *tool) {
    tool->count = 0;
    const char *p = spec;
    while(*p != '\0' && tool->count < TOOL_WORDS) {
        while(*p == ' ')
            p++;
        const size_t length = strcspn(p, " ");
        if(length == 0)
            break;
        snprintf(tool->words[tool->count++], COV_PATH_MAX, "%.*s", (int)length, p);
        p += length;
    }
}

/* The candidates, in order, for the compiler that built this file: its own
   gcov first, then the unversioned name. */
static size_t tool_candidates(const cov_config *config, char out[][COV_PATH_MAX]) {
    if(config->tool[0] != '\0') {
        snprintf(out[0], COV_PATH_MAX, "%s", config->tool);
        return 1;
    }
#if defined(__clang__)
    snprintf(out[0], COV_PATH_MAX, "llvm-cov-%d gcov", __clang_major__);
    snprintf(out[1], COV_PATH_MAX, "llvm-cov gcov");
    snprintf(out[2], COV_PATH_MAX, "gcov");
    return 3;
#elif defined(__GNUC__)
    snprintf(out[0], COV_PATH_MAX, "gcov-%d", __GNUC__);
    snprintf(out[1], COV_PATH_MAX, "gcov");
    return 2;
#else
    snprintf(out[0], COV_PATH_MAX, "gcov");
    return 1;
#endif
}

/* Run `tool` over `files[from, to)`; NULL output when it could not run. */
static char *run_tool(const tool_command *tool, char **files, size_t from, size_t to, int *status) {
    const char *argv[TOOL_WORDS + 3 + BATCH + 1];
    size_t n = 0;
    for(size_t i = 0; i < tool->count; i++)
        argv[n++] = tool->words[i];
    argv[n++] = "-b";
    argv[n++] = "-c";
    argv[n++] = "-t";
    for(size_t i = from; i < to; i++)
        argv[n++] = files[i];
    argv[n] = NULL;
    return cov_run_capture(argv, status);
}

/* `path`, relative to `root` when it lies under it. */
static const char *relative_to(const char *path, const char *root) {
    const size_t length = strlen(root);
    if(length > 0 && strncmp(path, root, length) == 0 && is_separator(path[length]))
        return path + length + 1;
    return path;
}

/* The project root, which is where molto runs the tests from, with symlinks
   resolved: a build that names sources by absolute path names them through
   the resolved one (/private/var, not /var, on macOS). */
static bool project_root(char *out, size_t size) {
    char here[PATH_LIMIT];
    if(getcwd(here, sizeof here) == NULL)
        return false;
#ifdef _WIN32
    return _fullpath(out, here, size) != NULL;
#else
    char resolved[PATH_MAX];
    if(realpath(here, resolved) == NULL || strlen(resolved) >= size)
        return false;
    snprintf(out, size, "%s", resolved);
    return true;
#endif
}

/* The files under `obj` ending in `suffix` whose sources `config` measures:
   an object's path under obj/ mirrors its source's, so "obj/src/x.c.gcda" is
   src/x.c. */
static void find_measured(const cov_config *config, const char *obj, const char *suffix,
                          path_list *measured) {
    path_list found = {0};
    find_suffixed(obj, suffix, &found);
    for(size_t i = 0; i < found.count; i++) {
        char source[PATH_LIMIT];
        snprintf(source, sizeof source, "%s", relative_to(found.paths[i], obj));
        source[strlen(source) - strlen(suffix)] = '\0';
        for(char *c = source; *c != '\0'; c++) {
            if(*c == '\\')
                *c = '/';
        }
        if(cov_config_measures(config, source))
            (void)path_list_add(measured, found.paths[i]);
    }
    path_list_free(&found);
}

bool cov_collect(const cov_config *config, const char *profile_dir, cov_report *report,
                 bool *instrumented, char *tool_used, size_t tool_size, char *err,
                 size_t err_size) {
    __gcov_dump();

    char obj[PATH_LIMIT];
    snprintf(obj, sizeof obj, "%s/obj", profile_dir);

    /* A compile with --coverage writes a .gcno beside the object; a build
       without one has nothing to measure, which is not a measurement of zero. */
    path_list notes = {0};
    find_measured(config, obj, ".gcno", &notes);
    *instrumented = notes.count > 0;
    path_list_free(&notes);

    path_list measured = {0};
    find_measured(config, obj, ".gcda", &measured);
    if(measured.count == 0) {
        snprintf(tool_used, tool_size, "no data");
        path_list_free(&measured);
        return true;
    }

    char candidates[3][COV_PATH_MAX];
    const size_t candidate_count = tool_candidates(config, candidates);
    bool ok = false;
    for(size_t c = 0; c < candidate_count && !ok; c++) {
        tool_command tool;
        tool_split(candidates[c], &tool);
        cov_report attempt = {0};
        ok = true;
        for(size_t from = 0; from < measured.count && ok; from += BATCH) {
            const size_t to = from + BATCH < measured.count ? from + BATCH : measured.count;
            int status = 0;
            char *output = run_tool(&tool, measured.paths, from, to, &status);
            ok = output != NULL && status == 0 && cov_parse_gcov(output, &attempt, err, err_size);
            free(output);
        }
        if(ok) {
            *report = attempt;
            snprintf(tool_used, tool_size, "%s", candidates[c]);
        } else {
            cov_report_free(&attempt);
        }
    }
    path_list_free(&measured);
    if(!ok) {
        snprintf(err, err_size,
                 "no gcov tool could read the data (tried %s%s%s%s%s); set 'tool' in %s",
                 candidates[0], candidate_count > 1 ? ", " : "",
                 candidate_count > 1 ? candidates[1] : "", candidate_count > 2 ? ", " : "",
                 candidate_count > 2 ? candidates[2] : "", COV_CONFIG_FILE);
        return false;
    }
    char root[PATH_LIMIT];
    if(project_root(root, sizeof root))
        cov_report_relativize(report, root);
    cov_report_filter(report, config);
    cov_report_sort(report);
    return true;
}
