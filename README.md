# moltest-coverage

Code coverage for C and C++ suites run by [moltest](https://github.com/moltobuild/moltest).
Add it to your development dependencies, build your tests with `--coverage`,
and every `molto test` ends with a coverage report of `src/`.

> **Status: design (M0).** Nothing is implemented yet; see [docs/PLAN.md](docs/PLAN.md).

## What it will do (0.1.0)

```text
moltest_coverage — src/ (gcov 13.2)

File               Lines           Branches        Functions   Missing
src/parser.c       112/140  80.0%   41/62  66.1%    9/10  90%  44-47, 88, 120-131
src/lexer.c         96/101  95.0%   30/32  93.8%    6/6  100%  77
TOTAL              208/241  86.3%   71/94  75.5%   15/16  94%

coverage 86.3% is under fail_under = 90.0 by 3.7 points
```

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

A moltest plugin: a constructor registers a reporter, and at the end of the run
it flushes the counters, runs the compiler's gcov over `src/`'s data and
reports ([ARCHITECTURE](docs/ARCHITECTURE.md)).

## License

Apache-2.0; see [LICENSE](LICENSE) and [NOTICE](NOTICE).
