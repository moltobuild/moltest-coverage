# Validation

## Commands
| Check | Command |
|---|---|
| Build | `molto build` |
| Tests | `molto test` |
| Format | `molto fmt --check` |
| Lint | `molto lint` |

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
