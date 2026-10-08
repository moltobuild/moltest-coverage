#!/bin/sh
#
# Install the molto release named by MOLTO_VERSION onto PATH, after checking it
# is the file the release hashed.
#
#   install-molto.sh <asset suffix> <sha256 command>
#
# The suffix is the part of the asset name after the version
# (`x86_64-linux`, `arm64-macos`, `x86_64-windows.exe`). The hash command is
# passed in because macOS has no sha256sum and Linux has no shasum by default.
set -eu

suffix=$1
sha=$2
asset="molto-${MOLTO_VERSION:-source}-${suffix}"
dir="${RUNNER_TEMP:-/tmp}/molto-bin"

mkdir -p "$dir"
cd "$dir"
case "$suffix" in
    *.exe) name=molto.exe ;;
    *) name=molto ;;
esac

# RFC-0024 is not in a released binary yet. Pin its immutable source revision
# until a release can replace MOLTO_SOURCE_REF in the workflow.
if [ -n "${MOLTO_SOURCE_REF:-}" ]; then
    git init --quiet source
    git -C source fetch --quiet --depth 1 https://github.com/moltobuild/molto.git "$MOLTO_SOURCE_REF"
    git -C source checkout --quiet --detach FETCH_HEAD
    [ "$(git -C source rev-parse HEAD)" = "$MOLTO_SOURCE_REF" ]
    make -C source build CC="${C_COMPILER:-cc}" -j2
    cp "source/build/$name" "$name"
else
    curl -fsSLO --retry 3 "${MOLTO_RELEASES}/v${MOLTO_VERSION}/${asset}"
    curl -fsSLO --retry 3 "${MOLTO_RELEASES}/v${MOLTO_VERSION}/SHA256SUMS"
    $sha --check --ignore-missing SHA256SUMS
    mv "$asset" "$name"
fi
chmod +x "$name"

# Under MSYS2 the shell does not inherit the runner's PATH (setup-msys2's
# default `path-type: minimal`), so GITHUB_PATH would not reach it; its own
# /usr/local/bin is on every MSYS2 shell's PATH.
if command -v cygpath >/dev/null 2>&1; then
    mkdir -p /usr/local/bin
    mv "$name" /usr/local/bin/
    molto --version
else
    echo "$dir" >> "$GITHUB_PATH"
    "./$name" --version
fi
