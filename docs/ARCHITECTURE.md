# Architecture

## Where it sits
```
molto test --profile coverage          (molto: compiles src/ and tests/ with --coverage)
 └─ <package>_tests                    (cwd = project root)
     ├─ moltest            runs the tests
     └─ moltest-coverage   reporter registered by a constructor (ADR 0001)
         on_run_end:
           1. flush counters          __gcov_dump()
           2. read config             ./moltest-coverage.toml (ADR 0003)
           3. find data               build/<profile>/obj/src/**/*.gcda
                                      (<profile> = parent of the test binary's dir)
           4. run the tool            gcov -b -c -t, from the project root (stdout,
                                      no temp files); paths made relative to the root
           5. parse .gcov text        lines, branches, functions (ADR 0002)
           6. report                  terminal; coverage.lcov; coverage.json
           7. gate                    fail the run when under fail_under
```

## Components (planned, src/)
| Unit | Role |
|---|---|
| `register.c` | constructor: registers the reporter with moltest |
| `config.c` | reads `moltest-coverage.toml` (a TOML subset, no dependency) |
| `collect.c` | flushes counters, finds `.gcda` files, runs the gcov tool |
| `gcov_parse.c` | parses annotated `.gcov` text into per-file line/branch/function records |
| `report_text.c`, `report_lcov.c`, `report_json.c` | the three outputs |

`gcov_parse` and the reports are pure (text in, records out; records in, text
out) and are tested without a compiler; `collect` is tested end to end.

## Key decisions
- In-process plugin of moltest: [ADR 0001](adr/0001-in-process-plugin.md)
- gcov's annotated text is the data source: [ADR 0002](adr/0002-gcov-text-format.md)
- `moltest-coverage.toml`: [ADR 0003](adr/0003-config-file.md)

## Invariants
- Only files under the configured `include` (default `src/`) are measured.
- No runtime dependency beyond moltest, libc and an external gcov tool.
- A run not built for coverage (no `.gcno` for the measured sources) says so
  and applies no floor: an ordinary `molto test` of a project that also
  measures itself must not fail on it. A coverage build in which none of the
  measured code ran is a measurement of zero, and a floor fails on it.

## Open questions
- **moltest version coupling — resolved.** molto allows one version per
  package, so the plugin links against the consumer's moltest: `src/` declares
  the reporter API v1 it uses (`src/moltest_api.h`) instead of including
  moltest.h, moltest refuses it by name if its API moves on, and
  `tests/test_moltest_api.c` checks the copy field by field.
- **The coverage runtime in a profile without `--coverage`** (KNOWN_ISSUES KI-1).
- **Tool choice.** Derived from the compiler that built the plugin
  (`__clang__` → `llvm-cov gcov`, GCC → `gcov-<major>` then `gcov`), overridable
  in the config. To validate on Windows (MSYS2 gcc).

## Manifest package interface

Project.toml is the only carried description (Molto RFC-0024). The plugin
exports include/ by convention and declares moltest in [deps], since its
sources call that API. The graph shares one runner with consumers. Development
configuration is ignored when the plugin is consumed.
