#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WORKDIR=${TMPDIR:-/tmp}/datoviz-c-integration-smoke
PREFIX="$WORKDIR/prefix"
PACKAGE_BUILD="$WORKDIR/cmake-package-build"
FETCH_BUILD="$WORKDIR/fetchcontent-build"
FETCH_PREEXISTING_GLFW_BUILD="$WORKDIR/fetchcontent-preexisting-glfw-build"
CANVAS_NO_GLSLC_BUILD="$WORKDIR/canvas-no-glslc-build"
DATOVIZ_BUILD="$WORKDIR/datoviz-build"

rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"

echo "Configuring Datoviz install smoke..."
cmake -S "$ROOT" -B "$DATOVIZ_BUILD" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DDVZ_BUILD_GUI=OFF \
    -DDVZ_BUILD_TESTING=OFF \
    -DDVZ_BUILD_EXAMPLES=OFF \
    -DDVZ_ENABLE_QT_BRIDGE=OFF \
    -DDVZ_ENABLE_CUDA=OFF >/dev/null
cmake --build "$DATOVIZ_BUILD" --target install >/dev/null

echo "Building installed-package CMake consumer..."
cmake -S "$ROOT/examples/c/integration/cmake_package" -B "$PACKAGE_BUILD" -GNinja \
    -DCMAKE_PREFIX_PATH="$PREFIX" >/dev/null
cmake --build "$PACKAGE_BUILD" >/dev/null

LD_LIBRARY_PATH="$PREFIX/lib:$PREFIX/lib64:${LD_LIBRARY_PATH:-}" \
DYLD_LIBRARY_PATH="$PREFIX/lib:$PREFIX/lib64:${DYLD_LIBRARY_PATH:-}" \
PATH="$PREFIX/bin:$PATH" \
    "$PACKAGE_BUILD/datoviz_cmake_package_example"

echo "Building FetchContent CMake consumer..."
cmake -S "$ROOT/examples/c/integration/fetchcontent" -B "$FETCH_BUILD" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DDATOVIZ_FETCHCONTENT_SOURCE_DIR="$ROOT" \
    -DDVZ_BUILD_GUI=OFF \
    -DDVZ_ENABLE_QT_BRIDGE=OFF \
    -DDVZ_ENABLE_CUDA=OFF >/dev/null
cmake --build "$FETCH_BUILD" --target datoviz_fetchcontent_example >/dev/null

LD_LIBRARY_PATH="$FETCH_BUILD/_deps/datoviz-build/src:${LD_LIBRARY_PATH:-}" \
DYLD_LIBRARY_PATH="$FETCH_BUILD/_deps/datoviz-build/src:${DYLD_LIBRARY_PATH:-}" \
    "$FETCH_BUILD/datoviz_fetchcontent_example"

echo "Building FetchContent CMake consumer with parent-owned GLFW..."
cmake -S "$ROOT/examples/c/integration/fetchcontent" -B "$FETCH_PREEXISTING_GLFW_BUILD" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DDATOVIZ_FETCHCONTENT_SOURCE_DIR="$ROOT" \
    -DDATOVIZ_FETCHCONTENT_PRELOAD_GLFW=ON \
    -DDATOVIZ_FETCHCONTENT_PRELOAD_GLSLC_VARIABLES=ON \
    -DDVZ_BUILD_GUI=OFF \
    -DDVZ_ENABLE_QT_BRIDGE=OFF \
    -DDVZ_ENABLE_CUDA=OFF >/dev/null
cmake --build "$FETCH_PREEXISTING_GLFW_BUILD" \
    --target datoviz_fetchcontent_example >/dev/null

LD_LIBRARY_PATH="$FETCH_PREEXISTING_GLFW_BUILD/_deps/datoviz-build/src:${LD_LIBRARY_PATH:-}" \
DYLD_LIBRARY_PATH="$FETCH_PREEXISTING_GLFW_BUILD/_deps/datoviz-build/src:${DYLD_LIBRARY_PATH:-}" \
    "$FETCH_PREEXISTING_GLFW_BUILD/datoviz_fetchcontent_example"

echo "Building Canvas without glslc..."
(
    unset VULKAN_SDK DVZ_GLSLC
    cmake -S "$ROOT" -B "$CANVAS_NO_GLSLC_BUILD" -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DDVZ_BUILD_CONTROLLER=OFF \
        -DDVZ_BUILD_CANVAS=ON \
        -DDVZ_BUILD_DRP2=OFF \
        -DDVZ_BUILD_SCENE=OFF \
        -DDVZ_BUILD_APP=OFF \
        -DDVZ_BUILD_GUI=OFF \
        -DDVZ_BUILD_TESTING=OFF \
        -DDVZ_BUILD_EXAMPLES=OFF \
        -DDVZ_INSTALL=OFF \
        -DDVZ_WITH_FREETYPE=OFF \
        -DDVZ_WITH_MSDF_ATLAS=OFF \
        -DDVZ_WITH_MSDF_SVG=OFF \
        -DDVZ_ENABLE_SHADERC=OFF \
        -DDVZ_GLSLC_AUTO_DISCOVERY=OFF \
        -DDVZ_ENABLE_QT_BRIDGE=OFF \
        -DDVZ_ENABLE_CUDA=OFF >/dev/null
)
if grep -Eq '^DVZ_GLSLC_EXECUTABLE:FILEPATH=.+glslc' "$CANVAS_NO_GLSLC_BUILD/CMakeCache.txt"; then
    echo "Canvas no-glslc smoke unexpectedly discovered glslc" >&2
    exit 1
fi
cmake --build "$CANVAS_NO_GLSLC_BUILD" --target datoviz_canvas_layer >/dev/null

echo "C integration smoke passed: $WORKDIR"
