# moltest-coverage
Code coverage for C and C++ suites run by moltest. Added as a development
dependency, it registers itself with moltest and, at the end of a run built
with `--coverage`, reports which lines, branches and functions of `src/` the
suite executed: in the terminal, as lcov and JSON, and as a failed run when
coverage falls under a configured floor. The first community-style plugin of
moltest, and what lets molto drop its Makefile `coverage` target.

## Current focus
Milestone: M4 - Several executables · Spec: specs/002-several-executables.md (done) · Next step: merge, resolve KI-3, release 0.2.0

## Docs
[ROADMAP](ROADMAP.md) · [ARCHITECTURE](ARCHITECTURE.md) · [SECURITY](SECURITY.md) ·
[VALIDATION](VALIDATION.md) · [DEVELOPMENT](DEVELOPMENT.md) ·
[DEPENDENCIES](DEPENDENCIES.md) · [PROGRESS](PROGRESS.md) ·
[KNOWN_ISSUES](KNOWN_ISSUES.md) · [specs/](specs/) · [adr/](adr/)
