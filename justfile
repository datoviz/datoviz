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
