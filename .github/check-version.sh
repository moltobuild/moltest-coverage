#!/bin/sh
#
# Every place this package states its version, against the one asked for.
#
#   check-version.sh <version>      e.g. 0.1.0, or v0.1.0 (a tag)
#
# Project.toml is what molto builds and recipe.toml is what consumers read; a
# release where the two disagree is a red run instead of a published one.
set -eu

want=${1#v}
fail=0

check() {
    if [ "$2" = "$want" ]; then
        echo "ok   $1: $2"
    else
        echo "FAIL $1: $2, expected $want"
        fail=1
    fi
}

toml_version() {
    sed -n 's/^version *= *"\(.*\)".*/\1/p' "$1" | head -1
}

check Project.toml "$(toml_version Project.toml)"
check recipe.toml "$(toml_version recipe.toml)"

exit $fail
