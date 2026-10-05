# Known issues

## KI-4 A test binary in a subfolder of tests/ measured nothing — Status: Resolved (0.2.1)
- Repro: a coverage run whose last executable is in a subfolder of `tests/`
  (`per_file` with `tests/units/test_b.c`, or a molto isolated test), a floor set.
- Expected: the report and the floor. Actual: `src not built for coverage`, no
  report, and the floor silently not applied: the run passed.
- Cause: `cov_profile_dir` cut two components off the binary's path, which is
  `build/<profile>` only for a binary directly under `tests/`.
- Found adopting molto's first isolated test (`tests/services/test_source_service_git`).
- Fix: the profile is found from the right as `tests` preceded by `build/<profile>`.
  Tests: `tests/test_profile_dir.c`; `.github/e2e.sh` step 4 puts its last
  executable in `tests/units/` (red before the fix).

## KI-3 A suite of several executables fails its floor at full coverage — Status: Resolved (0.2.0)
- Repro: `mode = "per_file"`, two test files that each cover one of two
  functions, `fail_under = 90`, a coverage profile.
- Expected: one report at 100%, the run passes. Actual: each executable reports
  50% and fails the floor; the second also reports the first's lines missing.
- Cause: every executable erases the `.gcda` files in `on_run_start` and
  reports and judges in `on_run_end`; it cannot tell the first or last of a run.
- Test: `.github/e2e.sh` step 4: one report at 100%, the run passes (spec 002).
- Fix: molto RFC-0020 tells each executable its place (`MOLTO_TEST_INDEX`,
  `MOLTO_TEST_COUNT`); erase only in the first, report only in the last (ADR 0004).
- Fixed: `cov_position_parse` and register.c (spec 002); e2e step 4 green on
  Linux, macOS and Windows with molto 0.50.0, the first release that sets them.

## KI-2 A floor failed every ordinary `molto test` — Status: Resolved (0.1.1)
- Repro: `fail_under` in moltest-coverage.toml, then `molto test` (no coverage profile).
- Expected: the suite passes; the floor applies to coverage runs. Actual: `run failed: a floor is set and nothing was measured`.
- Found adopting moltest-coverage in molto. Fixed by telling an uninstrumented build (no `.gcno`) from a coverage build where nothing ran; `.github/e2e.sh` step 1 now runs with a floor set.

## KI-1 A consumer's test binary fails to link outside a coverage profile — Status: Fixed in molto (moltobuild/molto#87), pending a release
- Repro: a project with moltest_coverage in `[dev-deps]`; `molto test` (debug).
- Expected: the suite runs and the plugin says there is no coverage data.
  Actual: `Undefined symbols: ___gcov_dump, _llvm_gcda_emit_arcs ...`.
- Cause: the plugin is compiled with `--coverage` (it calls `__gcov_dump`), and
  molto passes a dependency's `[artifacts] flags` to the compile lines of the
  tests but not to their link line (molto RFC-0009: "flags a consumer must
  compile with"). Only a profile with `--coverage` in its own flags links the
  runtime. Needs a decision (molto change or a different flush).
