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

# Check compiled Vulkan tutorial chapter, shader, build, and documentation synchronization.
vulkan-tutorial-check:
    python3 tools/check_vulkan_tutorial.py

# Run all three compiled tutorial chapters offscreen and validate their captures.
vulkan-tutorial-smoke: build
    python3 tools/run_vulkan_tutorial.py

# Build all three tutorial chapters against an installed prefix, then validate them offscreen.
vulkan-tutorial-installed-smoke prefix runtime_dir="":
    #!/usr/bin/env bash
    set -euo pipefail
    args=(--installed-prefix "{{prefix}}")
    if [[ -n "{{runtime_dir}}" ]]; then
        args+=(--runtime-dir "{{runtime_dir}}")
    fi
    python3 tools/run_vulkan_tutorial.py "${args[@]}"

# Check mechanically derived public status facts for drift.
docs-status-check:
    python3 tools/check_docs_status.py

# Backward-compatible spelling for callers that use the check-* convention.
check-docs-status: docs-status-check

# Also compile/run Quickstart fixtures and validate generated API/status facts.
check-doc-snippets: check-howto-snippets vulkan-tutorial-check quickstart-check docs-api-check docs-status-check
