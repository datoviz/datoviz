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

export PATH="$(dirname "$python_bin"):$PATH"
export DVZ_CMAKE_ARGS="${DVZ_CMAKE_ARGS:--DDVZ_ENABLE_SHADERC=ON}"
export DVZ_WHEEL_RUNTIME_DIRS="${DVZ_WHEEL_RUNTIME_DIRS:-/usr/lib64:/usr/lib}"

"$python_bin" -m pip install -U pip build auditwheel wheel

mkdir -p build docs/images wheelhouse
cmake_args="${DVZ_CMAKE_ARGS:-}"
(
    cd build
    CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        ${cmake_args}
    ninja
)

"$python_bin" tools/bindings/extract_api.py
"$python_bin" tools/bindings/generate_ctypes.py
"$python_bin" tools/bindings/generate_array_facade.py

rm -rf wheelhouse
mkdir -p wheelhouse
"$python_bin" -m pip wheel . --no-deps -w wheelhouse \
    -C datoviz.release-wheel=true \
    -C "datoviz.platform-tag=${platform_tag}"

for whl in wheelhouse/datoviz-*.whl; do
    "$python_bin" tools/release_wheels/inspect_wheel.py --wheel "$whl" --native-deps
    "$python_bin" tools/release_wheels/check_wheel.py \
        --wheel "$whl" \
        --shaderc \
        --cmake-consumer \
        --qt-probe optional
done
