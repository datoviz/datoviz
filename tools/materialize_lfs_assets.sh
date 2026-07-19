#!/usr/bin/env bash
set -euo pipefail

if ! git lfs version >/dev/null 2>&1; then
    echo "git-lfs is required to materialize release assets" >&2
    exit 2
fi

git submodule foreach --recursive git lfs pull

font_path="data/assets/fonts/Roboto-Regular.ttf"
if grep -aq '^version https://git-lfs.github.com/spec/v1$' "$font_path"; then
    echo "$font_path is still a Git LFS pointer" >&2
    exit 2
fi
