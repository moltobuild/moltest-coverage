# Manifest package migration — Status: Done

## Goal
Adopt Molto RFC-0024: describe the consumer package with Project.toml alone.

## Acceptance criteria
- [x] The root recipe is removed and public options are declared in the manifest.
- [x] `molto package` builds the tracked, pruned consumer copy.
- [x] The existing suite and consumer checks pass using the migrated moltest.
- [x] CI uses a Molto revision supporting RFC-0024 and checks packaging.
- [x] Release version checks use the manifest without requiring a recipe.

## Scope
Keep existing package versions; release tagging and version bumps happen separately.
Use immutable dependency commits until compatible release tags are published.
