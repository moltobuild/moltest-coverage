# Known issues

## KI-1 A consumer's test binary fails to link outside a coverage profile — Status: Open
- Repro: a project with moltest_coverage in `[dev-deps]`; `molto test` (debug).
- Expected: the suite runs and the plugin says there is no coverage data.
  Actual: `Undefined symbols: ___gcov_dump, _llvm_gcda_emit_arcs ...`.
- Cause: the plugin is compiled with `--coverage` (it calls `__gcov_dump`), and
  molto passes a dependency's `[artifacts] flags` to the compile lines of the
  tests but not to their link line (molto RFC-0009: "flags a consumer must
  compile with"). Only a profile with `--coverage` in its own flags links the
  runtime. Needs a decision (molto change or a different flush).
