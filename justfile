# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

MAINTAINER := "Cyrille Rossant <cyrille.rossant@gmail.com>"
DESCRIPTION := "A C library for high-performance GPU scientific visualization"
TEMPLATES_DIR := "templates"

# Use a non-system shell so macOS keeps DYLD_* variables from direnv/.envrc.
set shell := ["bash", "-cu"]



# -------------------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------------------
# Imported task groups
# -------------------------------------------------------------------------------------------------

import 'justfiles/maintenance.just'
import 'justfiles/release.just'
import 'justfiles/build.just'
import 'justfiles/packages_legacy.just'
import 'justfiles/bindings.just'
import 'justfiles/webgpu_wasm.just'
import 'justfiles/wheels.just'
import 'justfiles/diagnostics.just'
import 'justfiles/examples_docs.just'
import 'justfiles/test.just'

# Check syntax and public C identifiers in handwritten How-To, Start, homepage, and Reference snippets.
check-howto-snippets:
    python3 -m unittest tools/tests/test_check_howto_snippets.py
    python3 tools/check_howto_snippets.py

# Check that every code excerpt in the Vulkan course chapters matches its step program.
vulkan-course-check:
    python3 tools/check_vulkan_course.py

# Run every Vulkan course step program offscreen and validate its capture.
vulkan-course-smoke: build
    python3 tools/run_vulkan_course.py

# Build the Vulkan course steps against a published wheel, as chapter 1 tells readers to.
vulkan-course-wheel-smoke version:
    python3 tools/run_vulkan_course.py --wheel "datoviz=={{version}}"

# Build the Vulkan course steps against an installed prefix as a standalone consumer would.
vulkan-course-installed-smoke prefix runtime_dir="":
    #!/usr/bin/env bash
    set -euo pipefail
    args=(--installed-prefix "{{prefix}}")
    if [[ -n "{{runtime_dir}}" ]]; then
        args+=(--runtime-dir "{{runtime_dir}}")
    fi
    python3 tools/run_vulkan_course.py "${args[@]}"

# Check mechanically derived public status facts for drift.
docs-status-check:
    python3 tools/check_docs_status.py

# Backward-compatible spelling for callers that use the check-* convention.
check-docs-status: docs-status-check

# Also compile/run Quickstart fixtures and validate generated API/status facts.
check-doc-snippets: check-howto-snippets vulkan-course-check quickstart-check docs-api-check docs-status-check

# Build the developer-only deterministic MSDF atlas generator.
text-atlas-generator-build:
    cmake --build build --target datoviz_text_atlas_generate

# Generate an untracked canonical Source Sans product directory.
text-atlas-generate output_dir primary="assets/runtime/fonts/SourceSans3-Regular.ttf": text-atlas-generator-build
    build/src/scene/datoviz_text_atlas_generate --primary "{{primary}}" --output-dir "{{output_dir}}"

# Validate an approved deterministic MSDF atlas manifest and textual include.
text-atlas-check manifest="assets/runtime/text/default_msdf_atlas.json" include_root=".":
    python3 tools/check_text_atlas_manifest.py --manifest "{{manifest}}" --repo-root . --include-root "{{include_root}}"

# Serialize an untracked C++ product directory into candidate manifest/include files.
text-atlas-serialize input_dir output_dir:
    python3 tools/check_text_atlas_manifest.py --serialize --input-dir "{{input_dir}}" --output-dir "{{output_dir}}" --repo-root .

# Regenerate and serialize twice, then require exact byte identity.
text-atlas-repro-check primary="assets/runtime/fonts/SourceSans3-Regular.ttf": text-atlas-generator-build
    #!/usr/bin/env bash
    set -euo pipefail
    atlas_tmp=$(mktemp -d)
    trap 'rm -rf "$atlas_tmp"' EXIT
    for run in a b; do
        build/src/scene/datoviz_text_atlas_generate --primary "{{primary}}" --output-dir "$atlas_tmp/$run/product"
        python3 tools/check_text_atlas_manifest.py --serialize --input-dir "$atlas_tmp/$run/product" --output-dir "$atlas_tmp/$run/candidate" --repo-root .
    done
    for file in product.json atlas_32.rgba atlas_64.rgba atlas_128.rgba; do
        cmp "$atlas_tmp/a/product/$file" "$atlas_tmp/b/product/$file"
    done
    cmp "$atlas_tmp/a/candidate/assets/runtime/text/default_msdf_atlas.json" "$atlas_tmp/b/candidate/assets/runtime/text/default_msdf_atlas.json"
    cmp "$atlas_tmp/a/candidate/src/scene/text/generated/text_default_msdf_atlas.inc" "$atlas_tmp/b/candidate/src/scene/text/generated/text_default_msdf_atlas.inc"
