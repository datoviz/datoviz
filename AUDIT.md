## Datoviz v0.4-dev Code Quality Audit

This document captures the issues observed while reviewing **include/**, **src/**, **testing/**, and the top-level **CMakeLists.txt**. Each section references the path that first triggered the finding.

### Public API Boundaries

1. **Private headers leaked to users** — `include/datoviz/thread/atomic.h:22` includes `_alloc.h` and uses `ANN()`/`dvz_calloc()`, which ship only inside `src/common`. External consumers cannot compile against this header. Either expose allocator helpers via a public header (e.g., `datoviz/common/alloc.h`) or keep the atomic API internal until it is self-contained.
2. **Platform-specific types in public headers** — `include/datoviz/common/mutex.h:34` and `include/datoviz/thread/thread.h:30` expose raw pthread types and include `<pthread.h>`. On Windows the build relies on PThreads4W, but client code won’t have that header unless they add it themselves. Consider opaque structs or a portable tinycthread wrapper to keep public headers OS-agnostic.
3. **Missing include in `_log.h`** — `src/common/_log.h:33` calls `strrchr()` without including `<string.h>`, so strict builds that include `_log.h` before standard headers break with implicit-declaration warnings. Add the missing include in the header itself.
4. **Umbrella header exposes unfinished modules** — `include/datoviz/datoviz.h:23-35` re-exports axes, drp, scene*, etc., although none of these modules build in v0.4. Users calling into those APIs will link against a shared library that lacks the implementations. Gate unfinished modules behind a feature macro or remove them from the umbrella header until their object libraries return.

### Core Modules

1. **Integer truncation in list implementation** — `src/ds/list.c:62-105` uses a `uint64_t` for `DvzList.count` but loops with `uint32_t`. Once the list grows past 2^32‑1 entries, insertion/removal loops overflow and corrupt memory. Use `size_t`/`uint64_t` consistently.
2. **Unsafe file I/O** — `src/fileio/fileio.cpp:19-57` (`dvz_file_size` and `dvz_read_file`) rely on `long` + `ftell()` and ignore `fread()` return values. Files larger than 2 GB overflow, and partial reads go unnoticed. Prefer `fseeko`/`ftello` and check every IO call.
3. **Leaks on error path** — `src/fileio/fileio.cpp:86-167` (`dvz_read_npy`) jumps to the `error:` label without closing the FILE*, leaking descriptors. It also leaves the optional `size` output untouched. Unify cleanup to close the file and reset outputs before returning `NULL`.
4. **Dead gzip support** — `src/fileio/fileio.cpp:185-258` wraps `dvz_read_gz()` in `#if HAS_ZLIB`, but `src/CMakeLists.txt:182-200` unconditionally sets `HAS_ZLIB=0` and there is no option to enable it. As written, the gzip loader always logs “Datoviz was not built with zlib support.” Either detect zlib via CMake or drop the API until compression is actually supported.
5. **Missing headers for syscall** — `src/common/log.c:72-180` calls `syscall(__NR_gettid)` on Linux without including `<sys/syscall.h>`, which is undefined on stricter toolchains. Add the include and consider caching the thread id to avoid a syscall per log line.
6. **Thread creation error handling** — `src/thread/thread.c:34-68` returns a `DvzThread*` even if `pthread_create()` or `dvz_mutex_init()` fail, leaving partially initialised objects that later crash. Abort creation (return `NULL` or assert) when either step fails, and extend the implementation to handle the Windows threading backend.
7. **FIFO “max capacity” conflict** — `src/thread/fifo.c:38-115` enforces `capacity <= DVZ_MAX_FIFO_CAPACITY` (256) even though `_fifo_resize()` doubles the buffer. When enqueueing beyond 255 entries the assert fires. Remove the hard cap or make it configurable per FIFO instance.
8. **Error callback can’t be cleared** — `src/common/error.c:24-33` forces a non-null argument in `dvz_error_callback()`, so there is no supported way to unregister the hook. Allow passing `NULL` to reset the callback and add tests to confirm `dvz_assert()` dispatches it.

### Testing & QA

1. **Missing coverage for exposed APIs** — `testing/dvztest.c:17-29` only wires common/ds/fileio/math/thread/vk suites. There are no tests for allocator switching (`dvz_use_mimalloc_allocator`), logging levels, gzip/PPM readers, or Windows stubs, even though those APIs are public. Add minimal regression tests now before more modules land.
2. **Timing-based thread tests** — `src/thread/tests/test_thread.c:24-66` relies on fixed `dvz_sleep()` delays to coordinate threads. On slow CI machines these values can be exceeded, causing false negatives. Sync using condition variables or atomics instead of wall-clock sleeps.
3. **Test harness reporting** — `testing/testing.cpp:68-170` groups tests but only keeps a running sum of failures; it does not report skipped/filtered counts or tag failing tests in the summary. Also, it always runs every matching test even when one fails, which makes triage harder in CI. Consider logging failing test names immediately and returning non-zero as soon as a standalone test fails.
4. **Sanitizers only instrument part of the tree** — `src/CMakeLists.txt:338-348` pushes sanitizer flags into `datoviz`, `datoviz_common`, `datoviz_ds`, `datoviz_fileio`, `datoviz_math`, and `datoviz_thread`, but omits `datoviz_vk`, `dvztest`, and helper libs. As a result, running the project under ASan still misses large swaths of code. Reuse the `DVZ_ALL_TARGETS` list to apply sanitizers uniformly.

### Build & Tooling

1. **Global include directories** — `CMakeLists.txt:34` still calls `include_directories(${PROJECT_SOURCE_DIR}/include)`, leaking Datoviz headers into every third-party target compiled after it (e.g., mimalloc). Prefer `target_include_directories` on the specific Datoviz targets.
2. **Undefined `ENABLE_VALIDATION_LAYERS` default** — `src/CMakeLists.txt:182-224` writes `ENABLE_VALIDATION_LAYERS=${ENABLE_VALIDATION_LAYERS}` into `DVZ_COMPILE_DEFINITIONS`, but the variable is never set in v0.4. This expands to `-DENABLE_VALIDATION_LAYERS=` which many compilers treat as “true”. Provide an explicit default (0) so `#if ENABLE_VALIDATION_LAYERS` behaves deterministically.
3. **Source globbing without `CONFIGURE_DEPENDS`** — Every module (`src/common/CMakeLists.txt`, `src/ds/CMakeLists.txt`, etc.) uses `file(GLOB … "*.c*")` without `CONFIGURE_DEPENDS`. Adding files later will not trigger a build-system refresh unless the user re-runs CMake manually. Add `CONFIGURE_DEPENDS` or list sources explicitly.
4. **Stale comments/naming** — `src/thread/CMakeLists.txt:1` still carries “File I/O” from a copy/paste. Cleaning these up now will reduce confusion once more modules are enabled.

### Open Questions / Follow-Up

1. Should `datoviz/thread` stay public, or can it move under `src/common` to stop leaking private headers while the API stabilises?
2. Do you plan to support gzip/PNG decoding in v0.4 via zlib or keep the old custom readers? Deciding now will guide whether `HAS_ZLIB` becomes a real CMake option.
3. Is Windows still meant to be Tier‑1? If so, now is the right time to hide pthread types from the public ABI or standardise on a portable threading abstraction before more modules depend on them.

Let me know if you’d like help addressing any of these issues or drafting patches/tests.
