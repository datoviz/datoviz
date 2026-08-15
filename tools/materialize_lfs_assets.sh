#!/usr/bin/env bash
set -euo pipefail

if ! git lfs version >/dev/null 2>&1; then
    echo "git-lfs is required to materialize release assets" >&2
    exit 2
fi

profile="${1:-release}"
case "$profile" in
    release|fonts)
        required_paths=(
            "assets/fonts/Roboto-Regular.ttf"
            "assets/fonts/RobotoMono-Medium.ttf"
        )
        ;;
    test)
        required_paths=(
            "assets/fonts/Roboto-Regular.ttf"
            "assets/fonts/RobotoMono-Medium.ttf"
            "assets/textures/world.200412.3x5400x2700.jpg"
            "examples/allen_ibl/prepared/allen_ibl_mesh_color.npy"
        )
        ;;
    *)
        echo "usage: $0 [release|fonts|test]" >&2
        exit 2
        ;;
esac

include_paths=$(IFS=,; echo "${required_paths[*]}")
git -C data lfs pull --include="$include_paths"

for relative_path in "${required_paths[@]}"; do
    path="data/$relative_path"
    if [[ ! -f "$path" ]] || grep -aq '^version https://git-lfs.github.com/spec/v1$' "$path"; then
        echo "$path is missing or still a Git LFS pointer" >&2
        exit 2
    fi
done
