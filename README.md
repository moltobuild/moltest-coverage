# moltest-coverage

[![CI](https://github.com/moltobuild/moltest-coverage/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/moltobuild/moltest-coverage/actions/workflows/ci.yml)

Code coverage for C and C++ suites run by [moltest](https://github.com/moltobuild/moltest).
Add it to your development dependencies, build your tests with `--coverage`,
and every `molto test` ends with a coverage report of `src/`.

This checkout uses moltest v0.4.0 (reporter API v1) and requires Molto with
RFC-0024 support.

## What it does

```text
moltest_coverage (gcov-13)

File                  Lines     Branches  Functions  Missing
src/parser.c  112/140  80.0%  41/62  66.1%  9/10  90.0%  44-47, 88, 120-131
src/lexer.c    96/101  95.0%  30/32  93.8%  6/6  100.0%  77
TOTAL         208/241  86.3%  71/94  75.5%  15/16  93.8%

run failed: moltest_coverage: line coverage 86.3% is under fail_under = 90.0 by 3.7 points
```

Without a coverage build it says so in one line and stays out of the way, even
with a floor set: the floor applies to coverage runs.

- Line, branch and function coverage of `src/`, worst file first, with the
  lines no test reached.
- `fail_under`: the run fails when coverage drops under the floor.
- `coverage.lcov` for Codecov, Coveralls and genhtml; `coverage.json` for
  anything else.
- GCC and Clang, through the `gcov` each of them ships.

## Using it

```sh
molto add git+https://github.com/moltobuild/moltest#v0.4.0 --dev
molto add moltest_coverage --dev --path ../moltest-coverage
```

The package is named `moltest_coverage` in `[dev-deps]`. It links against your
own moltest v0.4.0, which must also be in `[dev-deps]`. The path refers to a
checkout containing this migration. After a compatible release is published,
replace it with that exact release tag.

Then measure with molto's built-in coverage profile (molto 0.48.0 or later):

```sh
molto test --profile coverage
```

With molto 0.47.2, which has no coverage profile, declare one yourself and run
`molto test --profile custom`:

```toml
[profile.custom]
opt_level = 0
debug_info = true
flags = ["--coverage"]
```

Optional `moltest-coverage.toml` at the project root:

```toml
include    = ["src"]
exclude    = ["src/generated"]
fail_under = 80.0
lcov       = "build/coverage.lcov"
json       = "build/coverage.json"
```

## How it works

A moltest plugin: a constructor registers a reporter. When the run starts it
erases the last run's `.gcda` files; when it ends it flushes the counters
(`__gcov_dump`), runs the gcov that matches your compiler (`gcov-<N>`,
`llvm-cov gcov`, `gcov`; or `tool` in the config) over the data of `src/`, and
reports. The plugin is itself compiled and linked with `--coverage`, which is
what puts the coverage runtime in every test binary that links it
([ARCHITECTURE](docs/ARCHITECTURE.md)).

A suite of several executables (`mode = "per_file"`) is one run: molto tells
each executable its place (`MOLTO_TEST_INDEX` of `MOLTO_TEST_COUNT`, molto
RFC-0020), so only the first erases and only the last reports and applies the
floors, over what all of them ran. That needs molto 0.50.0 or later; with an
older molto each executable is measured, and judged, on its own (KI-3).

## License

Apache-2.0; see [LICENSE](LICENSE) and [NOTICE](NOTICE).

## Manifest packages (RFC-0024)

This checkout requires a Molto build with RFC-0024 support. Project.toml is
the only consumer description; run `molto package` before tagging a release.
Older release tags still use recipes and require older Molto consumers.
CI temporarily builds the immutable Molto revision in `MOLTO_SOURCE_REF`.

The manifest pins moltest `v0.4.0` as a runtime dependency. If your project
also names moltest, use the same `v0.4.0` tag so the dependency graph shares one
runner. Published plugin tags predating this migration still require their
older runner; use a release containing this change with moltest `v0.4.0`.
