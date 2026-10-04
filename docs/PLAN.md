# moltest-coverage
Code coverage for C and C++ suites run by moltest. Added as a development
dependency, it registers itself with moltest and, at the end of a run built
with `--coverage`, reports which lines, branches and functions of `src/` the
suite executed: in the terminal, as lcov and JSON, and as a failed run when
coverage falls under a configured floor. The first community-style plugin of
moltest, and what lets molto drop its Makefile `coverage` target.

## Current focus
Milestone: M2 - MVP · Spec: specs/001-mvp-report.md · Next step: release molto 0.47.1, get CI green, release 0.1.0

## Docs
[ROADMAP](ROADMAP.md) · [ARCHITECTURE](ARCHITECTURE.md) · [SECURITY](SECURITY.md) ·
[VALIDATION](VALIDATION.md) · [DEVELOPMENT](DEVELOPMENT.md) ·
[DEPENDENCIES](DEPENDENCIES.md) · [PROGRESS](PROGRESS.md) ·
[KNOWN_ISSUES](KNOWN_ISSUES.md) · [specs/](specs/) · [adr/](adr/)
