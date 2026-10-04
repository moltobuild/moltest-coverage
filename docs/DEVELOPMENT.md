# Development

```sh
molto build
molto test
```

## Trying it on a project
```toml
[dev-deps]
moltest = { git = "https://github.com/moltobuild/moltest", branch = "master" }
moltest_coverage = { path = "../moltest-coverage" }

[profile.custom]          # until molto has a built-in coverage profile
opt_level = 0
debug_info = true
flags = ["--coverage"]
```
```sh
molto test --profile custom
```

## Conventions
Same as moltest: C23 (`-std=c2x`), molto preset for format and lint,
public symbols prefixed `moltest_coverage_`, Conventional Commits.
