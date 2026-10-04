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
           4. run the tool            gcov -b -c -p | llvm-cov gcov -b -c -p, in a temp dir
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
- A run without coverage data (not built with `--coverage`) says so and does
  not fail, unless `fail_under` is set: then it fails, because a floor nobody
  measured is not met.

## Open questions
- **moltest version coupling.** molto allows one version per package, so
  moltest-coverage cannot pin its own moltest; it has to use the consumer's.
  Likely: the recipe declares no `[deps]` on moltest and requires the consumer
  to have it, with the plugin API version checked at registration.
- **Tool choice.** Derived from the compiler that built the plugin
  (`__clang__` → `llvm-cov gcov`, GCC → `gcov-<major>` then `gcov`), overridable
  in the config. To validate on Windows (MSYS2 gcc).
