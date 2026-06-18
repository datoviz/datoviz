# Dead-Code And Public-Surface Disposition Audit

Status: read-only audit, 2026-06-18.

Scope: v0.3-era utility headers and related implementations under `ds`, `thread`, `common/mutex`,
and `math/array`, plus umbrella exposure through `datoviz.h`, `ds.h`, `thread.h`, and `dvzmath.h`.

Method: direct source inspection and `rg` reference checks across `include`, `src`, `testing`,
`examples`, `tools`, `docs`, `spec`, and `agents`, excluding existing generated/build output only
where noted. No code, build, generated docs, or submodule state was changed for this audit.


## Summary

The active v0.4 scene/app/runtime path does not need the installed `ds` API, the legacy
`DvzArray` API, or the `DvzFifo`/`DvzDeq` queue API as public surface. `src/common/mutex.c`,
`src/thread/atomic.cpp`, and `src/thread/thread.c` still support active internal implementation
paths, but their installed headers expose platform or callback-shaped utility APIs that should not
look like ordinary v0.4 user APIs.

The most important distinction is public-surface exposure versus internal utility use:

1. `datoviz.h` does not include `ds.h` or `thread.h`, but it does include `dvzmath.h`, which includes
   `math/array.h`.
2. The raw `ctypes` generator filters callable functions by `DVZ_EXPORT`; these utility functions
   are not emitted as callable raw functions today.
3. The header/type extractor still records typedefs, enums, and layoutable records from installed
   headers, so `DvzArray`, `DvzDataType`, `DvzList`, `DvzDeq*`, `DvzFifo`, `DvzThread`, and related
   records leak into generated C reference/raw type namespaces.


## Disposition Table

| API family | Classification | Current reference evidence | Used outside own module/tests? | Included by top-level public umbrella? | Generated C reference/raw `ctypes` effect | Recommended RC1 action | Removal or privatization risk |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `include/datoviz/ds*.h`, `include/datoviz/ds/`, `src/ds/list.c`, `src/ds/map.cpp` | remove from build | Headers: `include/datoviz/ds.h:19` includes `ds/list.h`, `include/datoviz/ds.h:20` includes `ds/map.h`; declarations at `include/datoviz/ds/list.h:76`, `include/datoviz/ds/map.h:60`; implementations at `src/ds/list.c:49`, `src/ds/map.cpp:59`; tests at `src/ds/tests/test_list.c:32`, `src/ds/tests/test_map.c:32`; build inclusion at `src/ds/CMakeLists.txt:6` and `src/CMakeLists.txt:307`. | No. `rg` finds `dvz_list*` and `dvz_map*` only in their headers, implementations, and `src/ds/tests/*`, plus generated docs/types. | No. `include/datoviz/datoviz.h:19-31` omits `ds.h`; `include/datoviz.h:9` forwards to it. | Policy includes `datoviz/ds.h` at `spec/bindings/ctypes.yml:13`; extractor default includes it at `tools/bindings/extract_api.py:27`. No callable raw functions are emitted because declarations lack `DVZ_EXPORT`, but types leak: `docs/reference/c-api/types.md:7045`, `docs/reference/c-api/types.md:7069`, `datoviz/_ctypes.py:2436`, `datoviz/_ctypes.py:2448`. | Remove `ds.h` and `include/datoviz/ds/` from installed headers and binding policy; remove `datoviz_ds` from core components if no private consumer appears; delete or quarantine `src/ds` tests with the implementation removal. | Low for active v0.4 behavior; medium for source compatibility with any external v0.3 utility users. `map.cpp` removal also reduces a C++ object in core. |
| `include/datoviz/math/array.h`, `src/math/array.c`, `src/math/array_structs.h` | remove from build | Header is included by `include/datoviz/dvzmath.h:29`; `datoviz.h` includes `dvzmath.h` at `include/datoviz/datoviz.h:22`; declarations at `include/datoviz/math/array.h:109`, `include/datoviz/math/array.h:266`, `include/datoviz/math/array.h:304`; implementation at `src/math/array.c:211`; private struct at `src/math/array_structs.h:37`; tests at `src/math/tests/test_array.c:68`; build inclusion via glob at `src/math/CMakeLists.txt:5`. | No. `rg` finds `DvzArray`, `DvzDataType`, `DVZ_DTYPE_*`, and `dvz_array*` only in `math/array` headers/implementation/tests and generated docs/types. | Yes. `include/datoviz/datoviz.h:22` -> `include/datoviz/dvzmath.h:29` -> `include/datoviz/math/array.h`. | Policy includes `datoviz/dvzmath.h` at `spec/bindings/ctypes.yml:14`; extractor default includes it at `tools/bindings/extract_api.py:28`. No callable raw functions are emitted, but enum/type stubs leak: `docs/reference/c-api/types.md:6643`, `docs/reference/c-api/types.md:6787`, `datoviz/_ctypes.py:235`, `datoviz/_ctypes.py:414`, `datoviz/_ctypes.py:1848`. | First remove `math/array.h` from `dvzmath.h`; then remove `array.c` and `array_structs.h` from the math build if still unused. If any dtype enum is still desired, replace it with a narrow v0.4 format/type enum in the owning module, not the legacy array object. | Low for active runtime; high public-surface benefit because this is currently exposed through `<datoviz.h>`. Main risk is stale examples or external v0.3 code using `DvzArray`. |
| `include/datoviz/thread/fifo.h`, `DvzFifo`, `DvzDeq`, `src/thread/fifo.c`, `src/thread/fifo_structs.h` | remove from build | Header is included by `include/datoviz/thread.h:20`; declarations at `include/datoviz/thread/fifo.h:107`, `include/datoviz/thread/fifo.h:221`, `include/datoviz/thread/fifo.h:350`, `include/datoviz/thread/fifo.h:384`; implementation at `src/thread/fifo.c:36`, `src/thread/fifo.c:418`; private definitions at `src/thread/fifo_structs.h:29`, `src/thread/fifo_structs.h:60`; tests at `src/thread/tests/test_fifo.c:83`, `src/thread/tests/test_fifo.c:239`; build inclusion via glob at `src/thread/CMakeLists.txt:5`. | No active consumers. `rg` finds FIFO/Deq use only in `src/thread/fifo.c`, `src/thread/fifo_structs.h`, and `src/thread/tests/test_fifo.c`, plus binding policy/docs. | No through `<datoviz.h>`. `include/datoviz/datoviz.h:19-31` omits `thread.h`; focused include `datoviz/thread.h` exposes it. | Policy includes `datoviz/thread.h` at `spec/bindings/ctypes.yml:26` and names `dvz_deq_callback` at `spec/bindings/ctypes.yml:61`; generator has special callback handling at `tools/bindings/generate_ctypes.py:771`. No callable raw functions are emitted, but types leak: `docs/reference/c-api/types.md:6799`, `docs/reference/c-api/types.md:6901`, `datoviz/_ctypes.py:2008`, `datoviz/_ctypes.py:2132`, `datoviz/_ctypes.py:2968`. | Remove `fifo.h` from installed `thread.h` first. If no hidden private consumer appears, remove `fifo.c`/`fifo_structs.h` from the thread build and delete the Deq callback policy special-case. | Low for active v0.4 behavior; medium because `DvzDeqItem` is already an installed-header compile hazard and callback policy mentions it. |
| `include/datoviz/common/mutex.h`, `src/common/mutex.c` | active internal implementation | Header exposes pthread types at `include/datoviz/common/mutex.h:22`, `include/datoviz/common/mutex.h:35`, `include/datoviz/common/mutex.h:44`; declarations at `include/datoviz/common/mutex.h:59`, `include/datoviz/common/mutex.h:108`; implementation at `src/common/mutex.c:26`; active internal use in logging `src/common/log.c:233`, app post callbacks `src/app/app.c:138`, `src/app/app.c:847`, PRNG `src/math/prng.cpp:28`, and thread/FIFO internals `src/thread/thread.c:56`, `src/thread/fifo.c:54`. | Yes. Used by `src/common/log.c`, `src/app/app.c`, `src/math/prng.cpp`, and `src/thread/*`. | Not directly. `include/datoviz/common.h:19-21` omits it and `include/datoviz/datoviz.h:19` includes only `common.h`; it is exposed via focused `datoviz/thread/thread.h:26`. | Type docs leak `DvzMutex`/`DvzCond`: `docs/reference/c-api/types.md:6751`, `docs/reference/c-api/types.md:7075`. Raw callable functions are not emitted. | Keep source/build. Move the installed header to a private/internal header or replace the public typedefs with opaque handles before RC1. Prefer private internal header because no public scene/app API should expose mutex ownership. | Medium. Internal call sites are real and cross-module, so privatization needs include-path cleanup. Public removal risk is acceptable; pthread ABI leakage is a larger RC risk. |
| `include/datoviz/thread/atomic.h`, `src/thread/atomic.cpp` | active internal implementation | Header includes private allocator `_alloc.h` at `include/datoviz/thread/atomic.h:22`; atomic type/API macros at `include/datoviz/thread/atomic.h:38`, `include/datoviz/thread/atomic.h:47`, declarations/inline definitions at `include/datoviz/thread/atomic.h:67`, `include/datoviz/thread/atomic.h:84`; implementation at `src/thread/atomic.cpp:34`; internal use in FIFO `src/thread/fifo.c:48`, `src/thread/fifo.c:723` and thread wrapper `src/thread/thread.c:58`, `src/thread/thread.c:73`. | Yes, but only inside `src/thread/*`; no scene/app/runtime user calls it directly. | No through `<datoviz.h>`. Exposed by focused `include/datoviz/thread.h:19`. | `thread.h` is in binding policy at `spec/bindings/ctypes.yml:26`, but no raw callable atomic functions are emitted. The installed header still creates source-level/private-header leakage. | Keep only if `DvzThread` or private FIFO remains. Move to private `src/thread` or `src/common` internals; remove `_alloc.h` from any installed include path. | Low to medium. C11/C++ split and private allocator include make public retention risky; internal replacement is straightforward. |
| `include/datoviz/thread/thread.h`, `src/thread/thread.c` | active internal implementation | Header exposes callback type and thread API at `include/datoviz/thread/thread.h:35`, `include/datoviz/thread/thread.h:37`, `include/datoviz/thread/thread.h:56`; implementation at `src/thread/thread.c:33`, `src/thread/thread.c:48`; active video use at `src/video/encoder_backend_kvazaar.c:539`, `src/video/encoder_backend_kvazaar.c:559`, `src/video/encoder_backend_kvazaar.c:568`; tests at `src/thread/tests/test_thread.c:45`; build inclusion via `src/thread/CMakeLists.txt:6`. | Yes. Used by `src/video/encoder_backend_kvazaar.c` outside `src/thread`. | No through `<datoviz.h>`. Exposed by focused `include/datoviz/thread.h:21`. | Binding policy includes `datoviz/thread.h` at `spec/bindings/ctypes.yml:26` and treats `dvz_thread` as a global callback at `spec/bindings/ctypes.yml:63`; raw callable function is not emitted, but types leak: `docs/reference/c-api/types.md:7261`, `datoviz/_ctypes.py:2800`, `datoviz/_ctypes.py:2992`. | Keep implementation for video or replace with a private worker helper. Remove from installed headers and binding policy unless Datoviz explicitly wants a public cross-platform thread API, which would need exported functions and ownership docs. | Medium. Active video dependency means build removal is unsafe today; public privatization risk is mostly external utility compatibility. |


## Prioritized Findings

1. `math/array.h` is the highest-priority public-surface leak because it is included by
   `<datoviz.h>` through `dvzmath.h`, yet `rg` shows no active consumer outside its own
   implementation/tests. This makes `DvzArray`, `DvzDataType`, and `DVZ_DTYPE_*` look like part of
   the v0.4 scene/app API.
2. `ds` is built into `datoviz_core` but has no active references outside `src/ds` tests. It should
   be treated as a removable v0.3 utility module, not an advanced public API.
3. `DvzFifo`/`DvzDeq` are not active runtime API and have known installed-header hazards:
   value-returning `DvzDeqItem` is declared while the struct definition lives in
   `src/thread/fifo_structs.h`. They also leave stale callback policy in raw-binding generation.
4. `common/mutex.h` is active internally but public-hostile: it exposes `pthread_mutex_t` and
   `pthread_cond_t` in installed headers. This should be private or opaque before RC1.
5. `thread/atomic.h` and `thread/thread.h` are implementation helpers. `thread/thread.h` has one
   active external module user in the video encoder, but neither header should remain installed as
   public API unless Datoviz commits to a documented threading utility surface.


## Suggested Cleanup Sequence

1. Remove `math/array.h` from `dvzmath.h`; regenerate API metadata/docs/raw ctypes and confirm
   `DvzArray`, `DvzDataType`, `DvzArrayCopyType`, and `DVZ_DTYPE_*` disappear from generated public
   surfaces.
2. Remove `datoviz/ds.h` from binding policy and installed public inventory; then remove
   `datoviz_ds` from `DVZ_CORE_COMPONENTS` if a full-tree build confirms no private references.
3. Split `thread.h`: keep any needed private implementation headers under `src/thread` or
   `src/common`, and remove `fifo.h`/`atomic.h`/`thread.h` from installed public headers unless an
   explicit advanced threading API is approved.
4. Remove `fifo.c`/`fifo_structs.h` and Deq callback binding policy if no private consumer appears
   after the header split.
5. Privatize `common/mutex.h` or replace it with opaque public handles; update internal includes in
   `app`, `common/log`, `math/prng`, and `thread`.
6. Regenerate C reference/raw ctypes and run the public header probes. The expected public-surface
   delta is removal of utility records/enums/types, not removal of active scene/app/runtime
   functions.


## Validation Notes

No build or generated-output commands were run for the audit itself. Before implementing the cleanup,
the narrow validation should be:

```sh
just build
just ctypes
just ctypes-check
git diff --check
```

For each removal commit, also run the relevant focused C tests before deleting tests, then run the
public header probes and distribution header smoke because this is installed-surface work.
