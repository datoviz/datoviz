test test_name="":
    @python3 tasks/test_driver.py run "{{test_name}}"
#

[linux]
atest test_name="": asan
    #!/usr/bin/env bash
    set -euo pipefail
    build_dir="$(pwd)/build-asan"
    # NOTE: we create build-asan/lsan.supp by copying parts of sanitizers/asan.ignore into it
    suppressions_file="${build_dir}/lsan.supp"
    mkdir -p "${build_dir}"
    grep '^leak:' sanitizers/asan.ignore > "${suppressions_file}" || true
    ASAN_OPTIONS="halt_on_error=1:detect_stack_use_after_return=1:strict_init_order=1:alloc_dealloc_mismatch=1:detect_invalid_pointer_pairs=1:malloc_context_size=20:verbosity=1" LSAN_OPTIONS="suppressions=${suppressions_file}${LSAN_OPTIONS:+:${LSAN_OPTIONS}}" ./build-asan/testing/dvztest {{test_name}}
#

[linux]
coverage filter="":
    @set -e
    @if ! command -v gcovr >/dev/null 2>&1; then \
        echo "gcovr is required for coverage reporting. Install it with 'pip install gcovr'."; \
        exit 1; \
    fi
    @rm -rf build-coverage
    @mkdir -p docs/images
    @mkdir -p build-coverage/coverage
    @ROOT=$(pwd) && cd build-coverage && cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DDVZ_ENABLE_COVERAGE=ON -DDVZ_ENABLE_ASAN_IN_DEBUG=OFF -DDVZ_ENABLE_CUDA=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
    @cd build-coverage && ninja
    @cd build-coverage && if [ -n "{{filter}}" ]; then ./testing/dvztest "{{filter}}"; else ./testing/dvztest; fi
    @ROOT=$(pwd) && cd build-coverage && gcovr --root "$ROOT" --filter "$ROOT/src" --filter "$ROOT/include" --exclude "$ROOT/external" --exclude "$ROOT/testing" --exclude "$ROOT/v0.3" --exclude "$ROOT/build" --txt-metric branch --print-summary
    @ROOT=$(pwd) && cd build-coverage && gcovr --root "$ROOT" --filter "$ROOT/src" --filter "$ROOT/include" --exclude "$ROOT/external" --exclude "$ROOT/testing" --exclude "$ROOT/v0.3" --exclude "$ROOT/build" --html --html-details -o coverage/index.html
    @echo "Coverage HTML report: build-coverage/coverage/index.html"
#

[linux]
mtest test_name="": msan
    #!/usr/bin/env bash
    set -euo pipefail
    MSAN_OPTIONS="halt_on_error=0:exit_code=0:print_stats=0:symbolize=1:abort_on_error=0" \
    ./build-msan/testing/dvztest {{test_name}} 2> >(awk -f tools/hide-msan.awk >&2)
#

[linux]
ttest test_name="": tsan
    #!/usr/bin/env bash
    set -euo pipefail
    TSAN_OPTIONS="ignore_noninstrumented_modules=1:verbosity=0" ./build-tsan/testing/dvztest {{test_name}}
#

test-inventory lane="":
    @python3 tasks/test_driver.py inventory "{{lane}}"
#

test-lane lane *args:
    @python3 tasks/test_driver.py lane "{{lane}}" -- {{args}}
#

test-fast *args:
    just test-lane fast-cpu {{args}}
#

test-scene-cpu *args:
    just test-lane scene-semantic {{args}}
#

test-drp2-contract *args:
    just test-lane drp2-contract {{args}}
#

test-runtime-vklite *args:
    just test-lane runtime-vklite {{args}}
#

test-render-smoke *args:
    just test-lane render-smoke {{args}}
#

test-render-conformance *args:
    just test-lane render-conformance {{args}}
#

test-slow *args:
    just test-lane slow-churn {{args}}
#

[linux]
[macos]
shaderc-smoke:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake --build build --target dvz_shaderc_smoke
    ./build/testing/dvz_shaderc_smoke
    shaderc_library="$(cmake -LA -N build | awk -F= '/^DVZ_SHADERC_LIBRARY:FILEPATH=/{print $2}')"
    if [[ -n "${shaderc_library}" && -f "${shaderc_library}" ]]; then
        cmake --build build --target dvz_shaderc_runtime_dirs_smoke
        shaderc_runtime_dir="$(dirname "${shaderc_library}")"
        DVZ_WHEEL_RUNTIME_DIRS="${shaderc_runtime_dir}" ./build/testing/dvz_shaderc_runtime_dirs_smoke
    fi
#

[linux]
canvas *args:
    ./build/testing/dvz_live_canvas {{args}}
#

[linux]
present-check *args:
    cmake --build build --target dvz_live_canvas scatter
    scripts/check_present_paths.sh {{args}}
#

[linux]
compare-present reference candidate="HEAD" *args:
    python3 tools/compare_present_benchmarks.py "{{reference}}" "{{candidate}}" {{args}}
#

[linux]
compare-interaction reference candidate="HEAD" *args:
    python3 tools/compare_present_benchmarks.py "{{reference}}" "{{candidate}}" --workload scatter-interaction {{args}}
#

[linux]
profile-canvas *args:
    just build-profile
    scripts/profile_live_canvas.sh --bin ./build-profile/testing/dvz_live_canvas {{args}}
#

[linux]
profile-canvas-release *args:
    just build-profile
    scripts/profile_live_canvas.sh --bin ./build-profile/testing/dvz_live_canvas {{args}}
#

[linux]
memory-canvas-release *args:
    just build-profile
    scripts/memory_live_canvas.sh --bin ./build-profile/testing/dvz_live_canvas {{args}}
#

[macos]
canvas *args:
    ./build/testing/dvz_live_canvas {{args}}
#

[windows]
canvas *args:
