# 004 Moltest 0.4.0 compatibility — Status: Done

## Goal
Use the released moltest v0.4.0 runner in the plugin and consumer tests.

## Acceptance criteria
- [x] AC1: Manifest and generated lock resolve v0.4.0 → `molto test`.
- [x] AC2: Self-tests and reporter behavior pass → `molto test --profile coverage`.
- [x] AC3: Consumer integration passes with v0.4.0 → `.github/e2e.sh`.
- [x] AC4: CI and current dependency docs select the same tag → reference review.

## Design
Change the exact runner reference without changing the plugin API or behavior.
No new dependency, build script, input handling or output path is introduced.

## Tasks
- [x] Update the manifest, CI, e2e defaults and dependency documentation.
- [x] Regenerate the lock and run build, tests, coverage, e2e, format and lint.

## Out of scope
Publishing releases or moving existing tags.
