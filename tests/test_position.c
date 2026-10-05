#include <moltest.h>

#include "../src/coverage.h"

/* A test executable's place in the run, from molto RFC-0020 (ADR 0004,
   spec 002 AC1-AC3). */

DESCRIBE(absent_variables_make_a_run_of_its_own) {
    const cov_position p = cov_position_parse(NULL, NULL);
    EXPECT_TRUE(p.valid);
    EXPECT_TRUE(p.first);
    EXPECT_TRUE(p.last);
}

DESCRIBE(the_first_and_the_last_are_told_apart) {
    cov_position p = cov_position_parse("1", "3");
    EXPECT_TRUE(p.valid);
    EXPECT_TRUE(p.first);
    EXPECT_FALSE(p.last);

    p = cov_position_parse("2", "3");
    EXPECT_FALSE(p.first);
    EXPECT_FALSE(p.last);
    EXPECT_EQ(2, (int)p.index);
    EXPECT_EQ(3, (int)p.count);

    p = cov_position_parse("3", "3");
    EXPECT_FALSE(p.first);
    EXPECT_TRUE(p.last);

    p = cov_position_parse("1", "1");
    EXPECT_TRUE(p.first);
    EXPECT_TRUE(p.last);
}

DESCRIBE(a_position_that_does_not_parse_is_a_run_of_its_own) {
    static const char *const bad[][2] = {
        {"1", NULL}, {NULL, "2"}, {"", "2"},  {"x", "2"}, {"1", "2x"},
        {"-1", "2"}, {"0", "2"},  {"3", "2"}, {"1", "0"}, {" 1", "2"},
    };
    for(size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        const cov_position p = cov_position_parse(bad[i][0], bad[i][1]);
        EXPECT_FALSE(p.valid);
        EXPECT_TRUE(p.first);
        EXPECT_TRUE(p.last);
    }
}
