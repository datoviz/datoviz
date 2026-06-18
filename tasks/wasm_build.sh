#!/usr/bin/env bash
set -euo pipefail

activate_emsdk()
{
    export EMSDK_QUIET=1
    if command -v emcmake >/dev/null 2>&1; then
        return
    fi
    if [[ -n "${EMSDK_ENV_SH:-}" && -f "$EMSDK_ENV_SH" ]]; then
        # shellcheck disable=SC1090
        source "$EMSDK_ENV_SH"
    elif [[ -n "${EMSDK:-}" && -f "$EMSDK/emsdk_env.sh" ]]; then
        # shellcheck disable=SC1090
        source "$EMSDK/emsdk_env.sh"
    elif [[ -f "$PWD/../emsdk/emsdk_env.sh" ]]; then
        # shellcheck disable=SC1091
        source "$PWD/../emsdk/emsdk_env.sh"
    elif [[ -f "$HOME/SDK/emsdk/emsdk_env.sh" ]]; then
        # shellcheck disable=SC1091
        source "$HOME/SDK/emsdk/emsdk_env.sh"
    elif [[ -f "$HOME/emsdk/emsdk_env.sh" ]]; then
        # shellcheck disable=SC1091
        source "$HOME/emsdk/emsdk_env.sh"
    else
        echo "Emscripten not found." >&2
        echo "Install/activate emsdk, put emcmake on PATH, or set EMSDK_ENV_SH=/path/to/emsdk_env.sh." >&2
        exit 1
    fi
}

env_check()
{
    activate_emsdk
    echo "emcc: $(command -v emcc)"
    echo "emcmake: $(command -v emcmake)"
    emcc --version | head -n 1
}

build_mode()
{
    local build_dir=$1
    local build_type=$2
    local flag_config=$3
    local c_flags=${4:-}
    local cxx_flags=${5:-}
    local linker_flags=${6:-}

    activate_emsdk
    cmake_args=(
        -S .
        -B "$build_dir"
        -DCMAKE_BUILD_TYPE="$build_type"
        -DCMAKE_SKIP_INSTALL_RULES=ON
        -DDVZ_BUILD_VK=OFF
        -DDVZ_BUILD_CANVAS=OFF
        -DDVZ_BUILD_APP=OFF
        -DDVZ_BUILD_GUI=OFF
        -DDVZ_WITH_FREETYPE=OFF
        -DDVZ_WITH_MSDF_ATLAS=OFF
        -DDVZ_ENABLE_KVAZAAR=OFF
        -DDVZ_WITH_GLFW=OFF
        -DDVZ_WITH_ZLIB=OFF
        -DDVZ_USE_MIMALLOC_RELEASE_DEFAULT=OFF
    )
    if [[ -n "$c_flags" ]]; then
        cmake_args+=("-DCMAKE_C_FLAGS_${flag_config}=${c_flags}")
    fi
    if [[ -n "$cxx_flags" ]]; then
        cmake_args+=("-DCMAKE_CXX_FLAGS_${flag_config}=${cxx_flags}")
    fi
    if [[ -n "$linker_flags" ]]; then
        cmake_args+=("-DCMAKE_EXE_LINKER_FLAGS_${flag_config}=${linker_flags}")
    fi
    emcmake cmake "${cmake_args[@]}"
    cmake --build "$build_dir" --target datoviz_wasm_scene -j 8
}

case "${1:-}" in
    env-check)
        env_check
        ;;
    build)
        shift
        build_mode "$@"
        ;;
    *)
        echo "usage: tasks/wasm_build.sh env-check | build <dir> <type> <flag-config> [c-flags] [cxx-flags] [linker-flags]" >&2
        exit 2
        ;;
esac
