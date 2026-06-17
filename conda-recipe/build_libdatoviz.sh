#!/usr/bin/env bash
set -euxo pipefail

cmake -S "$SRC_DIR" -B build-conda -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
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

cmake --build build-conda --target install
