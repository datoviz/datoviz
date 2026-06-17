#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
VERSION=${DATOVIZ_VALIDATE_VERSION:-0.4.0}
MODE=${1:-all}
WORKDIR=${DATOVIZ_DIST_VALIDATE_WORKDIR:-${TMPDIR:-/tmp}/datoviz-dist-validate}
SOURCE_BUNDLE=${DATOVIZ_SOURCE_BUNDLE:-}
SOURCE_SHA512=${DATOVIZ_SOURCE_SHA512:-}
VCPKG_ROOT=${VCPKG_ROOT:-/tmp/datoviz-vcpkg}
VCPKG_TRIPLET=${VCPKG_TRIPLET:-x64-linux-dynamic}
CONDA_BLD_DIR=${DATOVIZ_CONDA_BLD_DIR:-/tmp/datoviz-mamba-root/envs/build/conda-bld}


log()
{
    printf '\n==> %s\n' "$*"
}


die()
{
    printf 'error: %s\n' "$*" >&2
    exit 2
}


have()
{
    command -v "$1" >/dev/null 2>&1
}


require()
{
    have "$1" || die "required command not found: $1"
}


cmake_generator_args()
{
    if have ninja; then
        printf '%s\n' -GNinja
    fi
}


sha512_file()
{
    python - "$1" <<'PY'
import hashlib
import sys

h = hashlib.sha512()
with open(sys.argv[1], "rb") as f:
    for chunk in iter(lambda: f.read(1024 * 1024), b""):
        h.update(chunk)
print(h.hexdigest())
PY
}


prepare_workdir()
{
    rm -rf "$WORKDIR"
    mkdir -p "$WORKDIR"
}


ensure_source_bundle()
{
    if [ -n "$SOURCE_BUNDLE" ]; then
        [ -f "$SOURCE_BUNDLE" ] || die "DATOVIZ_SOURCE_BUNDLE does not exist: $SOURCE_BUNDLE"
        if [ -z "$SOURCE_SHA512" ]; then
            SOURCE_SHA512=$(sha512_file "$SOURCE_BUNDLE")
        fi
        return
    fi

    require python
    log "Creating release source bundle"
    local output
    output=$(python "$ROOT/tools/release_source_bundle.py" "$VERSION" --output-dir "$WORKDIR/source-bundle")
    SOURCE_BUNDLE=$(printf '%s\n' "$output" | sed -n '1p')
    SOURCE_SHA512=$(printf '%s\n' "$output" | sed -n '2p' | awk '{print $2}')
    [ -f "$SOURCE_BUNDLE" ] || die "source bundle was not created"
    [ -n "$SOURCE_SHA512" ] || die "source bundle SHA512 was not printed"
}


extract_source_bundle()
{
    ensure_source_bundle
    local extract_dir="$WORKDIR/source"
    rm -rf "$extract_dir"
    mkdir -p "$extract_dir"
    tar -xzf "$SOURCE_BUNDLE" -C "$extract_dir"
    find "$extract_dir" -mindepth 1 -maxdepth 1 -type d | head -n 1
}


run_cmake_consumer()
{
    local prefix=$1
    local build_dir=$2
    shift 2

    log "Building CMake package consumer against $prefix"
    rm -rf "$build_dir"
    cmake -S "$ROOT/examples/c/integration/cmake_package" -B "$build_dir" \
        $(cmake_generator_args) \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$prefix" \
        "$@"
    cmake --build "$build_dir"
    LD_LIBRARY_PATH="$prefix/lib:$prefix/lib64:$prefix/debug/lib:${LD_LIBRARY_PATH:-}" \
        "$build_dir/datoviz_cmake_package_example"
}


run_pkg_config_consumer()
{
    local prefix=$1
    local build_dir=$2
    local pc_path="$prefix/lib/pkgconfig:$prefix/lib64/pkgconfig:$prefix/share/pkgconfig"

    require cc
    require pkg-config
    log "Building pkg-config consumer against $prefix"
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    PKG_CONFIG_PATH="$pc_path" pkg-config --exists datoviz
    cat > "$build_dir/main.c" <<'EOF'
#include <datoviz.h>

int main(void)
{
    return dvz_version() == 0;
}
EOF
    # Intentional shell expansion: pkg-config emits compiler/linker words.
    PKG_CONFIG_PATH="$pc_path" \
        cc "$build_dir/main.c" $(PKG_CONFIG_PATH="$pc_path" pkg-config --cflags --libs datoviz) \
        -Wl,-rpath,"$prefix/lib" -Wl,-rpath-link,"$prefix/lib" \
        -o "$build_dir/datoviz_pkg_config_consumer"
    LD_LIBRARY_PATH="$prefix/lib:$prefix/lib64:$prefix/debug/lib:${LD_LIBRARY_PATH:-}" \
        "$build_dir/datoviz_pkg_config_consumer"
}


validate_source_install()
{
    require cmake
    local source_dir prefix build_dir
    source_dir=$(extract_source_bundle)
    prefix="$WORKDIR/source-prefix"
    build_dir="$WORKDIR/source-build"

    log "Building and installing source bundle"
    cmake -S "$source_dir" -B "$build_dir" \
        $(cmake_generator_args) \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DDVZ_BUILD_TESTING=OFF \
        -DDVZ_BUILD_EXAMPLES=OFF \
        -DDVZ_INSTALL=ON \
        -DDVZ_VENDORED_DEPS=OFF \
        -DDVZ_CGLM_SOURCE=SYSTEM \
        -DDVZ_MIMALLOC_SOURCE=SYSTEM \
        -DDVZ_KVAZAAR_SOURCE=OFF \
        -DDVZ_ENABLE_CUDA=OFF \
        -DDVZ_ENABLE_QT_BRIDGE=OFF
    cmake --build "$build_dir" --target install

    run_cmake_consumer "$prefix" "$WORKDIR/source-cmake-consumer"
    run_pkg_config_consumer "$prefix" "$WORKDIR/source-pkg-config-consumer"
}


validate_vcpkg()
{
    require cmake
    [ -x "$VCPKG_ROOT/vcpkg" ] || die "vcpkg not found at $VCPKG_ROOT/vcpkg; set VCPKG_ROOT"
    ensure_source_bundle

    log "Building vcpkg overlay from source bundle"
    "$VCPKG_ROOT/vcpkg" remove "datoviz:$VCPKG_TRIPLET" --classic >/dev/null 2>&1 || true
    VCPKG_DISABLE_METRICS=1 \
    DATOVIZ_VCPKG_SOURCE_URL="file://$SOURCE_BUNDLE" \
    DATOVIZ_VCPKG_SOURCE_SHA512="$SOURCE_SHA512" \
        "$VCPKG_ROOT/vcpkg" install datoviz --classic \
            --overlay-ports="$ROOT/vcpkg-overlay/ports" \
            --triplet "$VCPKG_TRIPLET"

    local prefix="$VCPKG_ROOT/installed/$VCPKG_TRIPLET"
    run_cmake_consumer "$prefix" "$WORKDIR/vcpkg-cmake-consumer" \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET"
    run_pkg_config_consumer "$prefix" "$WORKDIR/vcpkg-pkg-config-consumer"
}


conda_cmd()
{
    if [ -n "${MICROMAMBA:-}" ]; then
        "$MICROMAMBA" "$@"
    elif [ -n "${CONDA_EXE:-}" ]; then
        "$CONDA_EXE" "$@"
    elif have micromamba; then
        micromamba "$@"
    elif have conda; then
        conda "$@"
    elif [ -x /tmp/datoviz-micromamba/bin/micromamba ]; then
        /tmp/datoviz-micromamba/bin/micromamba "$@"
    else
        return 127
    fi
}


have_conda_tool()
{
    conda_cmd --version >/dev/null 2>&1
}


validate_conda()
{
    have_conda_tool || die "conda or micromamba not found; set MICROMAMBA or CONDA_EXE"
    [ -d "$CONDA_BLD_DIR/linux-64" ] || die "conda build directory not found: $CONDA_BLD_DIR"

    local env_prefix="$WORKDIR/conda-env"
    log "Creating clean conda environment from local packages"
    rm -rf "$env_prefix"
    conda_cmd create -y -p "$env_prefix" --override-channels \
        -c "file://$CONDA_BLD_DIR" -c conda-forge datoviz libdatoviz

    log "Running conda Python import smoke"
    conda_cmd run -p "$env_prefix" python - <<'PY'
import datoviz
import datoviz.raw as raw

scene = raw.dvz_scene()
raw.dvz_scene_destroy(scene)
PY

    run_cmake_consumer "$env_prefix" "$WORKDIR/conda-cmake-consumer"
    run_pkg_config_consumer "$env_prefix" "$WORKDIR/conda-pkg-config-consumer"
}


usage()
{
    cat <<EOF
Usage: $0 [all|source-install|vcpkg|conda|pkg-config]

Environment:
  DATOVIZ_VALIDATE_VERSION       Release version for generated bundles [$VERSION]
  DATOVIZ_DIST_VALIDATE_WORKDIR  Temporary work directory [$WORKDIR]
  DATOVIZ_SOURCE_BUNDLE          Existing datoviz-<version>-source.tar.gz
  DATOVIZ_SOURCE_SHA512          SHA512 for DATOVIZ_SOURCE_BUNDLE
  VCPKG_ROOT                     vcpkg checkout [$VCPKG_ROOT]
  VCPKG_TRIPLET                  vcpkg target triplet [$VCPKG_TRIPLET]
  MICROMAMBA or CONDA_EXE        conda frontend for conda validation
  DATOVIZ_CONDA_BLD_DIR          conda-bld directory [$CONDA_BLD_DIR]
EOF
}


main()
{
    case "$MODE" in
        -h|--help|help)
            usage
            return 0
            ;;
        source-install)
            prepare_workdir
            validate_source_install
            ;;
        vcpkg)
            prepare_workdir
            validate_vcpkg
            ;;
        conda)
            prepare_workdir
            validate_conda
            ;;
        pkg-config)
            prepare_workdir
            validate_source_install
            ;;
        all)
            prepare_workdir
            validate_source_install
            if [ -x "$VCPKG_ROOT/vcpkg" ]; then
                validate_vcpkg
            else
                log "Skipping vcpkg lane; set VCPKG_ROOT to enable it"
            fi
            if have_conda_tool && [ -d "$CONDA_BLD_DIR/linux-64" ]; then
                validate_conda
            else
                log "Skipping conda lane; set MICROMAMBA/CONDA_EXE and DATOVIZ_CONDA_BLD_DIR to enable it"
            fi
            ;;
        *)
            usage >&2
            die "unknown mode: $MODE"
            ;;
    esac

    log "Distribution validation passed: $MODE"
    printf 'workdir: %s\n' "$WORKDIR"
}


main
