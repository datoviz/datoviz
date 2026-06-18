drp2-fixtures *args='':
    @python3 tools/drp2_fixture_runner.py {{args}}
#

drp2-fixture-check *args='':
    @just drp2-fixtures {{args}}
#

webgpu-fixture-preflight *args='':
    @python3 tools/webgpu_fixture_preflight.py {{args}}
#

webgpu-fixture-check *args='':
    @just webgpu-fixture-preflight {{args}}
#

webgpu-runner-smoke:
    @node tools/webgpu_runner_smoke.mjs
#

webgpu-browser-smoke: wasm-scene-build
    @node tools/webgpu_browser_smoke.mjs
#

webgpu-gallery-check: wasm-scene-smoke webgpu-browser-smoke
#

webgpu-check: webgpu-gallery-check
#

wasm-env-check:
    #!/usr/bin/env bash
    set -euo pipefail
    export EMSDK_QUIET=1
    if ! command -v emcmake >/dev/null 2>&1; then
        if [ -n "${EMSDK_ENV_SH:-}" ] && [ -f "$EMSDK_ENV_SH" ]; then
            source "$EMSDK_ENV_SH"
        elif [ -n "${EMSDK:-}" ] && [ -f "$EMSDK/emsdk_env.sh" ]; then
            source "$EMSDK/emsdk_env.sh"
        elif [ -f "$PWD/../emsdk/emsdk_env.sh" ]; then
            source "$PWD/../emsdk/emsdk_env.sh"
        elif [ -f "$HOME/SDK/emsdk/emsdk_env.sh" ]; then
            source "$HOME/SDK/emsdk/emsdk_env.sh"
        elif [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
            source "$HOME/emsdk/emsdk_env.sh"
        else
            echo "Emscripten not found."
            echo "Install/activate emsdk, put emcmake on PATH, or set EMSDK_ENV_SH=/path/to/emsdk_env.sh."
            exit 1
        fi
    fi
    echo "emcc: $(command -v emcc)"
    echo "emcmake: $(command -v emcmake)"
    emcc --version | head -n 1
#

wasm-doctor: wasm-env-check
#

_wasm-scene-build-mode build_dir build_type flag_config c_flags='' cxx_flags='' linker_flags='':
    #!/usr/bin/env bash
    set -euo pipefail
    export EMSDK_QUIET=1
    if ! command -v emcmake >/dev/null 2>&1; then
        if [ -n "${EMSDK_ENV_SH:-}" ] && [ -f "$EMSDK_ENV_SH" ]; then
            source "$EMSDK_ENV_SH"
        elif [ -n "${EMSDK:-}" ] && [ -f "$EMSDK/emsdk_env.sh" ]; then
            source "$EMSDK/emsdk_env.sh"
        elif [ -f "$PWD/../emsdk/emsdk_env.sh" ]; then
            source "$PWD/../emsdk/emsdk_env.sh"
        elif [ -f "$HOME/SDK/emsdk/emsdk_env.sh" ]; then
            source "$HOME/SDK/emsdk/emsdk_env.sh"
        elif [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
            source "$HOME/emsdk/emsdk_env.sh"
        else
            echo "Emscripten not found."
            echo "Install/activate emsdk, put emcmake on PATH, or set EMSDK_ENV_SH=/path/to/emsdk_env.sh."
            exit 1
        fi
    fi
    cmake_args=(
        -S .
        -B "{{build_dir}}"
        -DCMAKE_BUILD_TYPE="{{build_type}}"
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
    if [ -n "{{c_flags}}" ]; then
        cmake_args+=("-DCMAKE_C_FLAGS_{{flag_config}}={{c_flags}}")
    fi
    if [ -n "{{cxx_flags}}" ]; then
        cmake_args+=("-DCMAKE_CXX_FLAGS_{{flag_config}}={{cxx_flags}}")
    fi
    if [ -n "{{linker_flags}}" ]; then
        cmake_args+=("-DCMAKE_EXE_LINKER_FLAGS_{{flag_config}}={{linker_flags}}")
    fi
    emcmake cmake "${cmake_args[@]}"
    cmake --build "{{build_dir}}" --target datoviz_wasm_scene -j 8
#

wasm-scene-build:
    @just _wasm-scene-build-mode build-wasm-scene Release RELEASE
#

wasm-scene-build-debug:
    @just _wasm-scene-build-mode build-wasm-scene-debug Debug DEBUG \
        "-O0 -g3 -fno-omit-frame-pointer" \
        "-O0 -g3 -fno-omit-frame-pointer" \
        "-sASSERTIONS=2 -sSTACK_OVERFLOW_CHECK=2 -sSTACK_SIZE=1048576 -gsource-map --source-map-base=/"
#

wasm-scene-build-asan:
    @just _wasm-scene-build-mode build-wasm-scene-asan Debug DEBUG \
        "-O1 -g -fsanitize=address -fno-omit-frame-pointer" \
        "-O1 -g -fsanitize=address -fno-omit-frame-pointer" \
        "-fsanitize=address -sASSERTIONS=2 -sSTACK_OVERFLOW_CHECK=2 -sSTACK_SIZE=1048576 -gsource-map --source-map-base=/"
#

wasm-scene-build-safeheap:
    @just _wasm-scene-build-mode build-wasm-scene-safeheap Debug DEBUG \
        "-O1 -g -fno-omit-frame-pointer" \
        "-O1 -g -fno-omit-frame-pointer" \
        "-sASSERTIONS=2 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=2 -sSTACK_SIZE=1048576 -gsource-map --source-map-base=/"
#

wasm-scene-stack-usage:
    @just _wasm-scene-build-mode build-wasm-scene-stackusage Release RELEASE \
        "-O3 -fstack-usage -Wframe-larger-than=32768" \
        "-O3 -fstack-usage -Wframe-larger-than=32768"
    @find build-wasm-scene-stackusage -name '*.su' -print0 | \
        xargs -0 awk '{print $2 "\t" $1}' | sort -nr | head -n 30
#

wasm-scene-smoke: wasm-scene-build
    @node tools/wasm_scene_smoke.mjs
    @python3 tools/webgpu_fixture_preflight.py build-wasm-scene/wasm/wasm_api_scene_point_pixel_marker_segment_path_primitive_image_mesh_panzoom.json
    @python3 tools/webgpu_fixture_preflight.py build-wasm-scene/wasm/wasm_api_scene_sphere_textured_mesh3d_arcball.json
    @python3 tools/webgpu_fixture_preflight.py build-wasm-scene/wasm/wasm_api_scenario_timer_animation.json
    @node tools/webgpu_runner_smoke.mjs --streams-only \
        build-wasm-scene/wasm/wasm_api_scene_point_pixel_marker_segment_path_primitive_image_mesh_panzoom.json \
        build-wasm-scene/wasm/wasm_api_scene_sphere_textured_mesh3d_arcball.json \
        build-wasm-scene/wasm/wasm_api_scenario_timer_animation.json
#

shader-abi-check:
    @python3 tools/check_scene_shader_abi.py
#

spec-check: shader-abi-check
    #!/usr/bin/env bash
    set -euo pipefail

    tmpdir="$(mktemp -d)"
    trap 'rm -rf "$tmpdir"' EXIT

    names=()
    pids=()
    logs=()

    run_check() {
        local name="$1"
        shift
        local log="$tmpdir/${#names[@]}.log"
        names+=("$name")
        logs+=("$log")
        "$@" >"$log" 2>&1 &
        pids+=("$!")
    }

    run_check "drp2 fixture runner" .venv/bin/python tools/drp2_fixture_runner.py
    run_check "webgpu fixture preflight" .venv/bin/python tools/webgpu_fixture_preflight.py
    run_check "webgpu runner smoke" node tools/webgpu_runner_smoke.mjs
    run_check "drp2 fixture tests" .venv/bin/pytest -q testing/test_drp2_fixture_runner.py
    run_check "webgpu preflight tests" .venv/bin/pytest -q testing/test_webgpu_fixture_preflight.py
    run_check "scheduler tests" .venv/bin/pytest -q testing/test_dvztest_scheduler.py
    run_check "scene query source guard" .venv/bin/pytest -q testing/test_scene_query_source_guard.py
    run_check "scene architecture source guard" .venv/bin/pytest -q testing/test_scene_architecture_source_guard.py
    run_check "scene visual boundary guard" .venv/bin/python tools/check_scene_visual_boundaries.py

    status=0
    for i in "${!pids[@]}"; do
        if wait "${pids[$i]}"; then
            printf 'PASS %s\n' "${names[$i]}"
        else
            printf 'FAIL %s\n' "${names[$i]}"
            status=1
        fi
        cat "${logs[$i]}"
    done

    exit "$status"
#

