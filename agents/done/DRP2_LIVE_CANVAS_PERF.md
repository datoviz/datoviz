# DRP2 Live Canvas Performance Findings

> **Status:** `HISTORICAL PERFORMANCE RECORD`
> **Measured on:** `2026-04-30`
> **Primary report:** `build/profiles/live-canvas-20260430-114526`
> **Post-fix report:** `build/profiles/live-canvas-20260430-115310`
> **Command:** `just profile-canvas-release --frames 10000`
> **Binary:** `build-profile/testing/dvz_live_canvas`

## Context

The live canvas benchmark originally showed that the direct clear path could reach roughly `10k+ fps`,
while the `scene-drp2` path was substantially slower. A profiling workflow was added so the comparison
can be repeated with a `RelWithDebInfo` build and Vulkan validation disabled:

```bash
just profile-canvas-release --frames 10000
```

Important caveat: before commit `2896e823`, `testing/dvz_live_canvas.c` hardcoded
`DVZ_INSTANCE_VALIDATION_FLAGS`, so `DVZ_USE_VALIDATION=OFF` did not actually disable Vulkan validation
for this binary. The useful baseline is the report generated after that fix:
`build/profiles/live-canvas-20260430-114526`.

The validation-free baseline report no longer samples `libVkLayer_khronos_validation.so`.
`libVkLayer_MESA_device_select.so` still appears, but that is environment/device-selection noise rather
than Khronos validation.

Commit `11625c84` fixed the largest measured issue by retiring submitted transient encoder/pass/command-
buffer semantic objects and searching runtime object tables newest-first. This prevents the persistent
semantic state from growing every frame and keeps hot object lookups near the newest entries.


## Benchmark Summary

Validation-free direct clear path:

```text
avg_ms: ~0.060-0.067 ms/frame
fps:    ~15k-17k
p50:    ~0.060-0.063 ms
p90:    ~0.096-0.103 ms
```

Validation-free `scene-drp2` path:

```text
avg_ms: ~0.173-0.181 ms/frame
fps:    ~5.5k-5.8k
p50:    ~0.116-0.124 ms
p90:    ~0.400-0.414 ms
```

Approximate DRP2 overhead:

```text
+0.11-0.12 ms/frame
~2.9x slower than direct clear
```

The absolute overhead is small for normal 60/120/240 Hz display budgets, but it is large for the
run-as-fast-as-possible immediate-present benchmark and should be treated as a real hot-path issue.

After commit `11625c84`, the same profiling command produced:

```text
clear avg_ms:      ~0.044-0.058 ms/frame
scene-drp2 avg_ms: ~0.044-0.046 ms/frame
scene-drp2 fps:    ~21.9k-22.6k
scene-drp2 p50:    ~0.030-0.032 ms
scene-drp2 p90:    ~0.068 ms
```

This makes the original DRP2 runtime lookup problem effectively resolved for the benchmark. The remaining
gap is small enough that the next optimization should be justified by another profile before implementation.


## Perf Counter Summary

`perf stat` confirms the overhead is CPU-side:

```text
clear task-clock:      ~0.579 s / 10k frames
scene-drp2 task-clock: ~1.969 s / 10k frames
extra CPU time:        ~1.39 s / 10k frames ~= 0.139 ms/frame
```

Instruction count:

```text
clear instructions:      ~3.38 B
scene-drp2 instructions: ~20.82 B
extra instructions:      ~17.45 B ~= 1.75 M extra instructions/frame
```

Cache behavior is the largest red flag:

```text
clear core L1-dcache-load-misses:      ~23 M
scene core L1-dcache-load-misses:      ~1.95 B
clear core L1 miss rate:               ~3.66%
scene core L1 miss rate:               ~62.44%
```

This strongly suggests pointer-heavy, cache-unfriendly runtime interpretation/lookup work in the DRP2
execution path.

Post-fix cache behavior is back near the direct clear path:

```text
post-fix scene core L1 miss rate: ~3.58%
```


## Hotspots

The validation-free `perf diff` and `perf report` make the dominant cost clear:

```text
_find_object        ~36.95%
_validate_command   ~16.40%
_open_pass          ~13.05%
```

Children view:

```text
_dvz_canvas_draw_scene_drp2  ~70.14%
dvz_drp2_runtime_execute     ~68.93%
_validate_command            ~66.50% children, ~16.40% self
_find_object                 ~37.00% children, ~36.95% self
_open_pass                   ~13.10% children, ~13.05% self
```

Interpretation: the bottleneck is not primarily Vulkan submission. The cost is DRP2 runtime
interpretation/semantic validation/object lookup in the per-frame draw path.

Post-fix, `_find_object`, `_validate_command`, and `_open_pass` no longer dominate. The remaining profile
is diffuse and includes frame-plan/stream emission and shaderc/glslang samples at low percentages.


## Optimization Ideas

### Completed

1. Retire submitted transient semantic objects after queue submission.
   - Implemented in commit `11625c84`.
   - Prevents per-frame accumulation of encoder/pass/command-buffer semantic objects.
   - Allows repeated transient ids in runtime execution without growing the semantic table.

2. Search runtime object tables newest-first.
   - Implemented in commit `11625c84`.
   - Keeps hot transient lookup cheap even when persistent resources exist.

### Deferred

1. Move semantic validation out of the per-frame hot loop for unchanged streams.
   - Validate the stream when it is built or updated.
   - Store a validated/prepared representation and execute that directly every frame.
   - Keep a debug/test mode that can still force per-frame validation when needed.

2. Replace repeated `_find_object` scans/lookups with an indexed table.
   - DRP2 object ids should map to runtime objects in O(1) where possible.
   - Avoid pointer chasing through generic object collections in the draw loop.
   - If ids are dense enough, use an id-indexed array; otherwise use a compact hash/index table built
     during preparation.

3. Compile the DRP2 command stream into an execution plan.
   - Resolve command object ids to stable runtime object pointers or indices once.
   - Precompute command handlers and backend-specific state needed by execution.
   - Make per-frame execution a tight linear pass over prepared commands.

4. Specialize common render-pass operations.
   - `_open_pass` is a visible hotspot; cache render-pass/open-state data needed for the canvas target.
   - Avoid recomputing attachment/rendering metadata when the canvas target and pass config are unchanged.

5. Split validation and execution APIs explicitly.
   - Example shape: `prepare/validate -> executable plan -> execute`.
   - Make it hard for the live path to accidentally call the full interpreter/validator each frame.
   - Preserve the current interpreter path for fixture tests and negative/error-path coverage.

6. Reduce per-frame diagnostics/report setup.
   - `dvz_diagnostic_report_init` is small in the current profile, but avoid initializing diagnostics on
     every hot execution unless an error path needs it.

7. Check shader/pipeline setup caching.
   - `libshaderc_shared` appears at low percentages in the validation-free profile. It is not the main
     bottleneck, but shader compilation or reflection should not happen in steady-state frame execution.
   - Confirm whether those samples are one-time startup/report noise or repeated per-frame work.


## Next Interesting Optimization

The next interesting optimization is cached frame-plan/DRP2 stream emission, but it is explicitly deferred
for now.

The live path still rebuilds a frame plan and emits a DRP2 stream every frame:

```text
dvz_frame_plan(...)
dvz_frame_plan_upload(...)
dvz_frame_plan_render(...)
dvz_frame_plan_render_visual(...)
dvz_frame_plan_emitter_emit_drp2(...)
dvz_drp2_runtime_execute(...)
```

A narrow experiment would cache the emitted DRP2 stream for this benchmark and keep only the borrowed
canvas frame target dynamic. A general prepared-plan API would be more complex because it needs explicit
ownership, invalidation, diagnostics, dynamic target handling, and resource lifetime tests.

Recommended policy:

1. Do not implement the general prepared-plan optimization immediately.
2. If future profiles show frame-plan/stream emission as a meaningful bottleneck, first prototype cached
   emission locally in the live canvas or scene path.
3. Promote it to an internal prepared execution API only if the local prototype produces a measurable win.
4. Expose a public prepared-plan API only after multiple callers need it and the invalidation semantics are
   clear.


## Related Memory Optimization

The next important memory optimization is to lazy-load heavyweight optional dynamic libraries such as
CUDA/NVENC/NVCUVID and shaderc. Plain canvas and DRP2 rendering should not load video encoder, decoder, or
runtime shader compiler libraries unless those features are requested. See
`agents/now/OPTIONAL_DYNAMIC_DEPENDENCIES.md`.


## Suggested Next Profiling Slices

1. Add a DRP2 runtime mode that skips semantic validation after a successful prepare step, then rerun:

   ```bash
   just profile-canvas-release --frames 10000
   ```

2. Add temporary counters around `_find_object`, `_validate_command`, and `_open_pass` to confirm call
   counts per frame and identify command types responsible for the most lookups.

3. Compare three draw modes if available or easy to add:
   - direct clear,
   - DRP2 runtime with validation disabled/cached,
   - DRP2 prepared execution plan.

4. Keep validation-free profiling separate from validation-layer smoke testing. Validation layers are useful
   for correctness, but they obscure the hot-path CPU profile.


## Current Conclusion

The original DRP2 live canvas slowdown was a CPU-side runtime-state growth and lookup problem, and it was
addressed by commit `11625c84`. Prepared frame-plan/stream emission remains an interesting future
optimization, but it should stay deferred until a fresh profile shows that the remaining emission work is
worth the added API and invalidation complexity.
