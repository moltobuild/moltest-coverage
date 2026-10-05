#!/bin/sh
#
# moltest-coverage as a user meets it: a library from `molto new`, this
# checkout as a development dependency, then three runs (spec 001):
#
#   1. a normal `molto test`, with a floor set: links, passes, says
#      it is not a coverage build, applies no floor                     (AC8)
#   2. a coverage profile: prints the table for src/ only              (AC1, AC3)
#   3. a floor it misses, with lcov and JSON: exit 1, files written    (AC5-AC7)
#   4. a per_file suite, two executables that cover 100% together:
#      one report, at 100%, after the last                (spec 002 AC4, AC5)
#
#   e2e.sh <moltest-coverage checkout>
set -eu

checkout=$1
fail() { echo "e2e: $*" >&2; exit 1; }

cd "$(mktemp -d)"
molto new e2e_lib
cd e2e_lib
molto add moltest_coverage --dev --path "$checkout"
# A function no test calls, so coverage is under 100%.
printf '\nint e2e_lib_unused(void) {\n    return 0;\n}\n' >> src/e2e_lib.c

echo "--- 1. normal profile, a floor set"
printf 'fail_under = 99\n' > moltest-coverage.toml
molto test > run1.txt 2>&1 || { cat run1.txt; fail "a normal molto test failed under a floor"; }
grep -q "moltest_coverage: src not built for coverage" run1.txt || { cat run1.txt; fail "no 'not built for coverage' line"; }
rm moltest-coverage.toml

echo "--- 2. coverage profile"
cat >> Project.toml <<'TOML'

[profile.custom]
opt_level = 0
debug_info = true
flags = ["--coverage"]
TOML
molto test --profile custom > run2.txt 2>&1 || { cat run2.txt; fail "the coverage run failed"; }
cat run2.txt
grep -q "^src/e2e_lib.c " run2.txt || fail "src/e2e_lib.c is not in the table"
grep -q "^TOTAL " run2.txt || fail "no TOTAL row"
if grep -q "^tests/" run2.txt; then fail "tests/ was measured"; fi

echo "--- 3. floor and outputs"
printf 'fail_under = 99\nlcov = "build/coverage.lcov"\njson = "build/coverage.json"\n' > moltest-coverage.toml
if molto test --profile custom > run3.txt 2>&1; then cat run3.txt; fail "a missed floor did not fail the run"; fi
grep -q "run failed: moltest_coverage: line coverage .* is under fail_under = 99.0" run3.txt \
    || { cat run3.txt; fail "the floor's reason is missing"; }
grep -q "^SF:src/e2e_lib.c$" build/coverage.lcov || fail "lcov has no record for src/e2e_lib.c"
grep -q '"totals"' build/coverage.json || fail "JSON has no totals"

echo "--- 4. per_file: two executables"
# Each test file covers one of two functions, so the suite covers every line.
# molto tells each executable its place (RFC-0020): the first erases, the last
# reports. Before KI-3 was fixed each one erased and judged alone, and failed.
cd ..
molto new e2e_two
cd e2e_two
molto add moltest_coverage --dev --path "$checkout"
sed 's/^mode = "single".*/mode = "per_file"/' Project.toml > Project.toml.new && mv Project.toml.new Project.toml
grep -q '^mode = "per_file"' Project.toml || fail "could not switch the suite to per_file"
cat >> Project.toml <<'TOML'

[profile.custom]
opt_level = 0
debug_info = true
flags = ["--coverage"]
TOML
cat > include/e2e_two.h <<'C'
int e2e_two_a(int x);
int e2e_two_b(int x);
C
cat > src/e2e_two.c <<'C'
#include <e2e_two.h>
int e2e_two_a(int x) {
    return x + 1;
}
int e2e_two_b(int x) {
    return x - 1;
}
C
rm -f tests/*.c
printf '#include <moltest.h>\n#include <e2e_two.h>\nDESCRIBE(a) { EXPECT_EQ(2, e2e_two_a(1)); }\n' > tests/test_a.c
printf '#include <moltest.h>\n#include <e2e_two.h>\nDESCRIBE(b) { EXPECT_EQ(1, e2e_two_b(2)); }\n' > tests/test_b.c
printf 'fail_under = 90\n' > moltest-coverage.toml
molto test --profile custom > run4.txt 2>&1 || { cat run4.txt; fail "a per_file suite at 100% failed (KI-3)"; }
cat run4.txt
[ "$(grep -c '^TOTAL ' run4.txt)" -eq 1 ] || fail "expected one report, after the last executable"
grep -qE '^TOTAL +[0-9]+/[0-9]+ +100.0%' run4.txt || fail "the report is not the whole suite's 100%"
grep -q 'executable 1 of 2; the report comes with the last' run4.txt ||
    fail "the first executable did not leave the report to the last"

echo "e2e: ok"
