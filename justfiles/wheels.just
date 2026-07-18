showwheel:
    @just wheel-inspect
#

wheel-local platform_tag='':
    @just wheel "{{platform_tag}}"
#

wheel-matrix:
    @python tools/release_wheels/wheel_matrix.py
#

wheel-validate *args:
    @python tools/release_wheels/wheel_matrix.py --validate-dist {{args}}
#

wheel-stage *args:
    @python tools/release_wheels/stage_wheel.py {{args}}
#

wheel-build *args:
    @python tools/release_wheels/build_wheel.py {{args}}
#

wheel-inspect *args:
    @python tools/release_wheels/inspect_wheel.py {{args}}
#

wheel-check *args:
    @python tools/release_wheels/check_wheel.py {{args}}
#

release-source-bundle version:
    @python tools/release_source_bundle.py {{version}}
#

distribution-validate-local mode='all':
    @tools/validate_distribution_local.sh {{mode}}
#

distribution-preflight-local mode='all':
    @just distribution-validate-local "{{mode}}"
#

c-consumer-smoke:
    @just c-integration-smoke
#

wheel-ci-local platform_tag='' rebuild='0' render='0':
    #!/usr/bin/env sh
    set -e
    just wheel-matrix
    if [ "{{rebuild}}" = "1" ]; then
        just build
    fi
    just wheel-stage --clean
    if [ -n "{{platform_tag}}" ]; then
        just wheel-build --platform-tag "{{platform_tag}}"
    else
        just wheel-build
    fi
    just wheel-inspect
    if [ -n "{{platform_tag}}" ]; then
        just wheel-validate --platform-tag "{{platform_tag}}"
    fi
    just wheel-inspect --native-deps
    if [ "{{render}}" = "1" ]; then
        just wheel-check --precompiled-shaders --shaderc --cmake-consumer --render --qt-probe optional
    else
        just wheel-check --precompiled-shaders --shaderc --cmake-consumer --qt-probe optional
    fi
#

wheel-manylinux-docker arch='x86_64':
    #!/usr/bin/env sh
    set -e
    image="${DATOVIZ_MANYLINUX_IMAGE:-quay.io/pypa/manylinux_2_34_x86_64:latest}"
    case "{{arch}}" in
        x86_64) ;;
        *)
            echo "unsupported local manylinux Docker arch: {{arch}}" >&2
            echo "set DATOVIZ_MANYLINUX_IMAGE and extend this recipe after a native or emulated {{arch}} builder is proven" >&2
            exit 2
            ;;
    esac
    docker run --rm \
        -e DVZ_CMAKE_ARGS="${DVZ_CMAKE_ARGS:-}" \
        -e DVZ_WHEEL_RUNTIME_DIRS="${DVZ_WHEEL_RUNTIME_DIRS:-}" \
        -e DATOVIZ_MANYLINUX_PYTHON="${DATOVIZ_MANYLINUX_PYTHON:-}" \
        -e DATOVIZ_MANYLINUX_GENERATE_CTYPES="${DATOVIZ_MANYLINUX_GENERATE_CTYPES:-}" \
        -e DATOVIZ_HOST_UID="$(id -u)" \
        -e DATOVIZ_HOST_GID="$(id -g)" \
        -v "$PWD:/workspace" \
        -w /workspace \
        "$image" \
        bash tools/release_wheels/manylinux_build_inside.sh "{{arch}}"
#

wheel platform_tag='': build
    #!/usr/bin/env sh
    set -e
    just wheel-stage --clean
    if [ -n "{{platform_tag}}" ]; then
        just wheel-build --platform-tag "{{platform_tag}}"
    else
        just wheel-build
    fi
    just wheel-inspect
    if [ -n "{{platform_tag}}" ]; then
        just wheel-validate --platform-tag "{{platform_tag}}"
    fi
#

testpypi-check platform_tag dist_dir='dist':
    #!/usr/bin/env sh
    set -e
    just wheel-validate --dist-dir "{{dist_dir}}" --platform-tag "{{platform_tag}}"
    python -c "import twine" 2>/dev/null || {
        echo "twine is required: python -m pip install --upgrade twine" >&2
        exit 1
    }
    python -m twine check "{{dist_dir}}"/datoviz-*.whl
#

testpypi-check-all dist_dir='dist':
    #!/usr/bin/env sh
    set -e
    just wheel-validate --dist-dir "{{dist_dir}}"
    python -c "import twine" 2>/dev/null || {
        echo "twine is required: python -m pip install --upgrade twine" >&2
        exit 1
    }
    python -m twine check "{{dist_dir}}"/datoviz-*.whl
#

testpypi-upload platform_tag dist_dir='dist' confirm='no':
    #!/usr/bin/env sh
    set -e
    if [ "{{confirm}}" != "yes" ]; then
        echo "Refusing to upload to TestPyPI without confirm=yes" >&2
        echo "Run: just testpypi-upload {{platform_tag}} {{dist_dir}} yes" >&2
        exit 1
    fi
    just testpypi-check "{{platform_tag}}" "{{dist_dir}}"
    python -m twine upload --repository testpypi "{{dist_dir}}"/datoviz-*.whl
#

testpypi-upload-all dist_dir='dist' confirm='no':
    #!/usr/bin/env sh
    set -e
    if [ "{{confirm}}" != "yes" ]; then
        echo "Refusing to upload the full wheelhouse to TestPyPI without confirm=yes" >&2
        echo "Run: just testpypi-upload-all {{dist_dir}} yes" >&2
        exit 1
    fi
    just testpypi-check-all "{{dist_dir}}"
    python -m twine upload --repository testpypi "{{dist_dir}}"/datoviz-*.whl
#

pypi-install-smoke:
    #!/usr/bin/env bash

    # HACK: work around: ERROR: Can not perform a '--user' install. User site-packages are not
    # visible in this virtualenv
    # see https://github.com/gitpod-io/gitpod/issues/1997
    export PIP_USER=false

    case "$(uname -s)" in
    *CYGWIN*|*MINGW*|*MSYS*) BINDIR="Scripts" ;;
    *) BINDIR="bin" ;;
    esac

    # Create a temporary venv.
    rm -rf venv_pypi
    python -m venv venv_pypi
    pushd venv_pypi

    # Make sure Datoviz is not installed in the venv before we pip install it.
    $BINDIR/python -c "exec('try: import datoviz\nexcept: print(\"datoviz not yet installed\")\nelse: raise RuntimeError(\"datoviz already installed\")')"

    # Install datoviz from PyPI
    $BINDIR/pip install datoviz

    # Check the Datoviz demo.
    $BINDIR/python -c "import datoviz; datoviz.demo()"

    # Cleanup the venv.
    popd
    rm -rf venv_pypi
#

checkartifactversion temp_dir:
    #!/usr/bin/env sh
    set -e
    version=$(just version)
    wheel_file=$(find "{{temp_dir}}" -name 'datoviz*.whl' | head -n 1)
    if [ -z "$wheel_file" ]; then
        echo "❌ No wheel file found!"
        rm -rf "{{temp_dir}}"
        exit 1
    fi

    if ! echo "$wheel_file" | grep -q "$version"; then
        echo "❌ Version mismatch: wheel '$wheel_file' does not contain expected version '$version'"
        rm -rf "{{temp_dir}}"
        exit 1
    fi
#

wheel-artifact-version-check temp_dir:
    @just checkartifactversion "{{temp_dir}}"
#

[linux]
checkartifact RUN_ID="":
    #!/usr/bin/env sh
    set -e
    run_id={{RUN_ID}}
    if [ -z "$run_id" ]; then
        run_id=$(just runid)
    fi
    temp_dir=$(mktemp -d)
    gh run download $run_id -n wheel-linux_x86_64 -D $temp_dir
    just checkartifactversion $temp_dir
    just wheel-check $temp_dir/datoviz*.whl
    exit_code=$?
    rm -rf "${temp_dir}"
    exit $exit_code
#

wheel-artifact-check RUN_ID="":
    @just checkartifact "{{RUN_ID}}"
#

[macos]
checkartifact RUN_ID="":
    #!/usr/bin/env sh
    set -e
    run_id={{RUN_ID}}
    if [ -z "$run_id" ]; then
        run_id=$(just runid)
    fi

    arch_str={{arch()}}
    echo $arch_str
    if [[ "$arch_str" == "aarch64" ]]; then
        platform="arm64"
    else
        platform="x86_64"
    fi

    temp_dir=$(mktemp -d)
    gh run download $run_id -n "wheel-macosx_$platform" -D $temp_dir
    just checkartifactversion $temp_dir
    ls $temp_dir/datoviz*.whl
    just wheel-check $temp_dir/datoviz*.whl
    exit_code=$?
    rm -rf "${temp_dir}"
    exit $exit_code
#

[windows]
checkartifact RUN_ID="":
    #!/usr/bin/env sh
    set -e
    run_id={{RUN_ID}}
    if [ -z "$run_id" ]; then
        run_id=$(just runid)
    fi
    temp_dir=$(mktemp -d)
    gh run download $run_id -n wheel-win_amd64 -D $temp_dir
    just checkartifactversion $temp_dir
    just wheel-check $temp_dir/datoviz*.whl
    exit_code=$?
    rm -rf "${temp_dir}"
    exit $exit_code
#

[linux]
[macos]
symbols: api-json
    @jq -r '.functions[].name' {{justfile_directory()}}/build/bindings/datoviz_api.json > {{justfile_directory()}}/symbols.map
#

[windows]
symbols: api-json
    @jq -r ".functions[].name" "{{justfile_directory()}}\\build\\bindings\\datoviz_api.json" > "{{justfile_directory()}}\\symbols.map"
#


[linux]
exports:
    @nm -D --defined-only build/libdatoviz.so
#

[macos]
exports:
    @nm -gU build/libdatoviz.dylib
#

[macos]
deps:
    @otool -L build/libdatoviz.dylib | sort -r
#

[linux]
deps:
    @ldd build/libdatoviz.so
#

[linux]
strip:
    @strip --strip-debug build/libdatoviz.so
#

[macos]
rpath:
    @echo "Printing RPATH:"
    @otool -l build/libdatoviz.dylib | grep -i "path"
#

[linux]
rpath:
    @echo "Printing RPATH:"
    @objdump -x build/libdatoviz.so | grep -i 'R.*PATH'
#

tryimport:
    @python -c "import datoviz"
#

api:
    @echo "Legacy API mega-target is quarantined. Use just bindings-preflight and just docs-api-check." >&2
    @exit 2
#
