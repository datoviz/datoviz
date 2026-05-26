# 🧠 **AGENTS.md – Datoviz v0.4-dev Architecture & Refactor Guide**

## 📘 Overview

This document provides essential rules and conventions for Codex (and other automated agents) to understand, modify, and extend the **Datoviz v0.4-dev** codebase.

Datoviz is a **modular C scientific visualization library**, currently being **refactored for version 0.4** with a clean, maintainable, and testable architecture. Backward compatibility with `v0.3` is not a constraint for the refactor; we prefer improving the architecture and cleaning up the codebase even if it means diverging from the old APIs.

The goals:

* One shared library target: **`datoviz`** (`libdatoviz` with platform-specific extension)
* Modular **object libraries** per component
* Clear boundary between **public API** and **internal code**
* Unified **test runner** for all modules
* CMake-based build and test system

### Branch policy (v0.4-dev)

In this branch, the Datoviz API is not treated as consumed by external users yet. Agents may make aggressive
API and ABI changes and can break compatibility whenever it improves architecture, correctness, or long-term
maintainability.

When refactoring, do NOT delete existing comments, keep them and update them if needed, but do not delete them.

Do NOT commit changes inside the `data` submodule, or commit large binary files anywhere in the
repository, without explicit approval from Cyrille for that specific commit.


### Current branch snapshot (2026-05-17)

* The low-level graphics stack (`vk`, `vklite`, `canvas`, `stream`, `video`, `window`) has completed
  its main ownership/boundary cleanup pass and remains the runtime foundation for v0.4.
* `drp2`, `scene`, and `app` are active v0.4 modules, not future scaffolding. The current vertical
  slice exists: scene/frame-plan emission -> DRP2 command stream -> vklite runtime -> canvas/stream
  frame execution, plus a small app presentation layer for offscreen and GLFW windows.
* Built-in scene visuals now include point, pixel, marker, primitive, mesh, path/segment, image,
  volume, and sphere impostors. Scene support also covers retained sampled fields, image colormap
  scale binding, colorbar bookkeeping, panzoom/arcball/fly/turntable controllers, narrow
  text/annotation bookkeeping, GPU-backed point pick / image probe request paths, and graph-backed
  panel techniques.
* Recent commits through `2026-05-17` hardened descriptor refresh for recreated DRP2 resources,
  added graph-backed EDL, SSAO, MSAA, sphere, and material-model paths, and centralized the active
  scene shader ABI, vertex attribute descriptor writes, depth-state decisions, and runtime bind
  layout ordering.
* Current recorded focused validation includes `just spec-check` with `123/123` DRP2 fixtures,
  `35/35` WebGPU preflight fixtures, `52` fixture-runner tests, and `7` schema/generation tests;
  `just test drp2` has passed `119/119`; `just test scene` has passed beyond the first
  EDL/material slices, with later focused shader/visual-family checks recorded in
  `agents/now/START.md`. Re-run the narrow target before relying on a newer slice.
* For the current execution summary and next-step guidance, start with
  `agents/now/START.md`, then use `agents/README.md` to find completed phase records.
* For the v0.4 feature-freeze, release-candidate, validation, packaging, and final-release roadmap,
  start with `agents/now/RELEASE.md`.

### 🏗️ Current refactor status (v0.4-dev)

* ✅ Active modules currently linked into `libdatoviz` by default are: `common`, `ds`, `fileio`,
  `geom`, `math`, `thread`, `input`, `window`, `canvas`, `stream`, `video`, `vk`, `vklite`,
  `drp2`, `scene`, and `app`.
* ✅ The active low-level graphics stack is the stable foundation; use it rather than creating a
  parallel presentation, frame-stream, or Vulkan wrapper path.
* 🚧 The highest-value remaining work is now targeted hardening: WebGPU/WGSL parity lanes,
  material/technique polish, richer picking/selection payloads, rendered text/annotations, and
  example/gallery pressure tests on the existing scene -> DRP2 -> runtime path.
* ✅ The active scene slice now covers retained visual rendering, repeated partial updates,
  multi-panel figures, per-panel runtime viewport/scissor handling, depth-enabled 2D/3D passes,
  request readbacks, descriptor refresh after stable resource recreation, and graph-backed
  postprocess / transparency / MSAA techniques through the scene -> DRP2 -> vklite/canvas path.
* ⏭️ Several other directories/headers remain scaffolding (for example `color`, `wasm`, text/gui,
  and richer renderer/client layers); keep them untouched unless explicitly requested.

---

## 🧩 **Project Structure**

**Important**: do not treat `v0.3` as an implementation target or compatibility constraint. Focus on
`include`, `src`, `testing`, and supporting roots such as `external/`.

### Documentation placement during v0.4 refactor

The current `docs/` tree is legacy v0.3 public documentation and should not receive new v0.4 design
notes, specifications, implementation plans, or architecture records. Put active v0.4 source-of-truth
material under `spec/scene/` (or the closest existing `spec/` subtree), and use `agents/` only for
execution status, handoff notes, and automation plans. Treat `docs/` as a future migration target when
the v0.4 public documentation is rebuilt.

Completed agent plans should not remain in active queues. When work tracked under `agents/now/` or
`agents/soon/` is done, remove the active plan, move or rewrite the final implementation record under
`agents/done/`, and update README/index links so future agents do not treat finished work as pending.

```
datoviz/
├── include/                    # Public headers (installed)
│   └── datoviz/
│       ├── datoviz.h           # Umbrella header
│       ├── app.h, canvas.h, common.h, drp2.h, ds.h, fileio.h, scene.h, stream.h, vk.h ...
│       ├── canvas/…, common/…, drp2/…, ds/…, math/…, scene/…, thread/…, vk/…, vklite/…
│       └── (additional module directories exist; many are stubs for upcoming work)
│
├── src/                        # Internal implementation (not installed)
│   ├── common/
│   │   ├── _alloc.h _assertions.h _error.h _log.h _macros.h _mutex.h _obj.h _time_utils.h
│   │   ├── assert.c error.c log.c mutex.c obj.c version.c
│   │   └── tests/
│   ├── ds/                     # Data-structure primitives
│   │   ├── list.c
│   │   └── map.cpp
│   ├── fileio/                 # File helpers
│   │   └── fileio.c
│   ├── geom/                   # CPU-side geometry containers and generators
│   ├── math/                   # Math utilities (C and C++)
│   │   ├── anim.c array.c box.c mock.c parallel.c prng.cpp rand.c stats.c vec.c
│   │   └── tests/
│   ├── thread/                 # Threading primitives
│   │   ├── atomic.cpp fifo.c thread.c
│   ├── input/                  # Input/router module
│   ├── window/                 # Window backends and event plumbing
│   ├── canvas/                 # Canvas/frame management
│   ├── stream/                 # Frame stream/sink registry
│   ├── video/                  # Video encoder backends and sink
│   ├── vk/                     # Vulkan backend (core Vulkan helpers)
│   ├── vklite/                 # Higher-level Vulkan convenience layer
│   ├── drp2/                   # Backend-agnostic rendering protocol stream + runtime
│   ├── scene/                  # Scene graph, frame-plan, retained objects, and DRP2 emitter
│   ├── app/                    # Small presentation layer over scene + canvas/runtime
│   ├── empty.c                 # Keeps the shared library non-empty
│   └── CMakeLists.txt          # Collects object modules into libdatoviz
│       (additional module folders exist; some are placeholders)
│
├── testing/
│   ├── testing.h               # Minimal test framework (suite/item API)
│   ├── testing.cpp
│   ├── CMakeLists.txt
│   └── dvztest.c               # Unified test runner
│
├── external/                   # Vendored third-party sources (cglm, tinycthread, etc.)
└── CMakeLists.txt              # Root build definition
```

Modules currently compiled into `libdatoviz` when the default build options are on: **`common`, `ds`,
`fileio`, `geom`, `math`, `thread`, `input`, `window`, `canvas`, `stream`, `video`, `vk`, `vklite`,
`drp2`, `scene`, and `app`**. Modules such as `color`/`wasm` remain inactive scaffolding and should stay
untouched unless explicitly requested.

### ⏩ Planned activation order

1. Harden the active scene/DRP2/app vertical slice: retained resources, borrowed canvas frame
   targets, runtime execution, readback/capture, request processing, and failure paths.
2. Pressure-test native 3D with mesh/depth/arcball examples before broad visual-family expansion.
3. Start a narrow WebGPU feasibility lane against the already-active DRP2 subset, without forking
   scene semantics or creating a parallel renderer contract.
4. Keep non-activated modules as scaffolding unless a task explicitly brings one online.

---

## ⚙️ **Build System (CMake)**

* Modules build as **OBJECT libraries** that glob both C and C++ sources:

  ```cmake
  file(GLOB MATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
  add_library(datoviz_math OBJECT ${MATH_SRC})

  target_include_directories(datoviz_math
      PUBLIC
          ${PROJECT_SOURCE_DIR}/include
          ${PROJECT_SOURCE_DIR}/src/common
          ${PROJECT_SOURCE_DIR}/external/cglm/include
  )

  target_compile_definitions(datoviz_math PUBLIC ${DVZ_COMPILE_DEFINITIONS})
  ```

* `src/common/CMakeLists.txt` publishes its directory with `INTERFACE` usage requirements so any consumer of `datoviz_common` can include `_alloc.h`, `_macros.h`, etc.
* `src/vk/CMakeLists.txt` follows the same OBJECT-library pattern (`datoviz_vk`), uses
  `${PROJECT_SOURCE_DIR}/external/volk`, and links with `datoviz_volk` so Vulkan entry points are resolved
  via Volk. The repository also contains `libs/vulkan`/`libs/shaderc` assets used by some packaging/test
  workflows (especially in `justfile` recipes).

* The root `src/CMakeLists.txt` assembles the shared library and registers the active modules:

  ```cmake
  add_library(datoviz SHARED empty.c)

  add_subdirectory(common)
  add_subdirectory(ds)
  add_subdirectory(fileio)
  add_subdirectory(geom)
  add_subdirectory(math)
  add_subdirectory(thread)
  add_subdirectory(input)
  add_subdirectory(window)
  add_subdirectory(canvas)
  add_subdirectory(stream)
  add_subdirectory(video)
  add_subdirectory(vk)
  add_subdirectory(vklite)
  add_subdirectory(drp2)
  add_subdirectory(scene)
  add_subdirectory(app)

  target_link_libraries(datoviz
      PRIVATE
          datoviz_common
          datoviz_ds
          datoviz_fileio
          datoviz_geom
          datoviz_math
          datoviz_thread
          datoviz_input
          datoviz_window
          datoviz_canvas
          datoviz_stream
          datoviz_video
          datoviz_vk
          datoviz_vklite
          datoviz_drp2
          datoviz_scene
          datoviz_app
          datoviz_volk
  )

  target_include_directories(datoviz
      PUBLIC
          ${PROJECT_SOURCE_DIR}/include
  )
  ```

`DVZ_COMPILE_DEFINITIONS` is assembled in `src/CMakeLists.txt` (OS/compiler switches,
`ENABLE_VALIDATION_LAYERS`, `DEBUG`, `VK_NO_PROTOTYPES`, feature flags), exported through a global property,
and then applied to registered targets in the top-level `CMakeLists.txt` (`src` modules and testing targets).
The current root build exposes layered feature options (`DVZ_BUILD_CORE`, `DVZ_BUILD_VK`,
`DVZ_BUILD_CANVAS`, `DVZ_BUILD_DRP2`, `DVZ_BUILD_SCENE`, `DVZ_BUILD_APP`, and currently-off
`DVZ_BUILD_WEBGPU`) and links only the enabled object modules into `libdatoviz`.
Some existing module `CMakeLists.txt` files still reference `${COMPILE_DEFINITIONS}` as legacy wiring;
do not copy that pattern in new code—use `${DVZ_COMPILE_DEFINITIONS}` instead.

## 🛠️ **Build & Test Commands**

The top-level `justfile` provides the primary workflow; Codex should stick to:

* `just clean` — remove generated build artifacts so the next build starts fresh.
* `just build` — configure (if needed) and compile the active targets through CMake.
* `just test [filter]` — execute the repo test workflow after a successful build; the unified
  `dvztest` runner remains the broad validation path.

Run these commands from the repository root. Other `just` recipes exist but are currently out of scope.

### Sandbox note (macOS + Vulkan)

When running tests that depend on Vulkan/GLFW/Metal (for example `vk`, `vklite`, `canvas`, `window`, or
filters such as `test_canvas_glfw`), do not rely on a strict sandboxed runtime. On macOS, sandboxed
execution can block Cocoa/Metal/LaunchServices access and make Vulkan initialization fail or hang.

Use:

* `direnv exec . just test [filter]` so the repo Vulkan environment from `.envrc` is applied.
* An unsandboxed/escalated command execution context for these graphics tests.

Keep regular non-graphics checks in sandbox when possible, but prefer unsandboxed execution for Vulkan-path
validation.

### Public vs Internal Includes

| Type               | Include Path                 | Example                             | Notes                                                        |
| ------------------ | ---------------------------- | ----------------------------------- | ------------------------------------------------------------ |
| Public aggregator  | `include/datoviz/math.h`     | `#include "datoviz/math.h"`         | Re-exports `math/*.h`, mirrors other modules                 |
| Public subheader   | `include/datoviz/math/vec.h` | `#include "datoviz/math/vec.h"`     | Most subheaders depend on `_macros.h` from `src/common`      |
| Shared internals   | `src/common/_alloc.h`        | `#include "_alloc.h"`               | Available because modules/tests add `${PROJECT_SOURCE_DIR}/src/common` |
| Module source file | `src/math/vec.c`             | `#include "datoviz/math/vec.h"`     | Implementations rely on public headers                       |
| Test helpers       | `src/common/tests/test_common.h` | `#include "testing.h"`           | Each module keeps optional `tests/` headers alongside sources |

Headers are installed under `${CMAKE_INSTALL_PREFIX}/include/datoviz/`. They currently expect `_macros.h`
(and friends) to be reachable through the build tree; keep `${PROJECT_SOURCE_DIR}/src/common` on include
paths until the public/private split is revisited.

```c
#include <datoviz/datoviz.h>
#include <datoviz/math.h>
```

---

## 🧠 **Header Naming & Inclusion Rules**

| Purpose               | File Example                  | Include Form                | Notes                               |
| --------------------- | ----------------------------- | --------------------------- | ----------------------------------- |
| Public API            | `include/datoviz/math.h`      | `#include "datoviz/math.h"` | Installed, stable                   |
| Submodule header      | `include/datoviz/math/rand.h` | Included from `math.h`      | Re-exported by the aggregator       |
| Shared internal       | `src/common/_alloc.h`         | `#include "_alloc.h"`       | Private helpers shared by modules   |
| Private module header | `src/color/_color.h`          | Local to module             | Optional, only when extra internals |
| Implementation file   | `src/common/assert.c`         | uses public + internal hdrs | Implementation stays with the module |

### ⚠ Reserved Identifier Note

Filenames starting with `_` are **legal**. In this codebase, internal helpers often use `_dvz_*`; keep using
that existing convention for internal symbols. Avoid introducing reserved forms such as double-underscore
identifiers (`__name`) or underscore-capital names (`_NAME`) in new code.

---

## 🧩 **Testing System**

### Structure

* All test sources live under `src/<module>/tests/*.c*`.
* Each module provides an entry-point (for example `int test_common(TstSuite* suite)`) that appends its test cases to the shared `TstSuite`.
* Assertions use the helpers in `testing.h` (`AT`, `AC`, `ACn`, `TEST_SIMPLE`, etc.).
* The repo keeps a unified runner executable, **`dvztest`**, and also builds narrower test binaries
  such as `dvztest_core`, `dvztest_vk`, `dvztest_canvas`, and `dvztest_integration` for focused loops.
* The unified runner builds the full suite, runs optional filters, prints summaries, and lists
  failing test names explicitly when failures occur.

### Example

```c
// src/common/tests/test_obj.c
#include "_assertions.h"
#include "_obj.h"
#include "test_common.h"
#include "testing.h"

int test_obj_1(TstSuite* suite, TstItem* item)
{
    ANN(suite);
    /* ... assertions with AT(...) ... */
    return 0;
}

int test_common(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "common";
    TEST_SIMPLE(test_obj_1);
    return 0;
}
```

### Runner

```c
// testing/dvztest.c
#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
#include "../src/app/tests/test_app.h"
#endif
#include "../src/common/tests/test_common.h"
#include "../src/ds/tests/test_ds.h"
#if defined(DVZ_HAS_DRP2) && DVZ_HAS_DRP2
#include "../src/drp2/tests/test_drp2.h"
#endif
#include "../src/fileio/tests/test_fileio.h"
#include "../src/math/tests/test_math.h"
#if defined(DVZ_HAS_SCENE) && DVZ_HAS_SCENE
#include "../src/scene/tests/test_scene.h"
#endif
#include "../src/stream/tests/test_stream.h"
#include "../src/thread/tests/test_thread.h"
#include "../src/input/tests/test_input.h"
#include "../src/window/tests/test_window.h"
#include "../src/canvas/tests/test_canvas.h"
#if (defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA) || (defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
#include "../src/video/tests/test_video.h"
#endif
#include "../src/vk/tests/test_vk.h"
#include "../src/vklite/tests/test_vklite.h"
#include "testing.h"

int main(int argc, char** argv)
{
    TstSuite suite = tst_suite();

#if defined(DVZ_HAS_APP) && DVZ_HAS_APP
    test_app(&suite);
#endif
    test_common(&suite);
    test_ds(&suite);
#if defined(DVZ_HAS_DRP2) && DVZ_HAS_DRP2
    test_drp2(&suite);
#endif
    test_fileio(&suite);
    test_math(&suite);
#if defined(DVZ_HAS_SCENE) && DVZ_HAS_SCENE
    test_scene(&suite);
#endif
    test_stream(&suite);
    test_thread(&suite);
    test_input(&suite);
    test_window(&suite);
    test_canvas(&suite);
#if (defined(DVZ_HAS_CUDA) && DVZ_HAS_CUDA) || (defined(DVZ_HAS_KVZ) && DVZ_HAS_KVZ)
    test_video(&suite);
#endif
    test_vk(&suite);
    test_vklite(&suite);

    tst_suite_run(&suite, argc >= 2 ? argv[1] : NULL);
    tst_suite_destroy(&suite);
    return 0;
}
```

### CMake

```cmake
add_library(testing STATIC testing.cpp)
target_include_directories(testing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/common
)

file(GLOB TEST_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*/tests/*.c*"
    "dvztest.c"
)

add_executable(dvztest ${TEST_SOURCES})
target_link_libraries(dvztest PRIVATE
    datoviz_app
    datoviz_common
    datoviz_ds
    datoviz_fileio
    datoviz_math
    datoviz_thread
    datoviz_input
    datoviz_window
    datoviz_canvas
    datoviz_stream
    datoviz_video
    datoviz_vk
    datoviz_vklite
    datoviz_drp2
    datoviz_scene
    datoviz_volk
    testing
)
target_include_directories(dvztest PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/common
)
target_compile_definitions(dvztest PRIVATE ${DVZ_COMPILE_DEFINITIONS})
add_test(NAME dvztest COMMAND dvztest)
```

Prefer the narrowest relevant validation loop for the slice you are changing:

* use `just test` for broad validation,
* use focused targets such as `dvztest_drp2`, `dvztest_scene`, `dvztest_vk`, `dvztest_canvas`, or
  `dvztest_integration` when working in a specific subsystem,
* keep `dvztest` as the unified end-to-end runner.

---

## 🧱 **Include & Linking Rules**

| Scope                           | Include                            | Link/Usage                         |
| ------------------------------- | ---------------------------------- | ---------------------------------- |
| Public headers                  | `${PROJECT_SOURCE_DIR}/include`    | PUBLIC                             |
| Shared internals (`src/common`) | `${PROJECT_SOURCE_DIR}/src/common` | PUBLIC / INTERFACE (propagated)    |
| Module-local extras             | `${CMAKE_CURRENT_SOURCE_DIR}`      | Add as needed (usually PRIVATE)    |

### OBJECT library caveat

CMake does not link object libraries on its own; usage requirements only flow once a target consumes them. Keep `${PROJECT_SOURCE_DIR}/src/common` on every module/test include list so `_macros.h` and other shared internals stay reachable.

---

## 🧩 **Naming Conventions**

| Type               | Pattern                                           | Example                      |
| ------------------ | ------------------------------------------------- | ---------------------------- |
| Functions          | `dvz_<module>_<action>()`                         | `dvz_math_vec3_add()`        |
| Internal functions | static or prefixed with `_dvz_`                   | `_dvz_log_error()`           |
| Files              | lowercase, underscores                            | `_alloc.c`, `math.c`         |
| Headers            | `_name.h` for internals, `datoviz/*.h` for public | `_alloc.h`, `datoviz/math.h` |
| Macros             | uppercase with `DVZ_` prefix                      | `DVZ_PI`, `DVZ_CHECK()`      |

---

## ⚙️ **Testing & Common Internals**

Shared internal headers in `src/common/` provide:

* `_alloc.h` — allocation helpers and aligned memory
* `_assertions.h` — assertion helpers (`ANN`, `ASSERT`, etc.)
* `_error.h` — error callbacks and reporting buffer
* `_log.h` — logging API
* `_macros.h` — global macros (`EXTERN_C_*`, `DVZ_EXPORT`, `MUTE_ON`, …)
* `_mutex.h` — pthread/tinycthread wrappers
* `_obj.h` — lifecycle helpers for `DvzObject`
* `_time_utils.h` and `_env.h` — misc utilities shared across modules

They are not installed, so keep `src/common` in include paths whenever you touch headers that depend on them.

---

## 🧠 **Design Philosophy**

* **Single shared library target** (`datoviz` / `libdatoviz.*`)
* **Explicit modular dependencies**
* **Consistent naming and include structure**
* **Public headers live in `include/datoviz/`; shared `_*.h` stay in `src/common` and remain reachable via include dirs**
* **One unified test executable** (`dvztest`)
* **Public headers cleanly grouped by module**
* **Performance is a core requirement** — keep the graphics stack lean, preserve immediate present mode
  for run-as-fast-as-possible benchmarks, and treat unexpected frame pacing or overhead regressions as
  first-class issues to investigate.
* **Roadmap discipline** — stabilize active modules first; keep non-activated modules as scaffolding until
  explicitly requested.

---

## 🧾 **Coding Conventions**

* Document every new module-level function with a short Doxygen-style docstring immediately above the definition. Start the block with `/**`, provide a one-sentence summary, leave a blank line, then add `@param` tags for every argument and an `@return` tag whenever the function returns a value (plus `@note`/`@see` when helpful). Use a single space between the leading `*` and tag identifiers (e.g., ` * @param ...`), avoid extra spacing, keep the description line within the project's preferred width, and treat it as the canonical explanation of the symbol's behavior.
* Visually separate neighbouring top-level functions with three blank lines (three empty lines) to make each implementation stand out. Attach any descriptive comments directly above the function and keep extra text out of the blank space.
* Reserve the `dvz_` prefix for symbols exposed via public headers. Internal `static` helpers should avoid `dvz_` (use `_dvz_` or no prefix) so the public naming space stays predictable.
* Prefer `dvz_memcpy()` / `dvz_memset()` from `src/common/_alloc.h` / `_compat.h` over `memcpy()` / `memset()` so every copy or fill goes through the shared allocator wrappers.
* Never call `malloc`, `calloc`, or `free` directly; rely on the allocation/deallocation helpers declared in `_alloc.h` and `_compat.h` (prefer `dvz_calloc` over `dvz_malloc` when zeroed memory suffices, alongside `dvz_free`) so every allocation flows through the datoviz allocator. All structs should be zero-initialized before use, reinforcing the `dvz_calloc` preference (or `dvz_memset()` after allocation when needed).
* Favor `dvz_fprintf()` / `dvz_vfprintf()` over `fprintf()` / `vfprintf()` (from `src/common/_compat.h`) to keep logging and error reporting within the safe compatibility layer.
* Prefer C-style function signatures throughout the codebase, including C++ translation units (`*.cpp`): avoid C++ standard library types (`std::vector`, `std::string`, references, templates) in function signatures and favor plain C arguments (pointers, counts, POD structs) instead.
* Avoid file-scope mutable state. Keep runtime and test-control state inside owning objects (`DvzCanvas`, `DvzCanvasSwapchain`, etc.) so behavior stays instance-scoped and test-safe.

### 🧱 **C File Organization**

* Structure every `*.c` implementation like the Vulkan modules (`src/vk/*`, `src/vklite/*`) so the separators, sections, and spacing are immediately recognizable.
* Use delimiter blocks for each section, e.g.

  ```
  /*************************************************************************************************/
  /*  Section Name                                                                               */
  /*************************************************************************************************/
  ```

  Keep the section title left-aligned inside the middle comment line and repeat the delimiter before and after blocks.
* Organize the sections in the same order every file: `Includes`, `Constants`, `Macros`, `Typedefs`, `Structs`, `Function prototypes`/`Helpers`, and finally `Functions`. If a section is empty, omit it rather than leaving a placeholder.
* Lines should stay within 100 characters whenever possible to keep the delimiters readable and to match the Vulkan-style formatting.
* Maintain a visual gap of three blank lines (three empty lines) between every top-level definition—structs, enums, or functions—to keep C source spacing consistent across the codebase.

These rules should be followed carefully whenever Codex edits or creates C source files.

### 🛡️ **C Robustness, Safety, and Undefined Behavior Avoidance**

When editing C code, treat robustness and undefined behavior avoidance as first-class requirements:

* Treat compiler warnings as defects. Prefer fixing warnings instead of suppressing them, and do not
  introduce new warnings in touched code.
* Do not rely on signed integer overflow, out-of-bounds pointer arithmetic, uninitialized storage,
  use-after-free, use-after-destroy, or strict-aliasing-sensitive casts.
* Check size arithmetic before allocation, indexing, byte copies, and row/stride calculations. Validate
  `count * sizeof(T)`, byte offsets, dimensions, pitches, and downcasts from `size_t`/`uint64_t` to
  narrower integer types.
* Do not dereference possibly NULL pointers. Use existing `ANN`, `ASSERT`, and explicit runtime checks
  according to whether the condition is an internal invariant or a recoverable runtime failure.
* Do not pass function calls, increments, assignments, or other side-effectful/nontrivial expressions
  directly to `ANN()`, `ASSERT()`, or `DVZ_ASSUME()`. Evaluate the expression once into a local variable
  first, then assert/assume on that variable; Clang ignores `__builtin_assume()` expressions with
  potential side effects and emits `-Wassume`.
* In `examples/c`, keep `EXAMPLE_CHECK()` conditions simple and side-effect free. Do not call
  mutating/status-returning functions directly inside `EXAMPLE_CHECK()`, and do not combine multiple
  setup calls with `&&` inside a single check. Evaluate each call into a local `int rc` or `bool ok`
  first, then check that result with a specific failure message. Object/factory calls should still be
  assigned first and checked with simple pointer comparisons such as `EXAMPLE_CHECK(image != NULL, ...)`.
* Make ownership explicit. Every pointer or Vulkan handle should be clearly owned or borrowed by the
  current object. Destroy/free paths must be idempotent, set pointers to `NULL`, set Vulkan handles to
  `VK_NULL_HANDLE`, and never destroy borrowed handles.
* Avoid retaining pointers into growable arrays, registries, or object tables across calls that may append,
  destroy, compact, grow, or reallocate them. Reacquire by stable id or index after any such mutation.
* Keep runtime and test-control state instance-scoped. Avoid file-scope mutable state and reset test hooks
  in fixture/object lifecycle helpers.
* Use assertions for violated internal invariants, not for expected runtime failures. Public or recoverable
  failure paths should return an error/status and clean up partially initialized objects.
* Clean up every partial-initialization failure path. Do not leave half-created objects in lookup tables
  unless they are marked destroyed/unusable and future lookups cannot accidentally use them.
* Add focused regression coverage for lifetime, bounds, ownership, and multi-frame bugs. Prefer tests that
  fail before the fix.

Vulkan-specific C safety rules:

* Distinguish owned and borrowed Vulkan handles explicitly. Borrowed swapchain/canvas image views,
  command buffers, semaphores, and images must not be destroyed, begun, ended, reset, or submitted by a
  subsystem that does not own that lifecycle.
* A borrowed frame command buffer is usually already recording. Wrap it when recording into it, but do not
  call `vkBeginCommandBuffer`/`vkEndCommandBuffer` unless the API contract gives ownership of recording.
* Track command-buffer recording state at the owner level when possible, especially across canvas,
  stream, DRP2, and vklite boundaries.
* Image transitions require a live image wrapper/handle and a known previous layout. Never transition a
  destroyed object, a borrowed handle with unknown ownership, or an object whose image wrapper is NULL.
* Long-running live paths should not accumulate transient per-frame runtime objects indefinitely. Destroy
  or recycle transient command/render-pass objects once their command stream has completed.
* Run Vulkan validation-layer smoke tests for changes touching `vk`, `vklite`, `canvas`, `scene`, `drp2`,
  command buffers, frame lifetimes, render targets, swapchains, or synchronization.

### 🔎 **Static and Dynamic Analysis Guidance**

Codex may and should run analysis tools when they are available and relevant to the touched code:

* Always run `git diff --check` before finalizing code changes.
* For routine C changes, run the narrowest relevant validation loop, for example `just build` plus
  `just test <module-or-filter>`.
* For nontrivial C changes touching allocation, byte sizes, pointer lifetimes, object tables, Vulkan
  resources, command buffers, frame lifetimes, or synchronization, prefer at least one static or dynamic
  analysis pass when practical.
* Useful static checks include `clang-tidy` on touched files, `scan-build`/Clang Static Analyzer for
  broader C changes, and `cppcheck --enable=warning,style,performance,portability` as a secondary pass.
* Useful dynamic checks include ASan/UBSan builds for CPU-heavy paths, Valgrind for CPU-only tests when
  allocator noise is manageable, Vulkan validation layers for graphics paths, and representative live
  smoke loops such as `dvz_live_canvas --frames 300`.
* If an analysis tool is unavailable, too noisy for the active subsystem, or impractical for the current
  environment, report that explicitly and fall back to focused tests and runtime validation.
* When investigating scene -> DRP2 -> app churn, descriptor pressure, or unexpected runtime object
  creation, enable the DRP2 app stream trace before guessing: use `DVZ_DRP2_TRACE=full` for every
  emitted app frame, or `DVZ_DRP2_TRACE=1` / `normal` to print full details only when the normalized
  stream changes. Add `DVZ_DRP2_TRACE_COLOR=0` or `NO_COLOR=1` when capturing logs. Remember that
  this app trace covers app-frame streams; request-only readback/probe streams may need their own
  focused logging or tests.

## 🚧 Refactor Roadmap Guidance

- **Active graphics stack (`vk`, `vklite`, `canvas`, `stream`, `video`, `window`):** prioritize robustness,
  resource-lifetime correctness, performance, and clear public/internal boundaries. Preserve immediate
  presentation paths for high-FPS benchmarking, and profile carefully before accepting slower live-loop
  behavior. Treat this stack as the foundation for higher layers.
- **Active scene/DRP2 stack (`scene`, `drp2`):** prioritize a small, tested vertical slice over broad API
  growth. Scene should emit frame plans and DRP2 streams; the native runtime should execute through
  `vklite` and borrowed canvas frames without scene owning swapchains, command-buffer lifecycle, or sinks.
- **Cross-module helpers:** if multiple modules need the same low-level utility, move it to `src/common`
  instead of duplicating it.
- **Scaffolding modules (`color`, `wasm`, text/gui, and broader renderer/client layers):** keep untouched
  unless explicitly asked to activate them; when activated, follow the standard pattern (OBJECT lib + public
  headers + tests + integration in `src/CMakeLists.txt` and `testing/dvztest.c`).

---

## 🚀 **For Codex and Automation Agents**

When generating or editing code:

1. **Follow module boundaries.**

   * Never mix logic from different `src/<module>/` directories.
   * Cross-module utilities must go into `src/common/`.

2. **Include properly.**

   * Internal code: `#include "_alloc.h"`
   * Public code: `#include "datoviz/math.h"`

3. **CMake:**

   * Glob sources with `file(GLOB ... "*.c*")` so C and C++ files are captured.
   * Always add `${PROJECT_SOURCE_DIR}/include` and `${PROJECT_SOURCE_DIR}/src/common` as PUBLIC include dirs; bolt on `${PROJECT_SOURCE_DIR}/external/...` paths when you use vendored deps (e.g. cglm).
   * Build modules with `add_library(datoviz_<name> OBJECT ...)` and link them into `datoviz` from `src/CMakeLists.txt`.
   * Prefer `${DVZ_COMPILE_DEFINITIONS}` for module target definitions; avoid introducing new ad-hoc compile-definition variables.

4. **Tests:**

   * For scene visual/shader work, read `spec/scene/implementation/VISUAL_SHADER_REFACTOR.md` first.
   * Run `just shader-abi-check` whenever changing `src/scene/glsl`, `src/scene/wgsl`, shader registry entries, visual pipeline bind/layout rules, or visual shader ABI docs.
   * Use the `testing.h` suite/item API (`TEST_SIMPLE`, `TEST`, `AT`, …).
   * Expose a `test_<module>(TstSuite* suite)` helper that appends your cases; invoke it from `dvztest` (no constructors).
   * Keep `dvztest` as the unified aggregation point, while using the narrower `dvztest_*` binaries for focused validation loops when available.
   * For intentionally failing/error-path checks, wrap the assertion in expected-error scope so
     known `log_error()` output is suppressed:
     `AT_EXPECTED_ERROR_STRICT(suite, (expr_that_should_fail))` (or
     `tst_expect_error_begin/end` manually).

5. **Naming:**

   * Prefix public symbols with `dvz_`.
   * Prefix internal helper functions with `_dvz_`.
   * File names are lowercase; internal headers/files start with `_`.

6. **Never install internal headers.**

   * Only install `include/datoviz/`.

7. **Use leading underscores carefully.**

   * Keep the existing `_dvz_*` internal naming convention, but avoid introducing reserved forms such as `__name` and `_NAME`.

8. **Collaboration preference (repo-wide).**

   * When a reusable behavior is needed, prefer improving/exposing the public API instead of adding a new private helper.
   * If a private helper is still needed, do not use the `dvz_` prefix (reserved for public API symbols) and avoid introducing new `_dvz_*` names.

9. **Treat vendored code as read-only by default.**

   * Do not modify files under `external/` unless the task explicitly asks for changes there.
   * Prefer fixing Datoviz-owned code (`src/`, `include/`, `testing/`, build wiring) instead of patching vendored dependencies.

10. **Test hooks and fault injection must be instance-scoped.**

   * Avoid global toggles for tests/fault injection; wire controls through the object under test.
   * Reset test-control flags in fixture lifecycle helpers to avoid cross-test leakage.

---

## ✅ **Checklist for Codex**

Before submitting a PR or automated refactor:

* [ ] New source in `src/<module>/` → uses `add_library(datoviz_<module> OBJECT ...)`
* [ ] Internal shared code → `src/common/_*.{c,h}`
* [ ] No new includes inside `include/datoviz/` unless public API
* [ ] `external/` unchanged unless explicitly requested in the task
* [ ] Test added under `src/<module>/tests/`
* [ ] Scene visual/shader changes follow `spec/scene/implementation/VISUAL_SHADER_REFACTOR.md`
* [ ] Scene shader ABI changes pass `just shader-abi-check`
* [ ] Builds with `cmake -B build && cmake --build build`
* [ ] Relevant validation passes (`just test` and/or the narrowest relevant `dvztest_*` target)
* [ ] C changes reviewed for UB, ownership, bounds, lifetime, and partial-failure cleanup risks
* [ ] Static/dynamic analysis considered for nontrivial C changes, with skipped tools noted
* [ ] Follows naming conventions (dvz_*, *dvz**, DVZ_*)
* [ ] Headers properly grouped and ordered (pragma once, consistent formatting)

---

### 🧩 Example: adding a new module

1. Create sources under `src/<module>/` (`<module>.c`, optional `_<module>_internal.h`).
2. Add `src/<module>/CMakeLists.txt`:

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
3. Add tests under `src/<module>/tests/` and expose a `test_<module>()` entry point.
4. Update `src/CMakeLists.txt` (`add_subdirectory(<module>)` + add `datoviz_<module>` to the
   appropriate component list).
5. Add public headers in `include/datoviz/<module>.h` and `include/datoviz/<module>/*.h` only if
   the API is public.

---

**This document defines the rules Codex must follow when generating, modifying, or extending Datoviz source code.**
If in doubt: internal code → `src/common/`, public API → `include/datoviz/`.
