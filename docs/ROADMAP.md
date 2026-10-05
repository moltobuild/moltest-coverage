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
- [x] CI on Linux, macOS, Windows (as moltest's), pinned to molto 0.47.2 (the release with #87 and #90)
- [x] Release 0.1.0 (tag v0.1.0)
- [x] Release 0.1.1 (KI-2 fixed; moltest pinned to v0.3.0)

## M3 - Adoption in molto
- [x] molto RFC-0019: a built-in `coverage` profile (molto 0.48.0)
- [x] molto replaces `make coverage` and `coverage.floor` with moltest-coverage (moltobuild/molto#92): `fail_under = 79.5` in its moltest-coverage.toml, the coverage job runs `make coverage` → `molto test --profile coverage`
- [ ] Bump the CI's molto to 0.48.0 and run e2e with `--profile coverage` instead of a custom profile

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
