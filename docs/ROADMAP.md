# Roadmap

## M0 - Repository and design
- [x] Repository, manifest, docs scaffold
- [x] ADR 0001 in-process plugin, ADR 0002 gcov text as the data source, ADR 0003 config file
- [x] Spec 001 (MVP report)

## M1 - moltest plugin API v1 (work in moltest, its spec 004)
- [x] Several reporters at once, registered from a constructor
- [x] A reporter can fail the run (needed by `fail_under`)
- [x] Versioned reporter struct (moltest 0.3.0, commit 51e653d on feat/plugin-api-v1)

## M2 - MVP (spec 001)
- [x] `moltest-coverage.toml` reader
- [x] Flush counters, find `.gcda` for `src/`, run gcov / `llvm-cov gcov`, parse `.gcov`
- [x] Lines, branches, functions per file
- [x] Terminal report, worst file first, missing line ranges
- [x] `fail_under` (lines, branches) fails the run
- [x] `coverage.lcov` and `coverage.json`
- [x] molto fix: a dependency's flags reach the link line (moltobuild/molto#87)
- [x] CI on Linux, macOS, Windows (as moltest's), pinned to molto 0.47.1 (the release with #87)
- [ ] Release 0.1.0

## M3 - Adoption in molto
- [ ] molto RFC: a built-in `coverage` profile (today: `[profile.custom] flags = ["--coverage"]`)
- [ ] molto replaces `make coverage` and `coverage.floor` with moltest-coverage

## Non-goals
- Instrumenting code itself: the compiler does that, the build system asks for it.
- Bundling gcov or llvm-cov.
- Covering tests/ or dependencies: what is measured is the code that ships.

## Backlog
- HTML report; Cobertura XML
- Exclusion markers (`LCOV_EXCL_LINE`, `LCOV_EXCL_START`/`STOP`)
- Per-test contexts: which test executed each line (`__gcov_reset`/`__gcov_dump` around each test)
- Combine runs (several platforms or profiles) into one report
- Diff coverage against a git ref
- Clang source-based coverage (`-fprofile-instr-generate`, `llvm-cov export`)
