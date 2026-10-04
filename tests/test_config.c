#include <moltest.h>

#include "../src/coverage.h"

#include <string.h>

/* moltest-coverage.toml (ADR 0003, spec 001 AC3, AC9). */

static cov_config config;
static char err[256];

static bool parse(const char *text) {
    cov_config_defaults(&config);
    err[0] = '\0';
    return cov_config_parse(text, "moltest-coverage.toml", &config, err, sizeof err);
}

DESCRIBE(defaults_measure_src_with_no_floor_and_no_files) {
    ASSERT_TRUE(parse(""));
    EXPECT_EQ(1, (int)config.include_count);
    EXPECT_STREQ("src", config.include[0]);
    EXPECT_TRUE(config.fail_under < 0);
    EXPECT_TRUE(config.fail_under_branches < 0);
    EXPECT_STREQ("", config.lcov);
    EXPECT_STREQ("", config.json);
    EXPECT_STREQ("", config.tool);
}

DESCRIBE(every_key_is_read) {
    ASSERT_TRUE(parse("# a comment\n"
                      "include = [\"src\", \"lib/\"]   # trailing comment\n"
                      "exclude = [\"src/generated\",]\n"
                      "\n"
                      "fail_under = 80.5\n"
                      "fail_under_branches = 60\n"
                      "lcov = \"build/coverage.lcov\"\n"
                      "json = \"build/coverage.json\"\r\n"
                      "tool = \"gcov-13\"\n"));
    EXPECT_EQ(2, (int)config.include_count);
    EXPECT_STREQ("lib", config.include[1]); /* trailing slash dropped */
    EXPECT_EQ(1, (int)config.exclude_count);
    EXPECT_STREQ("src/generated", config.exclude[0]);
    EXPECT_TRUE(config.fail_under > 80.4 && config.fail_under < 80.6);
    EXPECT_TRUE(config.fail_under_branches > 59.9 && config.fail_under_branches < 60.1);
    EXPECT_STREQ("build/coverage.lcov", config.lcov);
    EXPECT_STREQ("build/coverage.json", config.json);
    EXPECT_STREQ("gcov-13", config.tool);
}

DESCRIBE(config_errors_name_the_line) {
    EXPECT_FALSE(parse("fail_under = 80\nfail_undr = 90\n"));
    EXPECT_STREQ("moltest-coverage.toml:2: unknown key 'fail_undr'", err);

    EXPECT_FALSE(parse("fail_under = 120\n"));
    EXPECT_NOT_NULL(strstr(err, ":1: 'fail_under' must be a number from 0 to 100"));

    EXPECT_FALSE(parse("lcov = build/coverage.lcov\n"));
    EXPECT_NOT_NULL(strstr(err, "'lcov' must be a string"));

    EXPECT_FALSE(parse("include = \"src\"\n"));
    EXPECT_NOT_NULL(strstr(err, "'include' must be an array"));

    EXPECT_FALSE(parse("[coverage]\n"));
    EXPECT_NOT_NULL(strstr(err, "tables are not part of this file"));

    EXPECT_FALSE(parse("json = \"a.json\" extra\n"));
    EXPECT_NOT_NULL(strstr(err, "unexpected text after the value of 'json'"));
}

DESCRIBE(outputs_must_stay_inside_the_project) {
    EXPECT_FALSE(parse("lcov = \"/tmp/coverage.lcov\"\n"));
    EXPECT_NOT_NULL(strstr(err, "relative path inside the project"));
    EXPECT_FALSE(parse("json = \"build/../../x.json\"\n"));
    EXPECT_FALSE(parse("json = \"C:\\\\x.json\"\n"));
    EXPECT_TRUE(parse("json = \"build/..coverage.json\"\n")); /* a name, not a component */
}

DESCRIBE(only_included_files_are_measured) {
    ASSERT_TRUE(parse("include = [\"src\", \"lib\"]\nexclude = [\"src/generated\"]\n"));
    EXPECT_TRUE(cov_config_measures(&config, "src/parser.c"));
    EXPECT_TRUE(cov_config_measures(&config, "lib/x/y.c"));
    EXPECT_FALSE(cov_config_measures(&config, "src/generated/table.c"));
    EXPECT_FALSE(cov_config_measures(&config, "tests/test_parser.c"));
    EXPECT_FALSE(cov_config_measures(&config, "srcs/other.c")); /* a prefix, not a directory */
    EXPECT_FALSE(cov_config_measures(&config, "/usr/include/stdio.h"));
}
