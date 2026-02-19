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

### 🏗️ Current refactor status (v0.4-dev)

* ✅ Active modules currently linked into `libdatoviz` are: `common`, `ds`, `fileio`, `math`, `thread`,
  `input`, `window`, `canvas`, `stream`, `video`, `vk`, and `vklite`.
* 🚧 Vulkan-related modules (`vk`, `vklite`, `canvas`, `stream`, `video`) are still evolving quickly;
  expect frequent API/internal adjustments in these directories.
* ⏭️ Several other directories/headers remain scaffolding (for example `color`, `wasm`, and higher-level
  renderer/scene/client layers); keep them untouched unless explicitly requested.

---

## 🧩 **Project Structure**

**Important**: never go inside `v0.3` subfolder, it is the old version. Focus on `include`, `src`, `testing`, and supporting roots such as `external/`.

```
datoviz/
├── include/                    # Public headers (installed)
│   └── datoviz/
│       ├── datoviz.h           # Umbrella header
│       ├── axes.h, color.h, common.h, ds.h, fileio.h, math.h, renderer.h, visuals.h, vk.h ...
│       ├── axes/…, canvas/…, color/…, common/…, ds/…, math/…, thread/… # Sub-headers per module
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

Modules currently compiled into `libdatoviz`: **`common`, `ds`, `fileio`, `math`, `thread`, `input`,
`window`, `canvas`, `stream`, `video`, `vk`, `vklite`**. Modules such as `color`/`wasm` are currently
not linked and should remain untouched unless explicitly requested.

### ⏩ Planned activation order

1. Stabilize currently active modules (`vk`, `vklite`, `canvas`, `stream`, `video`, plus window/input integration).
2. Keep non-activated modules as scaffolding unless a task explicitly brings one online.
3. Continue staged bring-up of higher-level systems (renderer/scene/client layers) only when requested.

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
  add_subdirectory(math)
  add_subdirectory(thread)
  add_subdirectory(input)
  add_subdirectory(window)
  add_subdirectory(canvas)
  add_subdirectory(stream)
  add_subdirectory(video)
  add_subdirectory(vk)
  add_subdirectory(vklite)

  target_link_libraries(datoviz
      PRIVATE
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
          datoviz_volk
  )

  target_include_directories(datoviz
      PUBLIC
          ${PROJECT_SOURCE_DIR}/include
  )
  ```

`DVZ_COMPILE_DEFINITIONS` is assembled in `src/CMakeLists.txt` (OS/compiler switches, `LOG_USE_COLOR`,
`ENABLE_VALIDATION_LAYERS`, `DEBUG`, `VK_NO_PROTOTYPES`, feature flags), exported through a global property,
and then applied to registered targets in the top-level `CMakeLists.txt` (`src` modules and testing targets).
Some existing module `CMakeLists.txt` files still reference `${COMPILE_DEFINITIONS}` as legacy wiring;
do not copy that pattern in new code—use `${DVZ_COMPILE_DEFINITIONS}` instead.

## 🛠️ **Build & Test Commands**

The top-level `justfile` provides the primary workflow; Codex should stick to:

* `just clean` — remove generated build artifacts so the next build starts fresh.
* `just build` — configure (if needed) and compile the active targets through CMake.
* `just test [filter]` — execute the unified `dvztest` suite after a successful build (platform recipe).

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
* A single runner executable, **`dvztest`**, builds the suite, runs optional filters, prints summaries,
  and now lists failing test names explicitly when failures occur.

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
#include "../src/common/tests/test_common.h"
#include "../src/ds/tests/test_ds.h"
#include "../src/fileio/tests/test_fileio.h"
#include "../src/math/tests/test_math.h"
#include "../src/stream/tests/test_stream.h"
#include "../src/thread/tests/test_thread.h"
#include "../src/input/tests/test_input.h"
#include "../src/window/tests/test_window.h"
#include "../src/canvas/tests/test_canvas.h"
#if DVZ_HAS_CUDA
#include "../src/video/tests/test_video.h"
#endif
#include "../src/vk/tests/test_vk.h"
#include "../src/vklite/tests/test_vklite.h"
#include "testing.h"

int main(int argc, char** argv)
{
    TstSuite suite = tst_suite();

    test_common(&suite);
    test_ds(&suite);
    test_fileio(&suite);
    test_math(&suite);
    test_stream(&suite);
    test_thread(&suite);
    test_input(&suite);
    test_window(&suite);
    test_canvas(&suite);
#if DVZ_HAS_CUDA
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

`enable_testing()` in the root `CMakeLists.txt` is currently commented out, so invoke `./dvztest [filter]` directly unless you re-enable CTest.

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
* **Roadmap discipline** — stabilize active modules first; keep non-activated modules as scaffolding until
  explicitly requested.

---

## 🧾 **Coding Conventions**

* Document every new module-level function with a short Doxygen-style docstring immediately above the definition. Start the block with `/**`, provide a one-sentence summary, leave a blank line, then add `@param` tags for every argument and an `@return` tag whenever the function returns a value (plus `@note`/`@see` when helpful). Use a single space between the leading `*` and tag identifiers (e.g., ` * @param ...`), avoid extra spacing, keep the description line within the project's preferred width, and treat it as the canonical explanation of the symbol's behavior.
* Visually separate neighbouring top-level functions with three newline characters (two empty lines) to make each implementation stand out. Attach any descriptive comments directly above the function and keep extra text out of the blank space.
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
* Maintain a visual gap of two blank lines (three newline characters) between every top-level definition—structs, enums, or functions—to match the spacing seen in the Vulkan files.

These rules should be followed carefully whenever Codex edits or creates C source files.

## 🚧 Refactor Roadmap Guidance

- **Active graphics stack (`vk`, `vklite`, `canvas`, `stream`, `video`, `window`):** prioritize robustness,
  resource-lifetime correctness, and clear public/internal boundaries.
- **Cross-module helpers:** if multiple modules need the same low-level utility, move it to `src/common`
  instead of duplicating it.
- **Scaffolding modules (`color`, `wasm`, and higher-level renderer/scene/client layers):** keep untouched
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

   * Use the `testing.h` suite/item API (`TEST_SIMPLE`, `TEST`, `AT`, …).
   * Expose a `test_<module>(TstSuite* suite)` helper that appends your cases; invoke it from `dvztest` (no constructors).
   * Keep everything in the single `dvztest` executable—no per-module runners.
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
* [ ] Builds with `cmake -B build && cmake --build build`
* [ ] `./dvztest [filter]` passes (re-enable `enable_testing()` if you want `ctest`)
* [ ] Follows naming conventions (dvz_*, *dvz**, DVZ_*)
* [ ] Headers properly grouped and ordered (pragma once, consistent formatting)

---

### 🧩 Example: adding a new module “geometry”

1. Create sources under `src/geom/` (`geom.c`, `geom.cpp`, optional `_geom_internal.h`).
2. Add `src/geom/CMakeLists.txt`:

   ```cmake
   file(GLOB GEOM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
   add_library(datoviz_geom OBJECT ${GEOM_SRC})

   target_include_directories(datoviz_geom
       PUBLIC
           ${PROJECT_SOURCE_DIR}/include
           ${PROJECT_SOURCE_DIR}/src/common
   )

   target_compile_definitions(datoviz_geom PUBLIC ${DVZ_COMPILE_DEFINITIONS})
   ```
3. Add tests under `src/geom/tests/` and expose `int test_geom(TstSuite* suite)`.
4. Update `src/CMakeLists.txt` (`add_subdirectory(geom)` + link `datoviz_geom` into `datoviz`).
5. Add public headers in `include/datoviz/geom.h` and `include/datoviz/geom/*.h` if the API is public.

---

**This document defines the rules Codex must follow when generating, modifying, or extending Datoviz source code.**
If in doubt: internal code → `src/common/`, public API → `include/datoviz/`.
