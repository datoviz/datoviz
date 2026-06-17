#!/usr/bin/env sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 <wheel-datoviz-dir>" >&2
    exit 2
fi

dst="$1"

if [ ! -d "$dst" ]; then
    echo "wheel package directory not found: $dst" >&2
    exit 2
fi

rm -rf "$dst/include"
mkdir -p "$dst/include"
cp -a include/. "$dst/include/"
cp external/volk/volk.h "$dst/include/"
cp external/vk_mem_alloc.h "$dst/include/"
cp external/cimgui/cimgui.h "$dst/include/"
cp -a external/vulkan "$dst/include/"
cp -a external/vk_video "$dst/include/"

mkdir -p "$dst/lib/cmake/datoviz"
cp cmake/DatovizConfig.cmake.wheel "$dst/lib/cmake/datoviz/DatovizConfig.cmake"
cp cmake/DatovizConfig.cmake.wheel "$dst/lib/cmake/datoviz/datovizConfig.cmake"
