#include <moltest.h>

#include "../src/coverage.h"

#include <string.h>

/* The annotated gcov text, from both tools (ADR 0002, spec 001 AC4). */

/* What `llvm-cov gcov -b -c -t` printed for src/cov.c (Apple LLVM 21). */
static const char llvm_text[] = "        -:    0:Source:src/cov.c\n"
                                "        -:    0:Graph:build/custom/obj/src/cov.c.gcno\n"
                                "        -:    0:Data:build/custom/obj/src/cov.c.gcda\n"
                                "        -:    0:Runs:1\n"
                                "        -:    1:#include <cov.h>\n"
                                "function cov_add called 2 returned 100% blocks executed 75%\n"
                                "        2:    2:int cov_add(int a, int b) {\n"
                                "        2:    3:    if (a < 0)\n"
                                "branch  0 taken 2\n"
                                "branch  1 taken 0\n"
                                "    #####:    4:        return -1;\n"
                                "        2:    5:    return a + b;\n"
                                "        2:    6:}\n";

/* The same program as GCC's gcov prints it: `N*` on a line with an unexecuted
   block, `(fallthrough)`, `call` records, and a path with "./". */
static const char gcc_text[] = "        -:    0:Source:./src/cov.c\n"
                               "        -:    0:Graph:build/custom/obj/src/cov.c.gcno\n"
                               "        -:    0:Data:build/custom/obj/src/cov.c.gcda\n"
                               "        -:    0:Runs:1\n"
                               "        -:    1:#include <cov.h>\n"
                               "function cov_add called 2 returned 100% blocks executed 75%\n"
                               "        2:    2:int cov_add(int a, int b) {\n"
                               "       2*:    3:    if (a < 0)\n"
                               "branch  0 taken 2 (fallthrough)\n"
                               "branch  1 taken 0\n"
                               "    #####:    4:        return -1;\n"
                               "call    0 never executed\n"
                               "        2:    5:    return a + b;\n"
                               "        2:    6:}\n";

static cov_report report;
static char err[256];

AFTER_EACH() {
    cov_report_free(&report);
}

static void expect_cov_add_records(void) {
    ASSERT_EQ(1, (int)report.count);
    const cov_file *file = &report.files[0];
    EXPECT_STREQ("src/cov.c", file->path);

    const cov_totals t = cov_file_totals(file);
    EXPECT_EQ(5, (int)t.lines_total); /* lines 2-6; line 1 has no code */
    EXPECT_EQ(4, (int)t.lines_hit);
    EXPECT_EQ(2, (int)t.branches_total);
    EXPECT_EQ(1, (int)t.branches_hit);
    EXPECT_EQ(1, (int)t.functions_total);
    EXPECT_EQ(1, (int)t.functions_hit);

    ASSERT_EQ(1, (int)file->function_count);
    EXPECT_STREQ("cov_add", file->functions[0].name);
    EXPECT_EQ(2, file->functions[0].line);
    EXPECT_EQ(2, (int)file->functions[0].count);

    ASSERT_EQ(2, (int)file->branch_count);
    EXPECT_EQ(3, file->branches[0].line);
    EXPECT_EQ(0, file->branches[0].index);
    EXPECT_EQ(1, file->branches[1].index);

    char *missing = cov_missing_ranges(file);
    EXPECT_STREQ("4", missing);
    free(missing);
}

DESCRIBE(parses_llvm_gcov) {
    ASSERT_TRUE(cov_parse_gcov(llvm_text, &report, err, sizeof err));
    expect_cov_add_records();
}

DESCRIBE(parses_gcc_gcov) {
    ASSERT_TRUE(cov_parse_gcov(gcc_text, &report, err, sizeof err));
    expect_cov_add_records();
}

DESCRIBE(a_file_printed_twice_is_merged) {
    /* A header with inline code shows up once per object that includes it. */
    ASSERT_TRUE(cov_parse_gcov(llvm_text, &report, err, sizeof err));
    ASSERT_TRUE(cov_parse_gcov(llvm_text, &report, err, sizeof err));
    ASSERT_EQ(1, (int)report.count);
    const cov_file *file = &report.files[0];
    EXPECT_EQ(5, (int)file->line_count);
    EXPECT_EQ(4, (int)file->functions[0].count);
    EXPECT_EQ(4, (int)file->branches[0].taken);
}

DESCRIBE(several_files_in_one_output) {
    char both[2048];
    snprintf(both, sizeof both, "%s        -:    0:Source:src/other.c\n"
                                "    #####:    3:int unused;\n", llvm_text);
    ASSERT_TRUE(cov_parse_gcov(both, &report, err, sizeof err));
    ASSERT_EQ(2, (int)report.count);
    cov_report_sort(&report);
    EXPECT_STREQ("src/other.c", report.files[0].path); /* 0% sorts first */
}

DESCRIBE(exceptional_and_never_executed_count_as_missing) {
    ASSERT_TRUE(cov_parse_gcov("        -:    0:Source:src/x.c\n"
                               "    =====:   10:    throw_it();\n"
                               "    #####:   11:    a();\n"
                               "    #####:   12:    b();\n"
                               "        1:   13:    c();\n"
                               "    #####:   15:    d();\n"
                               "branch  0 never executed\n",
                               &report, err, sizeof err));
    char *missing = cov_missing_ranges(&report.files[0]);
    EXPECT_STREQ("10-12, 15", missing);
    free(missing);
    const cov_totals t = cov_file_totals(&report.files[0]);
    EXPECT_EQ(1, (int)t.branches_total);
    EXPECT_EQ(0, (int)t.branches_hit);
}

DESCRIBE(malformed_output_is_an_error) {
    EXPECT_FALSE(cov_parse_gcov("branch  0 taken 1\n", &report, err, sizeof err));
    EXPECT_NOT_NULL(strstr(err, "line 1: branch before any source file"));
    cov_report_free(&report);
    EXPECT_FALSE(cov_parse_gcov("        1:    3:x\n", &report, err, sizeof err));
}

DESCRIBE(absolute_paths_become_relative_to_the_project) {
    ASSERT_TRUE(cov_parse_gcov("        -:    0:Source:/w/app/src/a.c\n"
                               "        1:    1:int a;\n"
                               "        -:    0:Source:C:\\w\\app\\src\\b.c\n"
                               "        1:    1:int b;\n"
                               "        -:    0:Source:/w/application/src/c.c\n"
                               "        1:    1:int c;\n"
                               "        -:    0:Source:/usr/include/stdio.h\n"
                               "        1:    1:int d;\n",
                               &report, err, sizeof err));
    cov_report_relativize(&report, "/w/app/");
    EXPECT_STREQ("src/a.c", report.files[0].path);
    EXPECT_STREQ("/w/application/src/c.c", report.files[2].path); /* a prefix, not the root */
    EXPECT_STREQ("/usr/include/stdio.h", report.files[3].path);
    cov_report_relativize(&report, "c:\\w\\app");
    EXPECT_STREQ("src/b.c", report.files[1].path); /* drive letter, any case */
}
