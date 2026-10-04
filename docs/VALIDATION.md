# Validation

## Commands
| Check | Command |
|---|---|
| Build | `molto build` |
| Tests | `molto test` |
| Format | `molto fmt --check` |
| Lint | `molto lint` |

## CI
`.github/workflows/ci.yml`, molto pinned in `MOLTO_VERSION` (0.47.2, the first
with moltobuild/molto#87 and #90), installed by `.github/install-molto.sh`:

| Job | Runs on | Checks |
|---|---|---|
| Test | Linux (gcc), macOS (clang), Windows (MSYS2 gcc) | `molto build`, `molto test`, and that the plugin printed its own report |
| E2E | the same three | `.github/e2e.sh`: a `molto new` library, normal run, coverage run, missed floor with lcov and JSON |
| Style | Linux, LLVM 19 | `molto fmt --check`, `molto lint` (a gate) |

## Strategy
- Parsers and reports: unit tests on fixture `.gcov` texts and config files, no compiler needed.
- End to end: a fixture project under `tests/fixtures/` built with `--coverage`
  by molto in a child process; the test checks the report, the files and the exit status.
- Every acceptance criterion names its test.

## Definition of Done
- [ ] All acceptance criteria have passing tests
- [ ] Build, tests, format and lint pass
- [ ] SECURITY.md checklist reviewed
- [ ] Spec, ROADMAP and PLAN updated
- [ ] PROGRESS entry, commit, graphs refreshed
