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

## Releasing
1. One PR bumps `version` in `Project.toml` and `recipe.toml`;
   `.github/check-version.sh <version>` checks both.
2. After it merges, tag the merge commit and push the tag:
   `git tag -a v0.1.0 -m "moltest-coverage 0.1.0" && git push origin v0.1.0`.
3. `.github/workflows/release.yml` checks the tag against both, runs the CI on
   three platforms, and publishes the GitHub Release. Consumers pin it with
   `tag = "v0.1.0"`.

Running the Release workflow by hand rehearses all of it without publishing.

## Conventions
Same as moltest: C23 (`-std=c2x`), molto preset for format and lint,
public symbols prefixed `moltest_coverage_`, Conventional Commits.
