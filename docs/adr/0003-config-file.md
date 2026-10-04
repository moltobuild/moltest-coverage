# 0003 moltest-coverage.toml configures the plugin — Status: Accepted
Date: 2026-10-04

## Context
The report needs settings: what to measure, the floor, which outputs. The
dependency policy forbids a TOML library.

## Decision
An optional `moltest-coverage.toml` at the project root, read with a small
parser of our own that accepts a TOML subset: comments, `key = value` with
strings, numbers, booleans and arrays of strings, and no tables.

```toml
include    = ["src"]          # what is measured (default)
exclude    = ["src/generated"]
fail_under = 80.0             # lines, percent; absent = no floor
fail_under_branches = 60.0
lcov       = "build/coverage.lcov"   # absent = not written
json       = "build/coverage.json"
tool       = "gcov-13"        # absent = derived from the compiler
```

Unknown keys and anything outside the subset are errors that name the line:
a misspelt `fail_under` must not silently disable the floor.

## Alternatives considered
- **Environment variables**: zero parsing, but settings a project wants in
  version control end up in CI scripts.
- **A table in Project.toml**: one file, but molto ignores unknown tables and
  could claim the name later; and the parser problem is the same.

## Consequences
- A config reader to test, kept small by the subset.
- Absent file = defaults: measure `src/`, print the report, no floor, no files.
