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
