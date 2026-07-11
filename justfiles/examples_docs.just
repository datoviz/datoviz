example-c name *args: build
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ "$(uname)" == "Darwin" ]]; then
        vk_lib="${VULKAN_SDK:-}/lib"
        if [[ -d "${vk_lib}" ]]; then
            fallback_path="${vk_lib}${DYLD_FALLBACK_LIBRARY_PATH:+:${DYLD_FALLBACK_LIBRARY_PATH}}"
            export DYLD_FALLBACK_LIBRARY_PATH="${fallback_path}"
        fi
    fi
    exe="./build/examples/c/{{name}}"
    if [[ ! -x "${exe}" && "{{name}}" != */* ]]; then
        matches=(./build/examples/c/*/{{name}})
        if [[ ${#matches[@]} -eq 1 && -x "${matches[0]}" ]]; then
            exe="${matches[0]}"
        fi
    fi
    dvzr_path=""
    if [[ "{{name}}" == "visuals/mesh" ]]; then
        if [[ " {{args}} " =~ (^|[[:space:]])record=([^[:space:]]+) ]]; then
            dvzr_path="${BASH_REMATCH[2]}"
        elif [[ " {{args}} " =~ (^|[[:space:]])--record[[:space:]]+([^[:space:]]+) ]]; then
            dvzr_path="${BASH_REMATCH[2]}"
        elif [[ " {{args}} " == *" record "* || " {{args}} " == "record" ]]; then
            dvzr_path="./build/examples/c/visuals/mesh.dvzr"
        fi
        if [[ -n "${dvzr_path}" ]]; then
            rm -rf -- "${dvzr_path}"
        fi
    fi
    "${exe}" {{args}}
    if [[ "{{name}}" == "visuals/mesh" ]]; then
        if [[ -n "${dvzr_path}" ]]; then
            python3 tools/dvzr_to_webgpu_stream.py \
                "${dvzr_path}" \
                examples/webgpu/streams/mesh_dvzr_wgsl.json
        fi
    fi
#

example-watch name *args:
    @python3 tools/watch_c_example.py "{{name}}" {{args}}
#

[positional-arguments]
examples *args: build
    @python3 tools/run_c_examples.py "$@"

[positional-arguments]
examples-run *args: build
    @just examples "$@"

[positional-arguments]
review *args:
    #!/usr/bin/env bash
    set -euo pipefail
    batch=""
    tool_args=("$@")
    if [[ "$#" -gt 0 && "${1}" != -* ]]; then
        batch="${1}"
        tool_args=("${@:2}")
    fi
    run_build=1
    for arg in "${tool_args[@]}"; do
        if [[ "${arg}" == "--code" ]]; then
            run_build=0
        fi
    done
    if [[ "${run_build}" -eq 1 ]]; then
        just build
    fi
    if [[ -n "${batch}" ]]; then
        python3 tools/run_c_examples.py --batch "${batch}" "${tool_args[@]}"
    else
        mapfile -t batches < <(python3 -c 'from pathlib import Path; import yaml; manifest = yaml.safe_load(Path("examples/c/MANIFEST.yaml").read_text()) or {}; print("\n".join((manifest.get("batches") or {}).keys()))')
        if [[ "${#batches[@]}" -eq 0 ]]; then
            echo "No review batches defined." >&2
            exit 1
        fi
        for batch_name in "${batches[@]}"; do
            python3 tools/run_c_examples.py --batch "${batch_name}" "${tool_args[@]}"
        done
    fi

review-batches:
    #!/usr/bin/env python3
    import yaml
    from pathlib import Path

    manifest = yaml.safe_load(Path("examples/c/MANIFEST.yaml").read_text())
    batches = manifest.get("batches") or {}
    if not batches:
        print("No review batches defined.")
    else:
        for name, ids in sorted(batches.items()):
            print(f"{name:36s} {len(ids)} examples")

# Smoke-test fenced code blocks in documentation markdown files.
# Prerequisites: build (for C) and ctypes (for Python array facade).
doctest lang="both":
    @python3 tools/doctest.py --lang {{lang}} docs/index.md

docs-doctest lang="both":
    @just doctest "{{lang}}"

quickstart-c: build
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p build/examples/docs
    "${CC:-cc}" examples/docs/quickstart.c -o build/examples/docs/quickstart \
        -Iinclude -Lbuild/src -Wl,-rpath,"$PWD/build/src" -ldatoviz -lm

quickstart-check: quickstart-c
    @DVZ_QUICKSTART_TEST=1 python3 examples/docs/quickstart.py
    @./build/examples/docs/quickstart --frames 1

# Run all Python examples and generate screenshots in data/gallery/.
gallery-screenshots filter="":
    @echo "Legacy Python gallery screenshot flow is quarantined. Use just capture-gallery or just gallery-refresh." >&2
    @exit 2


# Capture v0.4 C gallery screenshots into data/gallery/v0.4/.
[positional-arguments]
capture-gallery *args: build
    @python3 tools/capture_gallery.py "$@"

[positional-arguments]
gallery-capture *args: build
    @just capture-gallery "$@"


# Refresh v0.4 gallery screenshots and generated docs with cached native captures.
[positional-arguments]
gallery-refresh *args: build
    @python3 tools/capture_gallery.py --all-screenshot --cache --jobs auto "$@"
    @python3 tools/build_gallery_webp.py
    @python3 tools/build_gallery_animations.py
    @python3 tools/check_gallery_media_pipeline.py
    @python tools/build_gallery.py
    @python3 tools/build_examples_manifest.py
    @python3 tools/build_capabilities.py
    @python3 tools/check_gallery_media.py
    @git diff --check


# Check that gallery screenshots exist, are nonblank, and match cached fingerprints.
[positional-arguments]
check-gallery-media *args:
    @python3 tools/check_gallery_media.py "$@"

[positional-arguments]
gallery-media-check *args:
    @just check-gallery-media "$@"


# Generate build-local WebP derivatives for gallery screenshots.
[positional-arguments]
gallery-webp *args:
    @python3 tools/build_gallery_webp.py "$@"

[positional-arguments]
gallery-webp-build *args:
    @just gallery-webp "$@"


# Generate build-local animated WebP previews for gallery examples.
[positional-arguments]
gallery-animations *args:
    @python3 tools/build_gallery_animations.py "$@"

[positional-arguments]
check-gallery-media-pipeline *args:
    @python3 tools/check_gallery_media_pipeline.py "$@"


# Build the build-local gallery media comparison report.
[positional-arguments]
gallery-media-compare *args:
    @python3 tools/compare_gallery_media.py "$@"


# Serve the build-local gallery media comparison report.
gallery-media-compare-serve:
    #!/usr/bin/env bash
    set -euo pipefail

    root="build/gallery-media-compare"
    if [[ ! -f "${root}/index.html" ]]; then
        echo "Missing ${root}/index.html; run: just gallery-media-compare --all-animated --force" >&2
        exit 2
    fi

    host="${DATOVIZ_GALLERY_COMPARE_HOST:-localhost}"
    start_port="${DATOVIZ_GALLERY_COMPARE_PORT:-8295}"
    end_port=$((start_port + 20))

    port=$(python3 - "$host" "$start_port" "$end_port" <<'PY'
    import socket
    import sys

    host = sys.argv[1]
    start_port = int(sys.argv[2])
    end_port = int(sys.argv[3])

    for port in range(start_port, end_port + 1):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                sock.bind((host, port))
            except OSError:
                continue
        print(port)
        break
    else:
        sys.exit(f"no free port found on {host}:{start_port}-{end_port}")
    PY
    )

    if [ "$port" != "$start_port" ]; then
        echo "Port ${host}:${start_port} is busy; serving gallery compare on ${host}:${port}."
    fi

    echo "Serving gallery compare at http://${host}:${port}/"
    python3 -m http.server "$port" --bind "$host" --directory "$root"


serve-gallery-compare: gallery-media-compare-serve


# Capture screenshots for the Start Here / Get Started documentation pages.
capture-start: build
    @python3 tools/capture_gallery.py --lane start
    @python3 tools/build_gallery_webp.py --lane start

gallery-capture-start: capture-start


# Capture the native C screenshots used by the local v0.4 landing prototype.
capture-landing: build
    @python3 tools/capture_gallery.py --landing

gallery-capture-landing: capture-landing


# Build the gallery Markdown files and machine-readable example manifests.
gallery:
    @echo "Generating build-local gallery WebP assets..."
    @python3 tools/build_gallery_webp.py
    @python3 tools/build_gallery_animations.py
    @python3 tools/check_gallery_media_pipeline.py
    @just docs-generate

docs-generate:
    @python3 tools/build_gallery.py
    @python3 tools/build_examples_manifest.py
    @python3 tools/build_capabilities.py

docs-check-generated:
    @python3 tools/check_generated_docs.py

check-example-manifests:
    @python3 tools/check_example_ids.py
    @python3 tools/check_example_manifests.py


# -------------------------------------------------------------------------------------------------
# Documentation
# -------------------------------------------------------------------------------------------------

doc: api-docs #gallery #headers
#

docs: doc
#

api-docs:
    @python3 tools/build_api_c.py
#

docs-api: api-docs
#

check-api-docs:
    @python3 tools/build_api_c.py --check
#

docs-api-check: check-api-docs
#

docs-assets:
    @python3 tools/build_gallery_webp.py --require-image-dir --quiet-missing
#

serve: docs-assets wasm-scene-build-if-needed
    #!/usr/bin/env bash
    set -euo pipefail

    host="${DATOVIZ_DOCS_HOST:-localhost}"
    start_port="${DATOVIZ_DOCS_PORT:-8294}"
    end_port=$((start_port + 20))

    port=$(python3 - "$host" "$start_port" "$end_port" <<'PY'
    import socket
    import sys

    host = sys.argv[1]
    start_port = int(sys.argv[2])
    end_port = int(sys.argv[3])

    for port in range(start_port, end_port + 1):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                sock.bind((host, port))
            except OSError:
                continue
        print(port)
        break
    else:
        sys.exit(f"no free port found on {host}:{start_port}-{end_port}")
    PY
    )

    if [ "$port" != "$start_port" ]; then
        echo "Port ${host}:${start_port} is busy; serving docs on ${host}:${port}."
    fi

    uv run --with mkdocs-material --with 'mkdocstrings[python]' --with pillow mkdocs serve -a "${host}:${port}"
#

docs-serve: serve
#

# Publish the mkdocs website on GitHub Pages.
publish confirm='no':
    #!/usr/bin/env bash
    set -e
    if [ "{{confirm}}" != "yes" ]; then
        echo "Refusing docs publish without confirm=yes." >&2
        exit 2
    fi

    # Get the current branch name
    current_branch=$(git rev-parse --abbrev-ref HEAD)

    # Check if the current branch is "main"
    if [ "$current_branch" != "main" ]; then
        echo "You can only publish the documentation from the main branch (current branch is '$current_branch')."
        exit 1
    fi

    pushd ../datoviz.github.io
    git pull
    mkdocs gh-deploy --config-file ../datoviz/mkdocs.yml --remote-branch main
    popd
#

docs-publish confirm='no':
    @just publish "{{confirm}}"
#
