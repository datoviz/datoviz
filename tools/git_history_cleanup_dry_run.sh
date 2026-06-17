#!/usr/bin/env bash
# Estimate the pre-RC Git history cleanup in a disposable mirror clone.
#
# This script never rewrites the current checkout and never pushes. It clones the current
# repository into /tmp by default, runs git-filter-repo there, and reports size/ref diagnostics.

set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
RUN_ID="$(date +%Y%m%d-%H%M%S)"
BASE_DIR="${DVZ_HISTORY_CLEANUP_TMP:-/tmp/datoviz-history-cleanup-${RUN_ID}}"
MIRROR="${BASE_DIR}/datoviz-clean.git"

PATH_ARGS=(
    --path docs/assets/references/
    --path bin/vulkan/
    --path libs/vulkan/
    --path libs/shaderc/
    --path libs/swiftshader/
    --path v0.3/
    --path-glob 'external/vulkan/*.hpp'
    --path-glob 'external/vulkan/*.cppm'
)

CLEANUP_PATHS=(
    docs/assets/references/
    bin/vulkan/
    libs/vulkan/
    libs/shaderc/
    libs/swiftshader/
    v0.3/
    'external/vulkan/*.hpp'
    'external/vulkan/*.cppm'
)

require_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: required tool '$1' is not available" >&2
        exit 1
    fi
}

print_pack_size() {
    git -C "$1" count-objects -vH | awk -F': ' '/size-pack/ {print $2}'
}

print_head_tree_size() {
    git -C "$1" ls-tree -r -l HEAD | awk '{sum += $4} END {printf "%.2f MiB", sum / 1024 / 1024}'
}

print_ref_counts() {
    git -C "$1" for-each-ref --format='%(refname)' refs/heads refs/remotes refs/tags |
        awk '
            /^refs\/heads\// { heads++ }
            /^refs\/remotes\// { remotes++ }
            /^refs\/tags\// { tags++ }
            END { printf "heads %d, remotes %d, tags %d\n", heads + 0, remotes + 0, tags + 0 }
        '
}

print_refs_with_paths() {
    local repo="$1"
    local heading="$2"

    echo "$heading"
    git -C "$repo" for-each-ref --format='%(refname:short)' refs/heads refs/remotes refs/tags |
        while IFS= read -r ref; do
            if git -C "$repo" log --format=%H --max-count=1 "$ref" -- "${CLEANUP_PATHS[@]}" |
                grep -q .; then
                echo "  $ref"
            fi
        done
}

main() {
    require_tool git
    require_tool git-filter-repo

    case "$BASE_DIR" in
        /tmp/* | /var/folders/*) ;;
        *)
            echo "error: refusing to use non-temporary DVZ_HISTORY_CLEANUP_TMP: $BASE_DIR" >&2
            exit 1
            ;;
    esac

    rm -rf "$BASE_DIR"
    mkdir -p "$BASE_DIR"

    echo "source: $ROOT"
    echo "mirror: $MIRROR"
    echo

    git clone --mirror --no-local "$ROOT" "$MIRROR"

    echo "Before filter:"
    echo "  pack size: $(print_pack_size "$MIRROR")"
    echo "  HEAD tree: $(print_head_tree_size "$MIRROR")"
    echo "  refs: $(print_ref_counts "$MIRROR")"
    print_refs_with_paths "$MIRROR" "  refs containing cleanup paths:"
    echo

    git -C "$MIRROR" filter-repo --force --invert-paths "${PATH_ARGS[@]}"
    git -C "$MIRROR" gc --prune=now --aggressive

    echo
    echo "After filter + gc:"
    echo "  pack size: $(print_pack_size "$MIRROR")"
    echo "  HEAD tree: $(print_head_tree_size "$MIRROR")"
    echo "  refs: $(print_ref_counts "$MIRROR")"
    print_refs_with_paths "$MIRROR" "  refs still containing cleanup paths:"
    echo
    echo "Disposable mirror retained for inspection:"
    echo "  $MIRROR"
}

main "$@"
