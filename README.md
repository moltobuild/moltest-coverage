# moltest-coverage

[![CI](https://github.com/moltobuild/moltest-coverage/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/moltobuild/moltest-coverage/actions/workflows/ci.yml)

Code coverage for C and C++ suites run by [moltest](https://github.com/moltobuild/moltest).
Add it to your development dependencies, build your tests with `--coverage`,
and every `molto test` ends with a coverage report of `src/`.

> **Status: MVP implemented, not released.** Needs moltest 0.3.0 and a molto
> with moltobuild/molto#87 (a dependency's flags reaching the link line). See
> [docs/PLAN.md](docs/PLAN.md).

## What it does

```text
moltest_coverage (gcov-13)

File                  Lines     Branches  Functions  Missing
src/parser.c  112/140  80.0%  41/62  66.1%  9/10  90.0%  44-47, 88, 120-131
src/lexer.c    96/101  95.0%  30/32  93.8%  6/6  100.0%  77
TOTAL         208/241  86.3%  71/94  75.5%  15/16  93.8%

run failed: moltest_coverage: line coverage 86.3% is under fail_under = 90.0 by 3.7 points
```

Without a coverage build it says so in one line and stays out of the way.

- Line, branch and function coverage of `src/`, worst file first, with the
  lines no test reached.
- `fail_under`: the run fails when coverage drops under the floor.
- `coverage.lcov` for Codecov, Coveralls and genhtml; `coverage.json` for
  anything else.
- GCC and Clang, through the `gcov` each of them ships.

## Using it

```sh
molto add git+https://github.com/moltobuild/moltest-coverage --dev
```

The package is named `moltest_coverage` in `[dev-deps]`. It links against your
own moltest (0.3.0 or later), which must also be in `[dev-deps]`.

```toml
[profile.custom]        # until molto has a built-in coverage profile
opt_level = 0
debug_info = true
flags = ["--coverage"]
```

```sh
molto test --profile custom
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

## License

Apache-2.0; see [LICENSE](LICENSE) and [NOTICE](NOTICE).
