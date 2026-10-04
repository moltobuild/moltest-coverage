# 001 MVP coverage report — Status: In progress
## Goal
At the end of a `molto test` built with `--coverage`, report line, branch and
function coverage of `src/`, write lcov and JSON on request, and fail the run
under a configured floor.

## Acceptance criteria
- [x] AC1: with moltest-coverage in `[dev-deps]` and a `--coverage` profile, the run prints a coverage table after the test summary → `.github/e2e.sh` step 2
- [x] AC2: the table lists every measured file worst first, with lines, branches and functions as covered/total and percent, and the missing line ranges (`12-14, 20`) → test: `text_report_lists_worst_first`
- [x] AC3: only files under `include` and not under `exclude` are measured; tests/ and dependencies never → tests: `only_included_files_are_measured`, `absolute_paths_become_relative_to_the_project`; e2e step 2
- [x] AC4: `.gcov` text from GCC and from `llvm-cov gcov` parses to the same records → tests: `parses_gcc_gcov`, `parses_llvm_gcov`
- [x] AC5: under `fail_under` (lines) or `fail_under_branches`, the run exits 1 and says which floor and by how much → `.github/e2e.sh` step 3
- [x] AC6: `lcov = "<path>"` writes a tracefile genhtml accepts (SF/FN/FNDA/BRDA/DA/LF/LH/BRF/BRH/end_of_record) → test: `lcov_has_every_record`
- [x] AC7: `json = "<path>"` writes totals and per-file figures with missing lines → test: `json_has_totals_and_files`
- [x] AC8: a run not built with `--coverage` prints one line saying so and exits as the tests decided, unless `fail_under` is set → `.github/e2e.sh` step 1
- [x] AC9: an invalid config (unknown key, bad value, path outside the project) fails the run naming file and line → test: `config_errors_name_the_line`

## Design
See [ARCHITECTURE](../ARCHITECTURE.md), ADRs 0001-0003. Depends on moltest spec 004.

## Security notes
Config and tool output are untrusted text; tool started with argv, no shell;
outputs confined to the project root (SECURITY.md).

## Tasks
- [x] moltest plugin API v1 available (M1, moltest 0.3.0)
- [x] config.c + tests
- [x] gcov_parse.c + GCC and llvm fixtures
- [x] collect.c (flush, find, run tool)
- [x] render.c (text, lcov, JSON)
- [x] register.c and `.github/e2e.sh`
- [x] README usage
- [ ] CI on Linux, macOS, Windows (after molto#87 is released)

## Out of scope
HTML, Cobertura, exclusion markers, per-test contexts, combine (Backlog).
