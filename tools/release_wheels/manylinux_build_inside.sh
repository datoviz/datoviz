#!/usr/bin/env bash
set -euo pipefail

arch="${1:-x86_64}"
case "$arch" in
    x86_64)
        platform_tag="manylinux_2_34_x86_64"
        ;;
    aarch64)
        platform_tag="manylinux_2_34_aarch64"
        ;;
    *)
        echo "unsupported manylinux arch: $arch" >&2
        exit 2
        ;;
esac

if [ "$(id -u)" -eq 0 ]; then
    dnf install -y \
        ccache \
        clang \
        freetype-devel \
        glslang \
        glslang-devel \
        libshaderc \
        libshaderc-devel \
        libX11-devel \
        libXcursor-devel \
        libXi-devel \
        libXinerama-devel \
        libXrandr-devel \
        mesa-libGL-devel \
        ninja-build \
        vulkan-loader-devel \
        zlib-devel
fi

python_bin="${DATOVIZ_MANYLINUX_PYTHON:-/opt/python/cp313-cp313/bin/python}"
if [ ! -x "$python_bin" ]; then
    echo "Python interpreter not found: $python_bin" >&2
    exit 2
fi

host_uid="${DATOVIZ_HOST_UID:-}"
host_gid="${DATOVIZ_HOST_GID:-}"
build_dir="${DATOVIZ_MANYLINUX_BUILD_DIR:-build/manylinux-${arch}}"
stage_dir="${build_dir}/wheel-stage"

cleanup_owner()
{
    if [ -n "$host_uid" ] && [ -n "$host_gid" ] && [ "$(id -u)" -eq 0 ]; then
        chown -R "$host_uid:$host_gid" \
            "$build_dir" \
            wheelhouse \
            datoviz/_ctypes.py \
            datoviz/_array_facade.py \
            build/api.json \
            build/bindings/datoviz_api.json \
            2>/dev/null || true
    fi
}
trap cleanup_owner EXIT

export PATH="$(dirname "$python_bin"):$PATH"
export DVZ_CMAKE_ARGS="${DVZ_CMAKE_ARGS:--DDVZ_ENABLE_SHADERC=ON -DDVZ_BUILD_TESTING=OFF -DDVZ_BUILD_EXAMPLES=OFF}"
export DVZ_WHEEL_RUNTIME_DIRS="${DVZ_WHEEL_RUNTIME_DIRS:-/usr/lib64:/usr/lib}"

"$python_bin" -m pip install -U pip auditwheel build libclang tqdm wheel

git config --global --add safe.directory /workspace || true
git config --global --add safe.directory /workspace/external/kvazaar || true

mkdir -p "$build_dir" docs/images wheelhouse
cmake_args="${DVZ_CMAKE_ARGS:-}"
(
    cd "$build_dir"
    CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake ../.. \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        ${cmake_args}
    ninja
)

if [ "${DATOVIZ_MANYLINUX_GENERATE_CTYPES:-1}" = "1" ]; then
    "$python_bin" tools/bindings/extract_api.py --clang clang
    "$python_bin" tools/bindings/generate_ctypes.py
    "$python_bin" tools/bindings/generate_array_facade.py
else
    test -f datoviz/_ctypes.py
    test -f datoviz/_array_facade.py
fi

rm -rf wheelhouse
mkdir -p wheelhouse
"$python_bin" tools/release_wheels/stage_wheel.py \
    --clean \
    --build-dir "$build_dir" \
    --stage-dir "$stage_dir"
"$python_bin" tools/release_wheels/build_wheel.py \
    --stage-dir "$stage_dir" \
    --dist-dir wheelhouse \
    --platform-tag "$platform_tag"

for whl in wheelhouse/datoviz-*.whl; do
    "$python_bin" tools/release_wheels/inspect_wheel.py --wheel "$whl" --native-deps
    "$python_bin" tools/release_wheels/check_wheel.py \
        --wheel "$whl" \
        --shaderc \
        --cmake-consumer \
        --qt-probe optional
done
