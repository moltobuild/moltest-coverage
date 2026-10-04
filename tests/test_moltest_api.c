#include <moltest.h>

#include "../src/moltest_api.h"

#include <stddef.h>

/* src/moltest_api.h is a copy of moltest's reporter API; this is what keeps it
   honest. Every field of both structs, at the same offset, of the same size. */

#define SAME_FIELD(copy, real, field)                                          \
    static_assert(offsetof(copy, field) == offsetof(real, field), #field);     \
    static_assert(sizeof(((copy *)0)->field) == sizeof(((real *)0)->field), #field)

SAME_FIELD(cov_moltest_summary, moltest_summary, files);
SAME_FIELD(cov_moltest_summary, moltest_summary, tests);
SAME_FIELD(cov_moltest_summary, moltest_summary, passed);
SAME_FIELD(cov_moltest_summary, moltest_summary, failed);
SAME_FIELD(cov_moltest_summary, moltest_summary, skipped);
SAME_FIELD(cov_moltest_summary, moltest_summary, warned);
SAME_FIELD(cov_moltest_summary, moltest_summary, assertions);
SAME_FIELD(cov_moltest_summary, moltest_summary, warnings);
SAME_FIELD(cov_moltest_summary, moltest_summary, seconds);
static_assert(sizeof(cov_moltest_summary) == sizeof(moltest_summary), "summary size");

SAME_FIELD(cov_moltest_reporter, moltest_reporter, api_version);
SAME_FIELD(cov_moltest_reporter, moltest_reporter, name);
SAME_FIELD(cov_moltest_reporter, moltest_reporter, on_run_start);
SAME_FIELD(cov_moltest_reporter, moltest_reporter, on_file_start);
SAME_FIELD(cov_moltest_reporter, moltest_reporter, on_test_start);
SAME_FIELD(cov_moltest_reporter, moltest_reporter, on_test_end);
SAME_FIELD(cov_moltest_reporter, moltest_reporter, on_file_end);
SAME_FIELD(cov_moltest_reporter, moltest_reporter, on_run_end);
SAME_FIELD(cov_moltest_reporter, moltest_reporter, ctx);
static_assert(sizeof(cov_moltest_reporter) == sizeof(moltest_reporter), "reporter size");
static_assert(sizeof(moltest_status) == sizeof(int), "status passed as int");

DESCRIBE(the_api_copy_matches_moltest) {
    EXPECT_EQ(MOLTEST_REPORTER_API, COV_MOLTEST_REPORTER_API);
}
