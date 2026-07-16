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

webgpu-data-check:
    @node tools/webgpu_data_loader_smoke.mjs
    @python3 tools/build_webgpu_data_bundles.py --check
#

webgpu-browser-smoke: wasm-scene-build
    @node tools/webgpu_browser_smoke.mjs
#

webgpu-gallery-check: wasm-scene-smoke webgpu-browser-smoke
#

webgpu-check: webgpu-gallery-check
#

wasm-env-check:
    @bash tasks/wasm_build.sh env-check
#

wasm-doctor: wasm-env-check
#

_wasm-scene-build-mode build_dir build_type flag_config c_flags='' cxx_flags='' linker_flags='':
    @bash tasks/wasm_build.sh build "{{build_dir}}" "{{build_type}}" "{{flag_config}}" "{{c_flags}}" "{{cxx_flags}}" "{{linker_flags}}"
#

wasm-scene-build:
    @just _wasm-scene-build-mode build-wasm-scene Release RELEASE
#

wasm-scene-build-if-needed:
    @python3 tools/ensure_wasm_scene_build.py
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

wasm-scene-smoke: wasm-scene-build webgpu-data-check
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

scene-visual-boundary-check:
    @python3 tools/check_scene_visual_boundaries.py
#

spec-check: shader-abi-check scene-visual-boundary-check
    @python3 tasks/spec_check.py
#
