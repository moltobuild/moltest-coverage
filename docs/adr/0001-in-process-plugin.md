# 0001 An in-process plugin of moltest — Status: Accepted
Date: 2026-10-04

## Context
Coverage for C needs three steps: instrument (the compiler, `--coverage`),
collect (the instrumented binary writes `.gcda`), report (a tool reads them).
Tools such as gcovr run after the tests, as a separate command. moltest's
ADR 0002 makes in-process reporters the first plugin model.

## Decision
moltest-coverage is a molto package (static library, consumed from source)
added to `[dev-deps]`. A constructor registers a moltest reporter; at the end
of the run it flushes the counters, produces the reports and can fail the run.
Adding the dependency is the whole setup; building with `--coverage` turns it
on.

## Alternatives considered
- **A CLI run after `molto test`** (gcovr-style): no moltest changes needed,
  but one more command to remember, and no access to the run (no per-test
  contexts, no failing the run in the same step).
- **A molto feature** (`molto test --coverage`): coverage would be molto's for
  every test framework; it is a later step for the profile half (M3) and does
  not exclude this.

## Consequences
- Needs moltest's plugin API v1: several reporters, registration from a
  constructor, and a way for a reporter to fail the run (moltest spec 004).
- The report runs inside the test binary, after the tests, with its cwd at the
  project root (verified with molto 0.47.0).
- Per-test contexts become possible later: the plugin sees every test start
  and end.
