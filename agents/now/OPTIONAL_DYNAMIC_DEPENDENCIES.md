# Optional Dynamic Dependencies

Status: implementation complete (2026-04-30).

## Motivation

The small `dvz_live_canvas` benchmark currently links and loads useful but optional libraries even when
the benchmark only opens a canvas and draws `scene-drp2`.

The current profile build links at least:

```text
libcuda.so.1
libnvidia-encode.so.1
libnvcuvid.so.1
libshaderc_shared.so.1
```

This is too much baseline dependency surface for a lightweight canvas app. These libraries are valuable
for video encoding, CUDA interop/decode, and runtime shader compilation, but they should not be loaded
unless the process actually uses those features.

## Desired Policy

Optional heavyweight dynamic libraries should be loaded lazily.

1. The default canvas/rendering path should not load video encoder, CUDA, decoder, or shader compiler
   libraries.
2. Libraries should be loaded on first use of the corresponding feature, for example:
   - creating an NVENC encoder,
   - creating a CUDA-backed video/decode path,
   - compiling GLSL at runtime with shaderc.
3. Add an explicit startup option or environment variable for applications that prefer eager loading.
   This keeps failure reporting deterministic for apps that want to validate all optional backends at
   startup.
4. Keep lightweight API/registry layers available without forcing the concrete backend libraries into
   every process.

## Priority

1. Video backends: `libcuda`, `libnvidia-encode`, and `libnvcuvid`.
2. Runtime shader compiler: `libshaderc_shared`.
3. Future GUI or app-layer backends such as Qt or ImGui should stay outside the base library unless the
   corresponding feature is enabled.

Do not spend effort lazy-loading small or core dependencies such as `cglm`, tiny threading helpers, or
Vulkan itself in graphics builds.

## Implementation Strategy

### Cross-platform dynload abstraction

A single small header `src/common/_dynload.h` wraps `dlopen`/`dlsym`/`dlclose` (Linux/macOS)
and `LoadLibrary`/`GetProcAddress`/`FreeLibrary` (Windows) behind three macros:

```c
dvz_dynlib_open(path)    // returns DvzDynLib handle (NULL on failure)
dvz_dynlib_sym(h, name)  // returns void* function pointer (NULL on failure)
dvz_dynlib_close(h)      // releases the handle
```

Each lazy-loader calls `dvz_dynlib_open` once behind a `static bool loaded` guard. Subsequent
calls skip the load entirely. The guard is set only when all required symbols resolve
successfully; a missing symbol leaves the backend unavailable and logs a clear error.

### shaderc (libshaderc_shared)

- **Symbols needed:** ~13 (`shaderc_compiler_initialize`, `shaderc_compile_into_spv`,
  `shaderc_result_get_bytes`, etc.)
- **Path:** bundled in the repo at `libs/shaderc/{linux,windows,macos_*}/`. CMake passes the
  resolved absolute path as `DVZ_SHADERC_LIB_PATH` compile definition, so `dvz_dynlib_open`
  gets an exact path — no search, no ambiguity.
- **Header:** keep `#include "shaderc/shaderc.h"` for type definitions; only the link-time
  symbol reference is removed.
- **CMake change:** remove `target_link_libraries(datoviz_drp2 PUBLIC datoviz_shaderc)`.
- **Failure mode:** GLSL compilation returns an error; WGSL/SPIR-V paths are unaffected.

### CUDA driver (libcuda)

- **Scope:** Linux and Windows only — already `#ifdef DVZ_HAS_CUDA` gated; macOS excluded.
- **Symbols needed:** ~10 (`cuInit`, `cuDeviceGet`, `cuCtxCreate`, `cuCtxSetCurrent`,
  `cuCtxDestroy`, `cuMemAlloc`, `cuMemFree`, `cuMemcpyHtoD`, `cuMemcpyDtoH`, plus
  `cuImportExternalMemory`-family for Vulkan interop).
- **Library name:** `"libcuda.so.1"` on Linux, `"nvcuda.dll"` on Windows.
- **Pattern:** static `_cuda_syms` struct of function pointers, filled by `_cuda_load()` on
  first encoder creation. `CU_CHECK` macro updated to go through the pointer table.
- **CMake change:** remove `target_link_libraries(datoviz_video PUBLIC CUDA::cuda_driver)`.
- **Side effect:** `libnvcuvid.so.1` disappears from `ldd` automatically — it was a transitive
  dep of `libcuda.so.1` at link time, not a direct Datoviz dependency.

### NVENC (libnvidia-encode)

- **Scope:** Linux and Windows, `DVZ_HAS_CUDA` gated.
- **Symbols needed:** 1 (`NvEncodeAPICreateInstance`). Everything else already goes through the
  `g_nvenc` function-pointer table that `NvEncodeAPICreateInstance` fills.
- **Library name:** `"libnvidia-encode.so.1"` on Linux, `"nvEncodeAPI64.dll"` on Windows.
- **CMake change:** remove `DVZ_NVENC_LIB` from `target_link_libraries`.
- **Failure mode:** encoder creation returns an error; the stub backend activates.

### Performance

`dlopen` on a warm page cache takes < 1 ms. The one-time load is buried inside encoder
creation (hundreds of ms of device setup) or first GLSL compile (50–200 ms of shader
compilation). No frame-path overhead after the first call.

### Architecture Sketch (original, for reference)

Keep the public Datoviz API stable around lightweight capability objects and backend registries.

For video:

1. `datoviz_video` should provide the generic encoder API, capability query API, and sink integration.
2. Concrete backends such as NVENC, CUDA decode/import, and kvazaar should be separate backend units.
3. Backend units should either be:
   - built as optional shared modules loaded with `dlopen`/`dlsym`, or
   - compiled into the process but only linked when an explicit build option asks for that backend.
4. Missing libraries should make the backend unavailable, not prevent a plain canvas app from starting.

For shader compilation:

1. Prefer precompiled SPIR-V or cached shader modules in normal render paths.
2. Load shaderc only when an API call asks to compile GLSL at runtime.
3. Preserve a clear error if runtime compilation is requested but the compiler library is unavailable.

## Memory Measurements

### Before lazy loading (2026-04-30, eager deps)

Report: `build/profiles/live-canvas-memory-20260430-120309`

```text
first_vmrss_kb:            10800
peak_vmrss_or_vmhwm_kb:   232012
last_vmhwm_kb:             230868
steady drift:              ~8.3 MB/min (stable window ~50 s)
```

### After lazy loading (2026-04-30)

Report: `build/profiles/live-canvas-memory-20260430-172727`

`ldd` confirms `libcuda.so.1`, `libnvidia-encode.so.1`, `libnvcuvid.so.1`, and `libshaderc_shared.so`
are absent from the binary's link-time dependencies.

```text
first_vmrss_kb:            6312   (−4.5 MB vs before)
peak_vmrss_or_vmhwm_kb:   227228  (−4.8 MB vs before)
last_vmhwm_kb:             227228
steady drift:              ~7.4 MB/min (stable window ~65 s)
```

Benchmark result:

```text
frames=1000000
elapsed=65.656330s
fps=15227.78
avg_ms=0.0657
p50=0.0576
p90=0.1110
p95=0.1243
p99=0.2282
max=11.3866
stutters >2ms=74 >5ms=6 >10ms=1 >16.67ms=0
```

### Interpretation

The original pre-implementation estimate of 60–110 MB savings was wrong. Actual peak RSS savings
are ~4.8 MB. The bulk of resident memory is the NVIDIA Vulkan driver stack (vkspirv, rtcore,
allocator, gpucomp, glcore), which is always loaded for a Vulkan canvas and is unaffected by
removing the video/shader compiler libraries from the link.

The slow RSS drift (~7–8 MB/min) is also unchanged. The drift was already traced (via Massif,
Valgrind, and pmap) to a growing private anonymous mapping near NVIDIA driver pages, not to
Datoviz heap allocations or to the video/compiler libraries. The lazy-load work does not fix
this drift and was never expected to.

The drift investigation conclusion stands:

1. The drift is almost certainly NVIDIA driver-internal (JIT cache, shader cache, or similar).
2. No Datoviz-owned definite leak was found.
3. A long-run memory gate (RSS/PSS slope threshold) remains a future goal.

## Leak/Drift Investigation Notes

Follow-up runs on `2026-04-30` compared `scene-drp2` against `clear`.

Clear path report:

```text
build/profiles/live-canvas-memory-20260430-121158
```

Command:

```text
./build-profile/testing/dvz_live_canvas --benchmark --frames 1000000 --draw clear
```

Result:

```text
frames=1000000
elapsed=63.383946s
fps=15773.71
avg_ms=0.0634
peak_vmrss_or_vmhwm_kb=173824
```

The clear path shows nearly the same slow RSS drift as `scene-drp2`:

```text
clear stable window:     +7700 KB over ~62.5 s, ~7.22 MB/min
scene-drp2 stable window: +7152 KB over ~50.4 s, ~8.32 MB/min
```

This strongly suggests the drift is below DRP2/scene emission, because it happens even when the app only
clears and presents.

Valgrind leak check:

```text
build/profiles/valgrind-live-canvas-20260430/clear-leak.log
```

The only definite leak reported was:

```text
184 bytes direct, 1,809 bytes indirect, from libdbus
```

No Datoviz-owned definite leak was visible in that small `clear` run. The rest of the Valgrind output was
dominated by NVIDIA/GLX uninitialized-value noise.

Massif:

```text
build/profiles/valgrind-live-canvas-20260430/clear-massif.out
```

Massif showed ordinary heap usage plateauing around:

```text
peak heap: ~20.78 MB
last heap after teardown: ~0.81 MB
```

This means the RSS drift is probably not normal malloc/free heap accumulation in Datoviz.

`smaps_rollup` and `pmap`:

```text
build/profiles/valgrind-live-canvas-20260430/clear-smaps-rollup.tsv
build/profiles/valgrind-live-canvas-20260430/clear-pmap2-5s.txt
build/profiles/valgrind-live-canvas-20260430/clear-pmap2-45s.txt
```

The steady drift from 5s to 60s was private anonymous memory:

```text
Private_Dirty: +6728 KB
Anonymous:     +6728 KB
Shared_Clean:  +12 KB
```

The 5s-to-45s `pmap` comparison pointed at one anonymous mapping:

```text
000072e0c06d3000 drss=5828 KB ddirty=5828 KB rw--- [ anon ]
```

That mapping sits near NVIDIA Vulkan/GL driver mappings (`libnvidia-rtcore`,
`libnvidia-glvkspirv`, `libnvidia-allocator`, `nvidiactl`, `libnvidia-gpucomp`,
`libnvidia-glcore`). This is not proof that the driver owns the growth, but it makes a Datoviz heap leak
less likely than driver/runtime anonymous cache growth.

Policy conclusion:

1. Treat long-run bounded memory as a Datoviz requirement even if the growth comes from the GPU driver.
2. Keep the `clear` path as the baseline memory guard; if `clear` drifts, investigate canvas/Vulkan/window
   or driver behavior before DRP2/scene.
3. Add a future long-run memory gate that samples RSS/PSS/private-dirty and fails only on sustained
   post-warmup growth above a configured slope.
4. After optional dynamic dependencies are made lazy, rerun the same tests. Removing eager video/compiler
   libraries should make the remaining driver/runtime footprint easier to interpret.
