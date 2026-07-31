#!/usr/bin/env bash
set -euxo pipefail

cmake -S "$SRC_DIR" -B build-conda-qtbridge -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DDVZ_BUILD_CORE=OFF \
    -DDVZ_BUILD_CONTROLLER=OFF \
    -DDVZ_BUILD_VK=OFF \
    -DDVZ_BUILD_CANVAS=OFF \
    -DDVZ_BUILD_DRP2=OFF \
    -DDVZ_BUILD_WEBGPU=OFF \
    -DDVZ_BUILD_SCENE=OFF \
    -DDVZ_BUILD_APP=OFF \
    -DDVZ_BUILD_GUI=OFF \
    -DDVZ_BUILD_TESTING=OFF \
    -DDVZ_BUILD_EXAMPLES=OFF \
    -DDVZ_INSTALL=OFF \
    -DDVZ_VENDORED_DEPS=OFF \
    -DDVZ_CGLM_SOURCE=SYSTEM \
    -DDVZ_MIMALLOC_SOURCE=OFF \
    -DDVZ_KVAZAAR_SOURCE=OFF \
    -DDVZ_ENABLE_KVAZAAR=OFF \
    -DDVZ_WITH_GLFW=OFF \
    -DDVZ_WITH_ZLIB=OFF \
    -DDVZ_WITH_FREETYPE=OFF \
    -DDVZ_WITH_MSDF_ATLAS=OFF \
    -DDVZ_WITH_MSDF_SVG=OFF \
    -DDVZ_ENABLE_CUDA=OFF \
    -DDVZ_ENABLE_SHADERC=OFF \
    -DDVZ_ENABLE_QT_BRIDGE=ON

cmake --build build-conda-qtbridge --target datoviz_qtbridge
install -Dm755 build-conda-qtbridge/qtbridge/libdatoviz_qtbridge.so "$SP_DIR/datoviz/libdatoviz_qtbridge.so"
