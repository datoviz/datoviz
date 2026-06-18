[linux]
test test_name="":
    #!/usr/bin/env bash
    set -euo pipefail
    ./build/testing/dvztest {{test_name}}
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

[macos]
test test_name="":
    ./build/testing/dvztest {{test_name}}
#

[macos]
test-inventory lane="":
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ -n "{{lane}}" ]]; then
        python3 tools/test_inventory.py \
            --lane "{{lane}}" \
            --output "build/testing/test_inventory_{{lane}}.json" \
            --markdown "build/testing/test_inventory_{{lane}}.md"
    else
        python3 tools/test_inventory.py \
            --output build/testing/test_inventory.json \
            --markdown build/testing/test_inventory.md
    fi
#

[macos]
test-lane lane *args:
    #!/usr/bin/env bash
    set -euo pipefail
    case_list="build/testing/test_lane_{{lane}}.txt"
    python3 tools/test_inventory.py \
        --lane "{{lane}}" \
        --output "build/testing/test_inventory_{{lane}}.json" \
        --markdown "build/testing/test_inventory_{{lane}}.md" \
        --case-list "${case_list}"
    ./build/testing/dvztest --case-list "${case_list}" {{args}}
#

[macos]
test-fast *args:
    just test-lane fast-cpu {{args}}
#

[macos]
test-scene-cpu *args:
    just test-lane scene-semantic {{args}}
#

[macos]
test-drp2-contract *args:
    just test-lane drp2-contract {{args}}
#

[macos]
test-runtime-vklite *args:
    just test-lane runtime-vklite {{args}}
#

[macos]
test-render-smoke *args:
    just test-lane render-smoke {{args}}
#

[macos]
test-render-conformance *args:
    just test-lane render-conformance {{args}}
#

[macos]
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
#

[windows]
test test_name="":
    ./build/testing/dvztest.exe {{test_name}}
#

[linux]
canvas *args:
    ./build/testing/dvz_live_canvas {{args}}
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
