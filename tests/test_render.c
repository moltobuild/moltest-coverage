#include <moltest.h>

#include "../src/coverage.h"

#include <string.h>

/* The three outputs (spec 001 AC2, AC6, AC7). Two files: one complete, one
   with a never-run function, a missing range and a branch never executed. */

static const char two_files[] = "        -:    0:Source:src/good.c\n"
                                "function good called 3 returned 100% blocks executed 100%\n"
                                "        3:    1:int good(void) {\n"
                                "        3:    2:    return 1;\n"
                                "        3:    3:}\n"
                                "        -:    0:Source:src/weak.c\n"
                                "function used called 1 returned 100% blocks executed 50%\n"
                                "        1:    1:int used(int x) {\n"
                                "        1:    2:    if (x)\n"
                                "branch  0 taken 1\n"
                                "branch  1 taken 0\n"
                                "        1:    3:        return 1;\n"
                                "    #####:    4:    return 0;\n"
                                "        -:    5:}\n"
                                "function unused called 0 returned 0% blocks executed 0%\n"
                                "    #####:    6:int unused(void) {\n"
                                "    #####:    7:    return 2;\n"
                                "branch  0 never executed\n"
                                "    #####:    8:}\n";

static cov_report report;

BEFORE_EACH() {
    char err[128];
    ASSERT_TRUE(cov_parse_gcov(two_files, &report, err, sizeof err));
    cov_report_sort(&report);
}

AFTER_EACH() {
    cov_report_free(&report);
}

DESCRIBE(text_report_lists_worst_first) {
    char *text = cov_render_text(&report, "gcov 13.2");
    ASSERT_NOT_NULL(text);
    EXPECT_NOT_NULL(strstr(text, "moltest_coverage (gcov 13.2)"));
    const char *weak = strstr(text, "src/weak.c");
    const char *good = strstr(text, "src/good.c");
    ASSERT_NOT_NULL(weak);
    ASSERT_NOT_NULL(good);
    EXPECT_TRUE(weak < good);
    /* weak.c: lines 3/7, branches 1/3, functions 1/2; missing 4-8, since
       line 5 has no code and so does not split the run */
    EXPECT_NOT_NULL(strstr(text, "3/7  42.9%"));
    EXPECT_NOT_NULL(strstr(text, "1/3  33.3%"));
    EXPECT_NOT_NULL(strstr(text, "1/2  50.0%"));
    EXPECT_NOT_NULL(strstr(text, "  4-8\n"));
    /* good.c has no branches: a dash, not 0/0 */
    EXPECT_NOT_NULL(strstr(text, "TOTAL"));
    EXPECT_NOT_NULL(strstr(text, "6/10  60.0%"));
    free(text);
}

DESCRIBE(text_report_says_when_there_is_nothing) {
    cov_report empty = {0};
    char *text = cov_render_text(&empty, "gcov");
    EXPECT_NOT_NULL(strstr(text, "no measured file has coverage data"));
    free(text);
}

DESCRIBE(lcov_has_every_record) {
    char *lcov = cov_render_lcov(&report);
    ASSERT_NOT_NULL(lcov);
    EXPECT_EQ(2, (int)(strstr(lcov, "SF:src/good.c") != NULL) + (strstr(lcov, "SF:src/weak.c") != NULL));
    EXPECT_NOT_NULL(strstr(lcov, "FN:6,unused\n"));
    EXPECT_NOT_NULL(strstr(lcov, "FNDA:0,unused\n"));
    EXPECT_NOT_NULL(strstr(lcov, "FNF:2\nFNH:1\n"));
    EXPECT_NOT_NULL(strstr(lcov, "BRDA:2,0,0,1\n"));
    EXPECT_NOT_NULL(strstr(lcov, "BRDA:2,0,1,0\n"));
    EXPECT_NOT_NULL(strstr(lcov, "BRDA:7,0,0,-\n")); /* never executed */
    EXPECT_NOT_NULL(strstr(lcov, "BRF:3\nBRH:1\n"));
    EXPECT_NOT_NULL(strstr(lcov, "DA:4,0\n"));
    EXPECT_NOT_NULL(strstr(lcov, "LF:7\nLH:3\nend_of_record\n"));
    free(lcov);
}

DESCRIBE(json_has_totals_and_files) {
    char *json = cov_render_json(&report, "gcov \"13\"");
    ASSERT_NOT_NULL(json);
    EXPECT_NOT_NULL(strstr(json, "\"tool\": \"gcov \\\"13\\\"\""));
    EXPECT_NOT_NULL(strstr(json, "\"totals\": {\"lines\": {\"covered\": 6, \"total\": 10, "
                                 "\"percent\": 60.00}"));
    EXPECT_NOT_NULL(strstr(json, "{\"path\": \"src/weak.c\""));
    EXPECT_NOT_NULL(strstr(json, "\"missing_lines\": [4, 6, 7, 8]"));
    free(json);
}
