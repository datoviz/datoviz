#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WORKDIR=${TMPDIR:-/tmp}/datoviz-c-integration-smoke
PREFIX="$WORKDIR/prefix"
PACKAGE_BUILD="$WORKDIR/cmake-package-build"
FETCH_BUILD="$WORKDIR/fetchcontent-build"
FETCH_GLFW_PLAIN_BUILD="$WORKDIR/fetchcontent-glfw-plain-build"
FETCH_GLFW_NAMESPACED_BUILD="$WORKDIR/fetchcontent-glfw-namespaced-build"
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

for GLFW_TARGET in PLAIN NAMESPACED; do
    if [ "$GLFW_TARGET" = PLAIN ]; then
        GLFW_BUILD="$FETCH_GLFW_PLAIN_BUILD"
    else
        GLFW_BUILD="$FETCH_GLFW_NAMESPACED_BUILD"
    fi

    echo "Building FetchContent consumer with parent-owned $GLFW_TARGET GLFW target..."
    cmake -S "$ROOT/examples/c/integration/fetchcontent" -B "$GLFW_BUILD" -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DDATOVIZ_FETCHCONTENT_SOURCE_DIR="$ROOT" \
        -DDATOVIZ_FETCHCONTENT_PRELOAD_GLFW="$GLFW_TARGET" \
        -DDVZ_BUILD_GUI=OFF \
        -DDVZ_ENABLE_QT_BRIDGE=OFF \
        -DDVZ_ENABLE_CUDA=OFF >/dev/null
    cmake --build "$GLFW_BUILD" --target datoviz_fetchcontent_example >/dev/null

    LD_LIBRARY_PATH="$GLFW_BUILD/_deps/datoviz-build/src:${LD_LIBRARY_PATH:-}" \
    DYLD_LIBRARY_PATH="$GLFW_BUILD/_deps/datoviz-build/src:${DYLD_LIBRARY_PATH:-}" \
        "$GLFW_BUILD/datoviz_fetchcontent_example"
done

echo "C integration smoke passed: $WORKDIR"
