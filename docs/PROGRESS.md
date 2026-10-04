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
