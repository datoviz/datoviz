# Optional Dynamic Dependencies

Status: important near-term memory optimization, not started.

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

## Architecture Sketch

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

## Memory Estimate

The current 1M-frame `scene-drp2` memory run reached about 232 MB peak RSS/HWM. A large part of that is
driver/runtime startup cost, not Datoviz scene data.

Rough expected baseline after removing eager optional loads:

```text
current peak RSS/HWM:          ~230-232 MB
expected lean canvas RSS:      ~120-170 MB
aggressive best-case target:   ~90-130 MB
```

This is an estimate, not a measured result. The reason for the wide range is that RSS includes driver
page mappings, loader behavior, shader/compiler runtime state, C++ runtime pages, and GPU-driver caches.
The first measurement to confirm this should be a profile build where CUDA/NVENC/NVCUVID are disabled or
lazy, followed by:

```bash
ldd build-profile/testing/dvz_live_canvas
just memory-canvas-release --frames 1000000 --draw scene-drp2
```

Success means the NVIDIA video/decode libraries disappear from `ldd`, and peak RSS drops materially
without changing the benchmark behavior.

## Latest Memory Profile

Report:

```text
build/profiles/live-canvas-memory-20260430-120309
```

Command:

```text
./build-profile/testing/dvz_live_canvas --benchmark --frames 1000000 --draw scene-drp2
```

Benchmark result:

```text
frames=1000000
elapsed=55.423837s
fps=18039.17
avg_ms=0.0554
p50=0.0482
p90=0.1019
p95=0.1262
p99=0.1328
max=33.5523
stutters >2ms=71 >5ms=13 >10ms=11 >16.67ms=3
```

Memory summary:

```text
first_vmrss_kb: 10800
last_vmrss_kb: 152668
peak_vmrss_or_vmhwm_kb: 232012
last_vmhwm_kb: 230868
```

Important interpretation:

1. Startup dominates the RSS jump:
   - ~10.8 MB at process start,
   - ~140.5 MB after 0.21 s,
   - ~224.2 MB after 0.42 s.
2. The steady run then rises from ~224.2 MB to ~232.0 MB over about 55 seconds.
3. That steady rise is about 7.8 MB across almost 1M frames, or roughly 8 bytes per frame if it is truly
   frame-linear.
4. The final RSS sample drops to ~152.7 MB while HWM stays high, which is probably process teardown and
   should not be treated as the steady-state memory level.

This does not show a catastrophic per-frame accumulation. It does show enough slow drift to justify a
follow-up leak/cache investigation after the optional-library baseline is fixed. The most useful next
memory test is to rerun the same 1M-frame benchmark after CUDA/NVENC/NVCUVID and shaderc are made lazy or
build-disabled for the plain canvas path.
