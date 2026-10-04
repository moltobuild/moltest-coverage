# 0002 gcov's annotated text is the data source — Status: Accepted
Date: 2026-10-04

## Context
`--coverage` produces `.gcno` (at compile time) and `.gcda` (at run time). Their
binary format belongs to the compiler and changes between versions (gcov's
"version 'B23', prefer 'B14'"). GCC and Clang both ship a gcov that reads the
pair: `gcov` and `llvm-cov gcov`.

## Decision
Run the gcov that matches the compiler with `-b -c -p` in a temporary
directory, and parse the annotated `.gcov` files it writes:
`count:line:source`, `#####` for an executable line never run, `-` for none,
`branch N taken X%`, `function NAME called N returned X%`.

## Alternatives considered
- **Parse `.gcda`/`.gcno` directly**: no external tool, but a binary format
  that moves with every compiler release.
- **`gcov --json-format`**: structured, but GCC 9+ only and not offered by
  `llvm-cov gcov`.
- **Clang source-based coverage** (`-fprofile-instr-generate`, `llvm-cov
  export`): better data for Clang, nothing for GCC; Backlog.

## Consequences
- The text format is the one gcovr and lcov parse and has been stable for
  decades; one parser serves GCC and Clang.
- The right tool must be found: derived from the compiler that built the
  plugin, overridable with `tool` in the config.
