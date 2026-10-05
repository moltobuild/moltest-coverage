#include <moltest.h>

#include "../src/coverage.h"

/* Where a test binary's build lives, from the binary's own path (KI-4). */

static char dir[512];

static bool profile_of(const char *self) {
    dir[0] = '\0';
    return cov_profile_dir(self, dir, sizeof dir);
}

DESCRIBE(a_binary_directly_under_tests_is_in_its_profile) {
    ASSERT_TRUE(profile_of("/p/build/coverage/tests/app_tests"));
    EXPECT_STREQ("/p/build/coverage", dir);
}

DESCRIBE(a_binary_in_a_subfolder_of_tests_is_in_the_same_profile) {
    /* per_file mirrors tests/services/test_x.c to tests/services/test_x, and
       molto's isolated tests do the same: two cuts landed in tests/. */
    ASSERT_TRUE(profile_of("/p/build/coverage/tests/services/test_x"));
    EXPECT_STREQ("/p/build/coverage", dir);
    ASSERT_TRUE(profile_of("/p/build/coverage/tests/a/b/c/test_x"));
    EXPECT_STREQ("/p/build/coverage", dir);
}

DESCRIBE(folders_named_build_or_tests_do_not_confuse_it) {
    /* molto has tests/build/; a project can live under a folder called build. */
    ASSERT_TRUE(profile_of("/home/build/p/build/coverage/tests/build/test_x"));
    EXPECT_STREQ("/home/build/p/build/coverage", dir);
    ASSERT_TRUE(profile_of("/p/build/coverage/tests/tests/test_x"));
    EXPECT_STREQ("/p/build/coverage", dir);
}

DESCRIBE(windows_separators_are_separators) {
    ASSERT_TRUE(profile_of("C:\\p\\build\\coverage\\tests\\services\\test_x.exe"));
    EXPECT_STREQ("C:\\p\\build\\coverage", dir);
}

DESCRIBE(a_path_outside_a_build_has_no_profile) {
    EXPECT_FALSE(profile_of("/usr/local/bin/app_tests"));
    EXPECT_FALSE(profile_of("/p/build/tests/x"));
    EXPECT_FALSE(profile_of(NULL));
}
