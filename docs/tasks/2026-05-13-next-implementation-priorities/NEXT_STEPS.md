# Next Implementation Priorities

This file expands the six priority lanes from `agents/now/V0_4_NEXT_STEPS.md` into concrete
implementation briefs.


## 1. Native 3D / Manual Smoke Slice

Goal: keep one strong native 3D smoke example that pressures mesh or lit primitive rendering through
the current scene -> DRP2 -> app path.

Current status:

1. `examples/c/visuals/mesh.c` already covers the intended native 3D example shape:
   `dvz_mesh()`, indexed cube geometry, per-face normals/colours, depth testing, perspective camera,
   `dvz_panel_set_arcball()` through `dvz_app_window_input()`, app resize synchronization, and
   `dvz_app_window_set_frame_callback()` for continuous motion.
2. Do not add a near-duplicate `hello_3d_arcball_depth_glfw.c` unless the example surface is being
   deliberately reorganized.
3. The remaining deliverable is to make capture/readback part of the smoke path, either by extending
   `visuals/mesh` with a small bounded-frame capture option or by documenting a paired capture
   command that proves the same rendered path produces nonblank pixels.

Files to inspect first:

1. `examples/c/visuals/mesh.c`,
2. `examples/c/visuals/mesh.c`,
3. `examples/c/visuals/point.c`,
4. `examples/c/techniques/pick_hover.c`,
5. `src/scene/tests/app.c` depth and app offscreen tests,
6. `src/app/app.c`.

Validation:

1. `just build`,
2. a focused scene/app test that already covers depth, for example
   `just test test_app_offscreen_lit_primitive_depth_orders_overlap`,
3. manual run of `./build/examples/c/visuals/mesh`,
4. manual resize, arcball drag, and capture check,
5. `git diff --check`.

Exit criteria:

1. arcball interaction works without breaking the retained scene stream,
2. depth ordering is visibly correct,
3. resize does not desynchronize panel coordinates or picking coordinates,
4. capture produces a nonblank image,
5. the example remains small enough to be a smoke test, not a demo framework.


## 2. Narrow WebGPU / DRP2 Feasibility Spike

Goal: test whether the current DRP2 subset can be replayed over WebGPU before the visual surface grows
too large.

Non-goals:

1. do not fork scene semantics,
2. do not build a full browser runtime,
3. do not add public WebGPU APIs yet,
4. do not widen DRP2 only for speculative browser features.

First deliverable:

1. make a DRP2 command compatibility matrix for WebGPU:
   `CreateBuffer`, `WriteBuffer`, `CreateTexture`, `WriteTexture`, `CreateShaderModule`,
   `CreateRenderPipeline`, `BeginRenderPass`, `SetViewport`, `SetScissor`, `SetVertexBuffer`,
   `SetIndexBuffer`, `SetBindGroup`, `Draw`, `DrawIndexed`, `CopyTextureToBuffer`, `QueueSubmit`,
   and readback replies,
2. replay a hard-coded or fixture-derived clear-only pass,
3. replay one primitive triangle/point fixture,
4. replay one image sampling fixture,
5. add readback for one small texture/buffer result,
6. add depth only after color/image replay works.

Suggested location:

1. start under `tools/` or `examples/webgpu/` as an experiment,
2. keep it isolated from `src/scene` and `src/drp2` until the contract gaps are clear,
3. reuse committed DRP2 JSON fixtures where possible.

Files/specs to inspect first:

1. `agents/later/DRP2_WEBGPU_ROADMAP.md`,
2. `spec/drp2/COMMANDS.md`,
3. `spec/drp2/fixtures/positive/scene_static_render_from_c.json`,
4. `spec/drp2/fixtures/positive/scene_texture_sampling_from_c.json`,
5. `spec/drp2/fixtures/positive/render_pass_pipeline_fixed_function_state.json`.

Validation:

1. document each unsupported command or semantic mismatch,
2. keep any browser smoke runnable with one command or README snippet,
3. add no new DRP2 command until a fixture can demonstrate the need,
4. update `spec/drp2/` only when the feasibility spike exposes a real contract problem.

Exit criteria:

1. clear, primitive draw, image sampling, readback, and depth are either replayed or have explicit
   documented blockers,
2. shader-language assumptions are documented,
3. no native runtime behavior is duplicated into scene code.


## 3. Targeted Hygiene / Static-Analysis Pass

Goal: improve safety of the hot path touched by recent scene, request, DRP2 runtime, and app commits.

Current status:

1. DRP2 texture-layout and scene image-probe plan byte-size overflow checks are done.
2. Borrowed frame-target depth attachments are retired through deferred destruction keyed by the
   borrowed command buffer.
3. Consumed scene pick/probe result slots are cleared after polling.
4. Scene test warning readiness was tightened for the render-pass-scope test and one direct
   `memset()` use.
5. DRP2 vklite backend object tables now trim destroyed tail slots after transient pass cleanup,
   explicit backend destroys, and deferred borrowed-frame retirement setup.
6. Latest focused validation recorded on `2026-05-14`: `just test drp2` (`83/83`) and
   `just test scene` (`143/143`).

Scope first:

1. `src/scene/scene.c`,
2. `src/scene/pick_probe.c`,
3. `src/scene/converter.c`,
4. `src/drp2/runtime.c`,
5. `src/drp2/stream.c`,
6. `src/app/app.c`,
7. `src/app/status.c`,
8. `src/app/trace.c`.

Checklist:

1. Done: verify size arithmetic before allocations, byte copies, texture rows, and readback
   offsets for the recent DRP2 texture and image-probe slices.
2. Done for the current borrowed-depth slice: verify every partial-initialization failure path frees
   or marks objects unusable.
3. Done for current borrowed frame targets: ensure borrowed frame targets and command buffers are
   never destroyed or ended by non-owners.
4. Done for vklite object tables: inspect transient DRP2/vklite runtime objects for per-frame
   accumulation.
5. Done for request queues/results: inspect request freshness, coalescing, and stale-result rejection
   for cross-panel leakage.
6. Remaining: check that app frame callbacks cannot mutate scene state while an emitted stream is
   live.
7. Remaining: check trace/status code for raw-struct hashing, padding dependence, and string-buffer
   bounds.
8. Continue adding focused regression tests before changing semantics.

Tools to try when available:

1. `git diff --check`,
2. `just build`,
3. narrow `just test scene`, `just test drp2`, and `just test app` filters,
4. `clang-tidy` on touched C files if compile commands are available,
5. `cppcheck --enable=warning,style,performance,portability` on touched owned files,
6. ASan/UBSan debug build for CPU-heavy request/trace paths when practical.

Exit criteria:

1. no known new warnings in touched code,
2. no obvious ownership or bounds issue remains in the recent request/readback/app trace slices,
3. any deferred issue is documented in a task record with a reproduction or code reference.


## 4. Manual Interactive Smoke Coverage

Goal: turn current interactive behavior into repeatable manual checks before adding many new features.

Current status:

1. the manual smoke matrix now lives in
   [../../architecture/manual_scene_smoke.md](/home/cyrille/GIT/Viz/datoviz/docs/architecture/manual_scene_smoke.md),
2. it lists the command, expected behavior, automated coverage, and known gaps for the current
   examples and focused test-only paths,
3. screenshots or captures remain optional unless they reveal a regression.

Smoke cases:

1. `techniques/pick_hover`: hover point picking, panel-coordinate mapping, stale hover suppression,
2. image probe example or new variant: probe four quadrants of a non-uniform image,
3. panzoom point scene: drag, wheel zoom, double-click reset,
4. arcball 3D scene from priority lane 1: rotate, resize, capture,
5. multi-panel scene: verify viewport/scissor isolation and per-panel controllers,
6. partial texture update scene: verify only the updated region changes across frames,
7. app trace/status: run normal trace mode and verify unchanged frames stay compact.

Validation:

1. run focused automated tests before manual checks,
2. record OS/GPU/backend for any manual anomaly,
3. add automated regression tests for deterministic failures discovered manually,
4. avoid leaving debug tracing in examples after investigation.

Exit criteria:

1. a future agent can reproduce the current interactive behavior without chat context,
2. manual checks cover both interaction and app lifecycle,
3. failures discovered manually are converted into focused automated tests where practical.


## 5. Rendered Text / Colorbars / Annotations

Goal: turn retained semantic objects into visible scene contributions without leaking atlas or backend
details into the public scene API.

Ordering:

1. render colorbar gradient first if possible, because it can reuse image/primitive infrastructure,
2. implement the smallest text rendering path needed for labels and ticks,
3. render colorbar ticks/labels after text works,
4. render simple annotation labels/callouts after text and fixed/controller placement rules are clear,
5. defer rich text shaping, font fallback, and advanced annotation layouts.

Files/specs to inspect first:

1. `include/datoviz/scene/text.h`,
2. `include/datoviz/scene/annotation.h`,
3. `include/datoviz/scene/scale.h`,
4. `spec/scene/semantics/TEXT.md`,
5. `spec/scene/semantics/LEGENDS_AND_COLORBARS.md`,
6. `spec/scene/semantics/ANNOTATIONS.md`,
7. `spec/scene/proposals/TEXT_DESIGN.md`,
8. `spec/scene/proposals/COLORBAR_COLORMAP_DESIGN.md`,
9. `spec/scene/proposals/ANNOTATION_TEXT_SCALE_API.md`.

Implementation constraints:

1. keep text/font/colorbar/annotation objects scene-owned,
2. lower rendering through frame plans and DRP2 emission,
3. do not expose glyph atlas pages, glyph UVs, Vulkan handles, or DRP2 ids in public structs,
4. keep panel-fixed overlays separate from controller-applied world objects,
5. add tests for retained object lifetime and emitted render contributions before broadening API.

Validation:

1. focused scene tests for bookkeeping and emitted contribution shape,
2. one offscreen capture test for visible colorbar/text pixels,
3. `just test scene`,
4. manual GLFW check only after offscreen checks pass.

Exit criteria:

1. a simple colorbar can be rendered from an existing scale/colormap,
2. a simple text label can be rendered at a stable panel position,
3. a simple annotation label can render without owning backend resources directly.


## 6. Selective Visual-Family Expansion

Goal: add visual families only when they pressure a real architectural need and can be tested through
the active scene -> DRP2 -> runtime path.

Selection rules:

1. prefer families that reuse existing DRP2/runtime capabilities,
2. prefer families that expose WebGPU portability problems early,
3. prefer families that improve manual smoke coverage or scientific utility,
4. avoid adding a family that requires a broad new renderer subsystem unless explicitly chosen.

Likely candidates after the first five lanes:

1. `segment` or richer `path`, to pressure grouped resources and per-group picking,
2. `marker`, to pressure per-item shape parameters and constant/per-item attribute sources,
3. mesh variants, to pressure normals, lighting/material state, and semantic region ids,
4. sampled-field/image variants, to pressure formats, colormaps, and probe payloads.

Defer for now unless explicitly prioritized:

1. volume rendering,
2. transparency-heavy families,
3. advanced text/glyph families beyond lane 5,
4. broad renderer/client abstractions outside the active scene/DRP2 path.

Per-family acceptance checklist:

1. public API added or confirmed,
2. spec contract checked under `spec/scene/visuals/`,
3. frame-plan contribution implemented,
4. DRP2 emission implemented,
5. native runtime execution tested,
6. one C example added,
7. focused scene tests added,
8. WebGPU feasibility implications noted if relevant.
