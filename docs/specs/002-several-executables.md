# 002 Several executables, one measurement — Status: Done
## Goal
A suite of several test executables (`per_file`, molto RFC-0021's isolated
tests) is measured and judged as one run: counters erased once, before the
first executable, and one report, after the last (KI-3, ADR 0004).

## Acceptance criteria
- [x] AC1: no `MOLTO_TEST_INDEX`/`MOLTO_TEST_COUNT`: first and last at once, today's behaviour → test: absent_variables_make_a_run_of_its_own
- [x] AC2: index 1 of n is the first and not the last; n of n the last and not the first; 1 of 1 both → test: the_first_and_the_last_are_told_apart
- [x] AC3: one variable missing, not a number, 0, or an index over the count: invalid, measured as a run of its own → test: a_position_that_does_not_parse_is_a_run_of_its_own
- [x] AC4: only the first erases the `.gcda` files; only the last collects, prints, writes lcov/JSON and applies the floors; the others print one line saying the report comes with the last → test: e2e step 4
- [x] AC5: a `per_file` suite of two executables that cover 100% together passes `fail_under = 90` with one report at 100% → test: e2e step 4

## Design
ADR 0004. `cov_position_parse` (src/position.c) is pure; register.c reads the
environment once in `on_run_start` and acts on the result.

## Security notes
Two environment variables parsed as unsigned decimals, digits only; anything
else is rejected and the safe behaviour (a run of its own) applies.

## Tasks
- [x] `cov_position_parse` and its unit tests (AC1-AC3)
- [x] register.c: erase in the first, report in the last (AC4)
- [x] e2e step 4 expects a pass at 100% (AC5); passes on molto master
- [ ] CI on molto 0.50.0, the first release with RFC-0020
- [ ] KI-3 resolved, ADR 0004 Accepted, release 0.2.0

## Out of scope
Executables run in parallel: molto RFC-0020 keeps the first and the last alone.
