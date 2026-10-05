# 0004 Erase in the first executable, report in the last — Status: Proposed
Date: 2026-10-05

## Context
A suite can be several executables (`per_file`, and molto RFC-0021's isolated
tests). The plugin runs inside each and sees only that one: each erases the
counters of the one before and applies the floor to its own part (KI-3).
libgcov merges counters across executables correctly; only who erases and who
judges is wrong, and nothing inside an executable tells it apart.

## Decision
Read the run position molto sets (RFC-0020): `MOLTO_TEST_INDEX` and
`MOLTO_TEST_COUNT`.
- Erase the `.gcda` files only when the index is 1.
- Collect, print, write lcov/JSON and apply the floors only when the index
  equals the count. The others print one line: the report comes with the last.
- Both absent (a binary run by hand, an older molto): first and last at once,
  today's behaviour. Present but unparsable: the same, with a warning, never a
  partial report presented as whole.

## Alternatives considered
- **Guess first and last** from timestamps or lock files under `build/`:
  breaks on a binary run by hand, an interrupted run, two runs at once.
- **Let molto report** after the last executable: molto RFC-0019 keeps the
  report in the test framework.
- **Never erase, report every time:** every executable but the last still
  judges a partial measurement.

## Consequences
- Needs a molto that ships RFC-0020; with an older one KI-3 remains.
- A run that crashes the last executable prints no report; its own failure says why.
- If molto runs executables in parallel, "index equals count" stops meaning
  "finished last" (RFC-0020, unresolved); this ADR is then superseded.
