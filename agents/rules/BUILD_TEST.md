# Build, Test, And Module Rules

These rules cover the build system, include boundaries, module activation, and validation loops.

## Module Activation

Use the `DVZ_BUILD_*` options in [CMakeLists.txt](../../CMakeLists.txt) and conditional targets in [src/CMakeLists.txt](../../src/CMakeLists.txt) as the module inventory. Controller primitives and the Dear ImGui overlay are enabled by default; optional targets follow their build flags and dependency checks. Do not infer module status from historical scaffolding notes.

## Build Commands

Run from the repository root:

```sh
just build
just test <filter>
```

Use `just build` for incremental compilation and `just test <filter>` for focused tests. Use `just clean` only when diagnosing stale build output or intentionally rebuilding from scratch.

For tests depending on Vulkan, GLFW, Metal, swapchains, or graphics presentation on macOS, use the repo environment:

```sh
direnv exec . just test <filter>
```

## Python Binding Freshness

Datoviz's generated Python binding files are local artifacts. Before running Python-facing validation, GSP integration checks, packaging smoke tests, or release checks after public API work, make the local binding match the headers:

```sh
just ctypes
just ctypes-check
```

Run this whenever a task touches:

- `include/datoviz/**`
- exported `dvz_*` function signatures or public structs/enums
- `spec/bindings/**`
- `tools/bindings/**`
- generated facade behavior or Python packaging paths that import `datoviz._ctypes`

`just ctypes-check` includes the binding freshness check, NumPy-adaptation check, binding policy validation, and C ABI layout validation. If it fails, regenerate with `just ctypes`, fix the header/policy/generator mismatch, and rerun the check before reporting success.

## Validation Strategy

Instruction-only changes need relevant link/content checks and `git diff --check`. Public documentation follows [documentation validation](../now/DOCUMENTATION.md), including recipe build dependencies. For code, use the narrowest relevant validation loop:

1. Use `just test <module-or-filter>` for focused module work.
2. Use focused binaries such as `dvztest_drp2`, `dvztest_scene`, `dvztest_vk`, `dvztest_canvas`, or `dvztest_integration` when available.
3. Use `just ctypes-check` after public API/header/binding changes, especially before Python or GSP integration validation.
4. Use `just test` or `dvztest` when the affected scope requires broad validation.
5. Run `git diff --check` before finalizing code changes.

Once required checks pass, repeat or broaden them only for new changes, failures, or unresolved risks. Keep exact-artifact release validation separate from ordinary implementation checks.

For nontrivial C changes touching allocation, byte sizes, pointer lifetimes, object tables, Vulkan resources, command buffers, frame lifetimes, or synchronization, consider static or dynamic analysis when practical.

Useful tools include `clang-tidy`, `scan-build`, `cppcheck`, ASan/UBSan builds, Valgrind for CPU-only paths, Vulkan validation layers, and representative live smoke loops such as `dvz_live_canvas --frames 300`. If a tool is unavailable, too noisy, or impractical, report that and fall back to focused tests.

## Gallery Media

Before changing capture, animation, encoding, or media-cache behavior, read the [gallery media pipeline](../../docs/contributors/gallery-media.md). For example contributions and promotion, read [adding examples](../../docs/contributors/adding-examples.md).

## CMake Pattern

Modules build as object libraries. New modules should follow the existing pattern:

```cmake
file(GLOB MODULE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_<module> OBJECT ${MODULE_SRC})

target_include_directories(datoviz_<module>
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(datoviz_<module> PUBLIC ${DVZ_COMPILE_DEFINITIONS})
```

Add the module with `add_subdirectory(<module>)` and link the object library from `src/CMakeLists.txt` only when the module is active.

Use `${DVZ_COMPILE_DEFINITIONS}` for module target definitions. Do not copy legacy patterns that refer to `${COMPILE_DEFINITIONS}` in new code.

## Include Boundaries

Public headers live under `include/datoviz/` and are installed. Internal implementation lives under `src/`. Shared internal helpers live under `src/common/`.

Use public includes for public API:

```c
#include "datoviz/math.h"
#include "datoviz/math/vec.h"
```

Use shared internal includes in implementation:

```c
#include "_alloc.h"
#include "_assertions.h"
```

Keep `${PROJECT_SOURCE_DIR}/src/common` on module and test include paths so `_macros.h` and other shared internals remain reachable until the public/private split is revisited.

Only install headers from `include/datoviz/`.

## Testing Pattern

All module test sources live under `src/<module>/tests/*.c*`.

Each module exposes an entry point such as:

```c
int test_<module>(TstSuite* suite);
```

That function appends test cases to the shared suite using the `testing.h` suite/item API (`TEST_SIMPLE`, `TEST`, `AT`, `AC`, `ACn`, and related helpers). Invoke module entry points from the unified runner; do not use constructors for registration.

For intentionally failing/error-path checks, wrap the assertion in an expected-error scope so known `log_error()` output is suppressed:

```c
AT_EXPECTED_ERROR_STRICT(suite, (expr_that_should_fail));
```

The unified `dvztest` runner remains the broad aggregation point.
