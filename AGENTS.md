# 🧠 **AGENTS.md – Datoviz v0.4-dev Architecture & Refactor Guide**

## 📘 Overview

This document provides essential rules and conventions for Codex (and other automated agents) to understand, modify, and extend the **Datoviz v0.4-dev** codebase.

Datoviz is a **modular C scientific visualization library**, currently being **refactored for version 0.4** with a clean, maintainable, and testable architecture.

The goals:

* One shared library: **`libdatoviz.so`**
* Modular **object libraries** per component
* Clear boundary between **public API** and **internal code**
* Unified **test runner** for all modules
* CMake-based build and test system

When refactoring, do NOT delete existing comments, keep them and update them if needed, but do not delete them.

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
│   │   ├── _alloc.h _assert.h _error.h _log.h _macros.h _mutex.h _obj.h _time_utils.h
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
│   ├── empty.c                 # Keeps the shared library non-empty
│   └── CMakeLists.txt          # Collects object modules into libdatoviz
│       (additional module folders exist but are currently placeholders)
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

Only `common`, `ds`, `fileio`, `math`, and `thread` currently build and link into `libdatoviz.so`; other module directories are scaffolding that should be kept untouched unless explicitly revived.

---

## ⚙️ **Build System (CMake)**

* Active modules (`src/common`, `src/ds`, `src/fileio`, `src/math`, `src/thread`) build as **OBJECT libraries** that glob both C and C++ sources:

  ```cmake
  file(GLOB MATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
  add_library(datoviz_math OBJECT ${MATH_SRC})

  target_include_directories(datoviz_math
      PUBLIC
          ${PROJECT_SOURCE_DIR}/include
          ${PROJECT_SOURCE_DIR}/src/common
          ${PROJECT_SOURCE_DIR}/external/cglm/include
  )

  target_compile_definitions(datoviz_math PUBLIC ${COMPILE_DEFINITIONS})
  ```

* `src/common/CMakeLists.txt` publishes its directory with `INTERFACE` usage requirements so any consumer of `datoviz_common` can include `_alloc.h`, `_macros.h`, etc.

* The root `src/CMakeLists.txt` assembles the shared library and registers the active modules:

  ```cmake
  add_library(datoviz SHARED empty.c)

  add_subdirectory(common)
  add_subdirectory(ds)
  add_subdirectory(fileio)
  add_subdirectory(math)
  add_subdirectory(thread)

  target_link_libraries(datoviz
      PRIVATE
          datoviz_common
          datoviz_ds
          datoviz_fileio
          datoviz_math
          datoviz_thread
  )

  target_include_directories(datoviz
      PUBLIC
          ${PROJECT_SOURCE_DIR}/include
  )
  ```

`COMPILE_DEFINITIONS` is assembled in `src/CMakeLists.txt` (OS/compiler switches, `LOG_USE_COLOR`, `ENABLE_VALIDATION_LAYERS`, `DEBUG`) and applied `PUBLIC` to every module and to `dvztest`.

## 🛠️ **Build & Test Commands**

The top-level `justfile` provides the primary workflow; Codex should stick to:

* `just clean` — remove generated build artifacts so the next build starts fresh.
* `just build` — configure (if needed) and compile the active targets through CMake.
* `just test` — execute the unified `dvztest` suite after a successful build.

Run these commands from the repository root. Other `just` recipes exist but are currently out of scope.

### Public vs Internal Includes

| Type               | Include Path                 | Example                             | Notes                                                        |
| ------------------ | ---------------------------- | ----------------------------------- | ------------------------------------------------------------ |
| Public aggregator  | `include/datoviz/math.h`     | `#include "datoviz/math.h"`         | Re-exports `math/*.h`, mirrors other modules                 |
| Public subheader   | `include/datoviz/math/vec.h` | `#include "datoviz/math/vec.h"`     | Most subheaders depend on `_macros.h` from `src/common`      |
| Shared internals   | `src/common/_alloc.h`        | `#include "_alloc.h"`               | Available because modules/tests add `${PROJECT_SOURCE_DIR}/src/common` |
| Module source file | `src/math/vec.c`             | `#include "datoviz/math/vec.h"`     | Implementations rely on public headers                       |
| Test helpers       | `src/common/tests/test_common.h` | `#include "testing.h"`           | Each module keeps optional `tests/` headers alongside sources |

Headers are installed under `/usr/include/datoviz/`. They currently expect `_macros.h` (and friends) to be reachable through the build tree; keep `${PROJECT_SOURCE_DIR}/src/common` on include paths until the public/private split is revisited.

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

Filenames starting with `_` are **legal**, but **identifiers** (macros, functions, variables) starting with `_` are **not** — avoid defining `_alloc()` or `_ALLOC_H_` inside code.

---

## 🧩 **Testing System**

### Structure

* All test sources live under `src/<module>/tests/*.c*`.
* Each module provides an entry-point (for example `int test_common(TstSuite* suite)`) that appends its test cases to the shared `TstSuite`.
* Assertions use the helpers in `testing.h` (`AT`, `AC`, `ACn`, `TEST_SIMPLE`, etc.).
* A single runner executable, **`dvztest`**, builds the suite, runs optional filters, and prints colourful summaries.

### Example

```c
// src/common/tests/test_obj.c
#include "_assert.h"
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
#include "testing.h"

int main(int argc, char** argv)
{
    TstSuite suite = tst_suite();

    test_common(&suite);

    const char* filter = (argc >= 2) ? argv[1] : NULL;
    tst_suite_run(&suite, filter);
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
target_link_libraries(dvztest PRIVATE datoviz_common testing)
target_include_directories(dvztest PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/common
)
target_compile_definitions(dvztest PRIVATE ${COMPILE_DEFINITIONS})
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
* `_assert.h` — assertion helpers (`ANN`, `ASSERT`, etc.)
* `_error.h` — error callbacks and reporting buffer
* `_log.h` — logging API
* `_macros.h` — global macros (`EXTERN_C_*`, `DVZ_EXPORT`, `MUTE_ON`, …)
* `_mutex.h` — pthread/tinycthread wrappers
* `_obj.h` — lifecycle helpers for `DvzObject`
* `_time_utils.h` and `_env.h` — misc utilities shared across modules

They are not installed, so keep `src/common` in include paths whenever you touch headers that depend on them.

---

## 🧠 **Design Philosophy**

* **Single shared library** (`libdatoviz.so`)
* **Explicit modular dependencies**
* **Consistent naming and include structure**
* **Public headers live in `include/datoviz/`; shared `_*.h` stay in `src/common` and remain reachable via include dirs**
* **One unified test executable** (`dvztest`)
* **Public headers cleanly grouped by module**

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
   * Build modules with `add_library(datoviz_<name> OBJECT ...)` and link them into `libdatoviz.so` from `src/CMakeLists.txt`.

4. **Tests:**

   * Use the `testing.h` suite/item API (`TEST_SIMPLE`, `TEST`, `AT`, …).
   * Expose a `test_<module>(TstSuite* suite)` helper that appends your cases; invoke it from `dvztest` (no constructors).
   * Keep everything in the single `dvztest` executable—no per-module runners.

5. **Naming:**

   * Prefix public symbols with `dvz_`.
   * Prefix internal helper functions with `_dvz_`.
   * File names are lowercase; internal headers/files start with `_`.

6. **Never install internal headers.**

   * Only install `include/datoviz/`.

7. **Don’t define identifiers starting with `_`.**

   * Filenames can start with `_`, but identifiers (macros/functions) cannot.

---

## ✅ **Checklist for Codex**

Before submitting a PR or automated refactor:

* [ ] New source in `src/<module>/` → uses `add_library(datoviz_<module> OBJECT ...)`
* [ ] Internal shared code → `src/common/_*.{c,h}`
* [ ] No new includes inside `include/datoviz/` unless public API
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

   target_compile_definitions(datoviz_geom PUBLIC ${COMPILE_DEFINITIONS})
   ```
3. Add tests under `src/geom/tests/` and expose `int test_geom(TstSuite* suite)`.
4. Update `src/CMakeLists.txt` (`add_subdirectory(geom)` + link `datoviz_geom` into `datoviz`).
5. Add public headers in `include/datoviz/geom.h` and `include/datoviz/geom/*.h` if the API is public.

---

**This document defines the rules Codex must follow when generating, modifying, or extending Datoviz source code.**
If in doubt: internal code → `src/common/`, public API → `include/datoviz/`.
