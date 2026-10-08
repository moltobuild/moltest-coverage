# moltest-coverage
Code coverage for C and C++ suites run by moltest. Added as a development
dependency, it registers itself with moltest and, at the end of a run built
with `--coverage`, reports which lines, branches and functions of `src/` the
suite executed: in the terminal, as lcov and JSON, and as a failed run when
coverage falls under a configured floor. The first community-style plugin of
moltest, and what lets molto drop its Makefile `coverage` target.

## Current focus
Milestone: Moltest 0.4.0 compatibility · Spec: [004](specs/004-moltest-0.4.0.md) · Next step: merge compatibility PR and publish a new plugin release

## Docs
[ROADMAP](ROADMAP.md) · [ARCHITECTURE](ARCHITECTURE.md) · [SECURITY](SECURITY.md) ·
[VALIDATION](VALIDATION.md) · [DEVELOPMENT](DEVELOPMENT.md) ·
[DEPENDENCIES](DEPENDENCIES.md) · [PROGRESS](PROGRESS.md) ·
[KNOWN_ISSUES](KNOWN_ISSUES.md) · [specs/](specs/) · [adr/](adr/)

## Package migration
Current task: RFC-0024 ([spec](specs/003-manifest-package.md)); remove the recipe and validate the manifest consumer interface.
