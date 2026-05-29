# Build, Test, And Module Rules

These rules cover the build system, include boundaries, module activation, and validation loops.


## Active Modules

Active modules currently linked into `libdatoviz` by default are:

`common`, `ds`, `fileio`, `geom`, `math`, `thread`, `input`, `window`, `canvas`,
`stream`, `video`, `vk`, `vklite`, `drp2`, `scene`, and `app`.

Modules such as `color`, `wasm`, richer text/gui layers, and broader renderer/client layers remain
scaffolding. Keep them untouched unless a task explicitly activates them.


## Build Commands

Run from the repository root:

```sh
just clean
just build
just test [filter]
```

Use `just build` for normal compilation and `just test [filter]` for the repo test workflow.

For tests depending on Vulkan, GLFW, Metal, swapchains, or graphics presentation on macOS, use the
repo environment:

```sh
direnv exec . just test [filter]
```


## Validation Strategy

Prefer the narrowest relevant validation loop while iterating:

1. Use `just test <module-or-filter>` for focused module work.
2. Use focused binaries such as `dvztest_drp2`, `dvztest_scene`, `dvztest_vk`,
   `dvztest_canvas`, or `dvztest_integration` when available.
3. Use `just test` or `dvztest` for broad validation.
4. Run `git diff --check` before finalizing code changes.

For nontrivial C changes touching allocation, byte sizes, pointer lifetimes, object tables, Vulkan
resources, command buffers, frame lifetimes, or synchronization, consider static or dynamic
analysis when practical.

Useful tools include `clang-tidy`, `scan-build`, `cppcheck`, ASan/UBSan builds, Valgrind for
CPU-only paths, Vulkan validation layers, and representative live smoke loops such as
`dvz_live_canvas --frames 300`. If a tool is unavailable, too noisy, or impractical, report that
and fall back to focused tests.


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

Add the module with `add_subdirectory(<module>)` and link the object library from
`src/CMakeLists.txt` only when the module is active.

Use `${DVZ_COMPILE_DEFINITIONS}` for module target definitions. Do not copy legacy patterns that
refer to `${COMPILE_DEFINITIONS}` in new code.


## Include Boundaries

Public headers live under `include/datoviz/` and are installed. Internal implementation lives under
`src/`. Shared internal helpers live under `src/common/`.

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

Keep `${PROJECT_SOURCE_DIR}/src/common` on module and test include paths so `_macros.h` and other
shared internals remain reachable until the public/private split is revisited.

Only install headers from `include/datoviz/`.


## Testing Pattern

All module test sources live under `src/<module>/tests/*.c*`.

Each module exposes an entry point such as:

```c
int test_<module>(TstSuite* suite);
```

That function appends test cases to the shared suite using the `testing.h` suite/item API
(`TEST_SIMPLE`, `TEST`, `AT`, `AC`, `ACn`, and related helpers). Invoke module entry points from
the unified runner; do not use constructors for registration.

For intentionally failing/error-path checks, wrap the assertion in an expected-error scope so known
`log_error()` output is suppressed:

```c
AT_EXPECTED_ERROR_STRICT(suite, (expr_that_should_fail));
```

The unified `dvztest` runner remains the broad aggregation point.
