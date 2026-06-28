checkstructs:
    #!/usr/bin/env python
    import ctypes
    import json

    # Insure local datoviz module is imported.
    import sys
    sys.path.insert(0, '.')

    def _check_struct_sizes(json_path):
        """Check the size of the ctypes structs and unions with respect to the sizes output by
        the CMake process (small executable in tools/struct_sizes.c compiled and executed by CMake).
        """
        with open(json_path, "r") as f:
            sizes = json.load(f)
        import datoviz.raw as dvz
        for name, size_c in sizes.items():
            obj = getattr(dvz, name)
            assert obj
            size_ctypes = ctypes.sizeof(obj)
            assert size_ctypes > 0
            if size_c != size_ctypes:
                raise ValueError(
                    f"Mismatch struct/union size error with {name}, "
                    f"C struct/union size is {size_c} whereas the ctypes size is {size_ctypes}")
        print(f"Sizes of {len(sizes)} structs/unions successfully checked.")

    _check_struct_sizes('build/struct_sizes.json')
#

clang:
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    just build
#

[linux]
[macos]
_build-configure release mode:
    #!/usr/bin/env bash
    set -euo pipefail
    unset CC
    unset CXX
    build_dir="build"
    stamp="${build_dir}/.dvz-build-config"
    mkdir -p docs/images "${build_dir}"
    launcher="${DVZ_COMPILER_LAUNCHER:-ccache}"
    launcher_config=""
    launcher_args=()
    if [[ -n "${launcher}" && "${launcher}" != "OFF" && "${launcher}" != "off" ]]; then
        if command -v "${launcher}" >/dev/null 2>&1; then
            launcher_config="${launcher}"
            launcher_args=(
                "-DCMAKE_C_COMPILER_LAUNCHER=${launcher}"
                "-DCMAKE_CXX_COMPILER_LAUNCHER=${launcher}"
            )
        else
            launcher_args=(
                "-DCMAKE_C_COMPILER_LAUNCHER="
                "-DCMAKE_CXX_COMPILER_LAUNCHER="
            )
        fi
    else
        launcher_args=(
            "-DCMAKE_C_COMPILER_LAUNCHER="
            "-DCMAKE_CXX_COMPILER_LAUNCHER="
        )
    fi
    config="$(printf '%s\n' \
        "release={{release}}" \
        "cmake_args=${DVZ_CMAKE_ARGS:-}" \
        "generator=Ninja" \
        "compiler_launcher=${launcher_config}" \
        "export_compile_commands=ON")"
    needs_configure=0
    if [[ "{{mode}}" == "force" || ! -f "${build_dir}/CMakeCache.txt" || ! -f "${stamp}" ]]; then
        needs_configure=1
    elif ! cmp -s <(printf '%s\n' "${config}") "${stamp}"; then
        needs_configure=1
    fi
    if [[ "${needs_configure}" -eq 0 ]]; then
        exit 0
    fi
    cmake_args="${DVZ_CMAKE_ARGS:-}"
    (
        cd "${build_dir}"
        # Preserve existing behavior: DVZ_CMAKE_ARGS is a shell-split string of extra CMake flags.
        # shellcheck disable=SC2086
        cmake .. -GNinja \
            -DCMAKE_BUILD_TYPE="{{release}}" \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            "${launcher_args[@]}" \
            ${cmake_args}
    )
    printf '%s\n' "${config}" > "${stamp}"

[linux]
[macos]
configure release="Debug":
    @just _build-configure "{{release}}" force

[linux]
[macos]
compile:
    #!/usr/bin/env bash
    set -euo pipefail
    build_args=()
    if [[ -n "${DVZ_BUILD_JOBS:-}" ]]; then
        build_args+=(--parallel "${DVZ_BUILD_JOBS}")
    fi
    cmake --build build "${build_args[@]}"

[linux]
[macos]
build release="Debug":
    #!/usr/bin/env bash
    set -euo pipefail
    just _build-configure "{{release}}" auto
    build_args=()
    if [[ -n "${DVZ_BUILD_JOBS:-}" ]]; then
        build_args+=(--parallel "${DVZ_BUILD_JOBS}")
    fi
    cmake --build build "${build_args[@]}"
#

[linux]
[macos]
build-stats:
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ ! -f build/build.ninja ]]; then
        echo "build/build.ninja missing; run 'just build' first"
        exit 1
    fi
    commands="$(ninja -C build -t commands)"
    count_matches() {
        local pattern="$1"
        local count
        count="$(printf '%s\n' "${commands}" | rg -c "${pattern}" || true)"
        if [[ -z "${count}" ]]; then
            count=0
        fi
        printf '%s' "${count}"
    }
    echo "Ninja compile commands: $(count_matches ' -c ')"
    echo "Commands using compiler launcher: $(count_matches '(^| )([^ ]*/)?(ccache|sccache) ')"
    echo "Example shared helper compiles: $(count_matches ' -c .*(example_common\.c|example_style\.c|runner/scenario_runner\.c)')"
    if command -v ccache >/dev/null 2>&1; then
        ccache --show-stats | sed -n '1,14p'
    fi
#

[linux]
build-gprof release="RelWithDebInfo":
    #!/usr/bin/env bash
    set -euo pipefail
    unset CC
    unset CXX
    mkdir -p docs/images build-gprof
    launcher="${DVZ_COMPILER_LAUNCHER:-ccache}"
    launcher_args=()
    if [[ -n "${launcher}" && "${launcher}" != "OFF" && "${launcher}" != "off" ]] && command -v "${launcher}" >/dev/null 2>&1; then
        launcher_args=(
            "-DCMAKE_C_COMPILER_LAUNCHER=${launcher}"
            "-DCMAKE_CXX_COMPILER_LAUNCHER=${launcher}"
        )
    fi
    cmake_args="${DVZ_CMAKE_ARGS:-}"
    (
        cd build-gprof
        # shellcheck disable=SC2086
        cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}} -DDVZ_ENABLE_GPROF=ON \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "${launcher_args[@]}" ${cmake_args}
    )
    build_args=()
    if [[ -n "${DVZ_BUILD_JOBS:-}" ]]; then
        build_args+=(--parallel "${DVZ_BUILD_JOBS}")
    fi
    cmake --build build-gprof --target dvztest "${build_args[@]}"
#

[linux]
build-profile:
    #!/usr/bin/env bash
    set -euo pipefail
    unset CC
    unset CXX
    mkdir -p docs/images build-profile
    launcher="${DVZ_COMPILER_LAUNCHER:-ccache}"
    launcher_args=()
    if [[ -n "${launcher}" && "${launcher}" != "OFF" && "${launcher}" != "off" ]] && command -v "${launcher}" >/dev/null 2>&1; then
        launcher_args=(
            "-DCMAKE_C_COMPILER_LAUNCHER=${launcher}"
            "-DCMAKE_CXX_COMPILER_LAUNCHER=${launcher}"
        )
    fi
    cmake_args="${DVZ_CMAKE_ARGS:-}"
    (
        cd build-profile
        # shellcheck disable=SC2086
        cmake .. -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDVZ_USE_VALIDATION=OFF \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "${launcher_args[@]}" ${cmake_args}
    )
    build_args=()
    if [[ -n "${DVZ_BUILD_JOBS:-}" ]]; then
        build_args+=(--parallel "${DVZ_BUILD_JOBS}")
    fi
    cmake --build build-profile --target dvz_live_canvas "${build_args[@]}"
#

[linux]
run-gprof *args:
    @set -e
    @if [ ! -x "build-gprof/testing/dvztest" ]; then echo "build-gprof/testing/dvztest missing; run 'just build-gprof' first"; exit 1; fi
    @cd build-gprof/testing && ./dvztest {{args}}
    @if [ -f build-gprof/testing/gmon.out ]; then \
        echo "Profile stored in build-gprof/testing/gmon.out"; \
    else \
        echo "warning: gmon.out not produced"; \
    fi
#

[linux]
gprof-report gmon="build-gprof/testing/gmon.out" out="build-gprof/testing/gprof.txt":
    @set -e
    @bin="build-gprof/testing/dvztest"; \
    if [ ! -x "$bin" ]; then echo "$bin missing; run 'just build-gprof' first"; exit 1; fi; \
    gmon_file="{{gmon}}"; \
    if [ ! -f "$gmon_file" ]; then echo "profile '$gmon_file' not found"; exit 1; fi; \
    out_file="{{out}}"; \
    mkdir -p "$(dirname "$out_file")"; \
    gprof "$bin" "$gmon_file" > "$out_file"; \
    echo "Report written to $out_file"
#



[linux]
_sanitizer-build name:
    @set -e
    @mkdir -p docs/images build-{{name}}
    @cd build-{{name}}/ && \
      CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DDVZ_ENABLE_ASAN_IN_DEBUG=$([ "{{name}}" = "asan" ] && echo ON || echo OFF) \
        -DDVZ_ENABLE_CUDA=OFF \
        -DDVZ_SANITIZER={{name}} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    @cd build-{{name}}/ && ninja
#

[linux]
msan:
    just _sanitizer-build msan
#

[linux]
asan:
    just _sanitizer-build asan
#

[linux]
tsan:
    just _sanitizer-build tsan
#

[windows]
[linux]
release: symbols
    just build "Release" || just build "Release"
#

[macos]
release: symbols && bundledeps
    just build "Release" || just build "Release"
#

[windows]
build release="Debug":
    #!/usr/bin/env sh
    set -e
    unset CC
    unset CXX
    cmake_args="${DVZ_CMAKE_ARGS:-}"
    BUILD_DIR="build"
    mkdir -p "$BUILD_DIR"

    # Copy MinGW runtime libraries next to the built artifacts.
    MINGW64_DIR="$(dirname "$(which gcc)")"
    cp "$MINGW64_DIR/libgcc_s_seh-1.dll" "$BUILD_DIR"/
    cp "$MINGW64_DIR/libstdc++-6.dll" "$BUILD_DIR"/
    cp "$MINGW64_DIR/libwinpthread-1.dll" "$BUILD_DIR"/

    launcher="${DVZ_COMPILER_LAUNCHER:-ccache}"
    launcher_args=""
    if [ -n "$launcher" ] && [ "$launcher" != "OFF" ] && [ "$launcher" != "off" ] && command -v "$launcher" >/dev/null 2>&1; then
        launcher_args="-DCMAKE_C_COMPILER_LAUNCHER=$launcher -DCMAKE_CXX_COMPILER_LAUNCHER=$launcher"
    fi
    # shellcheck disable=SC2086
    cmake --preset=mingw -DCMAKE_BUILD_TYPE={{release}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${launcher_args} ${cmake_args}
    build_args=""
    if [ -n "${DVZ_BUILD_JOBS:-}" ]; then
        build_args="--parallel ${DVZ_BUILD_JOBS}"
    fi
    # shellcheck disable=SC2086
    cmake --build --preset mingw ${build_args}

    # Copy vcpkg_installed dll's to datoviz.exe location.
    if [ "{{release}}" == "Debug" ]; then
        cp "$BUILD_DIR/vcpkg_installed/x64-windows/debug/bin/"*.dll "$BUILD_DIR"/
    else
        cp "$BUILD_DIR/vcpkg_installed/x64-windows/bin/"*.dll "$BUILD_DIR"/
    fi
#

[windows]
msvc release="Debug":
    #!/usr/bin/env sh
    set -e
    unset CC
    unset CXX
    cmake_args="${DVZ_CMAKE_ARGS:-}"
    BUILD_DIR="build-msvc"
    mkdir -p "$BUILD_DIR"

    launcher="${DVZ_COMPILER_LAUNCHER:-ccache}"
    launcher_args=""
    if [ -n "$launcher" ] && [ "$launcher" != "OFF" ] && [ "$launcher" != "off" ] && command -v "$launcher" >/dev/null 2>&1; then
        launcher_args="-DCMAKE_C_COMPILER_LAUNCHER=$launcher -DCMAKE_CXX_COMPILER_LAUNCHER=$launcher"
    fi
    # shellcheck disable=SC2086
    cmake --preset=msvc -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${launcher_args} ${cmake_args}
    build_args=""
    if [ -n "${DVZ_BUILD_JOBS:-}" ]; then
        build_args="--parallel ${DVZ_BUILD_JOBS}"
    fi
    # shellcheck disable=SC2086
    cmake --build --preset msvc --config {{release}} ${build_args}

    # Copy vcpkg_installed dll's next to datoviz.exe.
    if [ "{{release}}" == "Debug" ]; then
        cp "$BUILD_DIR/vcpkg_installed/x64-windows/debug/bin/"*.dll "$BUILD_DIR"/
    else
        cp "$BUILD_DIR/vcpkg_installed/x64-windows/bin/"*.dll "$BUILD_DIR"/
    fi
#

# Cross-compile Datoviz for Windows using MinGW-w64 (GNU ABI)
[linux]
mingw release="Debug":
    #!/usr/bin/env bash
    set -euo pipefail
    unset CC
    unset CXX
    mkdir -p "build-mingw" docs/images
    launcher="${DVZ_COMPILER_LAUNCHER:-ccache}"
    launcher_args=()
    if [[ -n "${launcher}" && "${launcher}" != "OFF" && "${launcher}" != "off" ]] && command -v "${launcher}" >/dev/null 2>&1; then
        launcher_args=(
            "-DCMAKE_C_COMPILER_LAUNCHER=${launcher}"
            "-DCMAKE_CXX_COMPILER_LAUNCHER=${launcher}"
        )
    fi
    (
    cd "build-mingw"
    cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE={{release}} \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
        -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        "${launcher_args[@]}"
    )
    build_args=()
    if [[ -n "${DVZ_BUILD_JOBS:-}" ]]; then
        build_args+=(--parallel "${DVZ_BUILD_JOBS}")
    fi
    cmake --build build-mingw "${build_args[@]}"
