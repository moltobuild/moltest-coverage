# Progress

## 2026-10-04 — M0 repository and design
- Done: repository, manifest, docs scaffold, ADRs 0001-0003, spec 001; verified that a molto profile with `--coverage` instruments src/ and that `gcov -p` reads the result from the project root
- Commit: see `git log`
- Tests: none yet (no code)
- Next: moltest spec 004 (plugin API v1)

## 2026-10-04 — M1 done in moltest
- Done: moltest 0.3.0 implements the plugin API this plugin needs (moltest_add_reporter, api_version, on_test_start, moltest_fail_run)
- Commit: moltest 51e653d (branch feat/plugin-api-v1)
- Tests: moltest suite 21 passed, 17 skipped
- Next: M2, starting with the config reader

## 2026-10-04 — spec 001 MVP implemented
- Done: config reader, gcov text parser (GCC and llvm-cov), text/lcov/JSON reports, collection (flush, erase, find, tool fallback), reporter registration; moltest API declared locally (src/moltest_api.h) so the consumer's moltest is the only one; molto#87 so the plugin's --coverage reaches the link line
- Commit: c7a3cc3, e286e8e, 6eda42a
- Tests: `molto test` 17 passed and the plugin reports its own coverage; `.github/e2e.sh` ok with molto built from #87; fmt and lint clean
- Next: CI after a molto release with #87; Windows collection is untested until then

## 2026-10-04 — CI
- Done: CI workflow (test, e2e on Linux/macOS/Windows; style as a gate), molto pinned to 0.47.1
- Commit: see `git log`
- Tests: not run on runners yet: needs the molto 0.47.1 release
- Next: release molto 0.47.1, then CI green and moltest-coverage 0.1.0

## 2026-10-04 — preparing 0.1.0
- Done: release pipeline (#3); moltest pinned to tag v0.3.0 instead of master; README states requirements (molto 0.47.2+, moltest 0.3.0+) and the built-in coverage profile (molto 0.48.0+)
- Commit: see `git log`
- Tests: CI green on master (ba6c7e1)
- Next: tag v0.1.0; molto adopts moltest and moltest-coverage

## 2026-10-04 — 0.1.1
- Done: 0.1.0 released from master; KI-2 fixed (a floor no longer fails ordinary runs); version 0.1.1
- Commit: see `git log`
- Tests: unit suite and `.github/e2e.sh` (step 1 now with a floor) pass with molto 0.48.0
- Next: tag v0.1.1; molto pins it

## 2026-10-05 — adopted by molto
- Done: 0.1.1 released; molto (#92) measures itself with it: moltest_coverage v0.1.1 in [dev-deps], fail_under 79.5, make coverage → molto test --profile coverage (79.9% on the Linux runner)
- Commit: see `git log`
- Tests: molto's CI green on Linux, macOS and Windows with this plugin linked into its suite
- Next: CI on molto 0.48.0 and `--profile coverage` in e2e

## 2026-10-05 — M4 start: KI-3
- Done: found that a `per_file` suite fails its floor at full coverage (each
  executable erases and judges alone); KI-3 logged, e2e step 4 reproduces it,
  ADR 0004 proposed, M4 added; molto RFC-0020 (moltobuild/molto#95) merged as Draft
- Tests: e2e passes locally, step 4 asserts the failure
- Next: molto RFC-0020 accepted and implemented, then ADR 0004 here

## 2026-10-05 — spec 002: several executables, one measurement
- Done: `cov_position_parse` (src/position.c) reads molto's MOLTO_TEST_INDEX /
  MOLTO_TEST_COUNT (RFC-0020); only the first executable erases, only the last
  reports and judges; e2e step 4 flipped to expect one report at 100%; ADR 0004 Accepted
- Tests: 20 self-tests pass (3 new); e2e passes on molto master; fmt and lint clean
- Next: molto 0.50.0 released, bump the CI's MOLTO_VERSION, resolve KI-3, release 0.2.0

## 2026-10-05 — KI-3 resolved
- Done: CI on molto 0.50.0, e2e on the built-in coverage profile (closes M3's bump); KI-3 resolved
- Tests: CI green on Linux, macOS, Windows (Test, E2E, Style); e2e step 4 at 100%
- Next: release 0.2.0

## 2026-10-05 — KI-4: test binaries in a subfolder of tests/
- Done: `cov_profile_dir` finds `build/<profile>` from the right instead of cutting
  two components; a last executable under `tests/units/` measured nothing and
  skipped the floor. Found with molto's first isolated test.
- Tests: 25 self-tests (5 new in tests/test_profile_dir.c); e2e red on the old
  code, green on the fix; fmt and lint clean
- Next: release 0.2.1, then molto's [dev-deps]
