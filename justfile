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

# Check syntax and public C identifiers in handwritten How-To, Start, and homepage snippets.
check-howto-snippets:
    python3 tools/check_howto_snippets.py

# Also compile and run the complete Quickstart fixtures for one bounded frame.
check-doc-snippets: check-howto-snippets quickstart-check
