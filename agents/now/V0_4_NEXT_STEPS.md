# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT GUIDE`
> - **Updated on:** `2026-05-15`
> - **Purpose:** give future agents the practical next steps after the first scene -> DRP2 ->
>   vklite/canvas slice.


## Current Position

The low-level stack is the foundation:

1. `vk` owns low-level Vulkan instance/device/queue/memory primitives.
2. `vklite` owns higher-level Vulkan wrappers.
3. `canvas` owns frame acquisition, borrowed frame command buffers, swapchain/offscreen targets,
   and stream submission.
4. `stream` and sinks route frames to swapchain, offscreen, live image, and video consumers.

The active higher layer exists:

1. `drp2` owns backend-agnostic command streams, JSON/debug serialization, validation, and the
   native vklite runtime.
2. `scene` owns early scene graph objects, capability snapshots, diagnostic reports, frame plans,
   DRP2 emission, and request/result state; `app` owns the small presentation loop over scene,
   canvas, and the DRP2 runtime.
3. Built-in visual families currently implemented are `point`, `primitive`, `mesh`, path-as-line/strip,
   and `image`.
4. Panel controllers are live: panzoom and arcball feed per-panel transforms.
5. Per-panel viewport/scissor and offscreen multi-panel preservation work through the emitted DRP2
   path.
6. Retained sampled fields, scales, scene buffers, and primitive/mesh shading uniforms now share the
   scene -> frame-plan -> DRP2 binding path.
7. Interaction bookkeeping, queued pick/probe requests, result polling, selection/link objects,
   pinned readouts, and text/annotation retained objects now have first source implementations and
   focused bookkeeping tests.
8. GPU-backed request execution is narrow but real: point picking and image probing execute through
   auxiliary DRP2 streams and runtime readbacks after the main figure frame has populated runtime
   resources.
9. `app` is an active presentation module. Recent work added frame callbacks, compact/full DRP2
   trace output, default-on trace/logger colors with `NO_COLOR` support, combined FPS/status
   reporting, and figure-size synchronization before frame emission.
10. Scene runtime DRP2 emission now prepares resources before opening command encoders/render
    passes. Render-pass scopes should contain only pass-local state and draw commands.
11. The first DRP2-level DVZR recording/replay slice is active: incremental timestamped recording
    writes `manifest.json`, `stream.jsonl`, and external payload blobs; the initial portable JSON
    command subset covers hello, buffer/texture creation, and buffer/texture writes; unsupported
    commands still fall back to ABI-local raw command blobs. Loaded recordings expose indexed frame
    records, owned per-frame command streams, and runtime-level linear replay helpers.

Recent focused DVZR validation on 2026-05-15: clean detached `dvztest_drp2
drp2_recording_linear_roundtrip` passed, and full clean detached `dvztest_drp2 drp2` passed
`77/77` after the portable-command, frame-indexing, frame-stream, and runtime replay-helper
commits.

Follow-up DVZR slice on 2026-05-15: scene-emitted point, primitive, mesh, and image streams now
share one raw-free portable DVZR regression test, covering semantic replay after recording load.
The developer executable `build/testing/dvz_drp2_player` now opens a `.dvzr` recording and replays
it frame-by-frame through the semantic DRP2 runtime in default paced mode or `--fast` mode. Focused
validation: `just build`, `./build/testing/dvztest_drp2 drp2_recording` (`3/3`),
`./build/testing/dvztest_scene test_frame_plan_emit_scene_core_visuals_record_portable_dvzr`, and
`./build/testing/dvz_drp2_player --fast /tmp/dvz_scene_mesh_emit_portable.dvzr`.

Second follow-up DVZR slice on 2026-05-15: loaded recordings now expose raw fallback diagnostics via
`dvz_drp2_recording_raw_fallback_count()` and `dvz_drp2_recording_raw_fallback()`. The focused
regression records a valid stream with a deliberately unsupported `DestroyBuffer` portable command,
verifies the fallback command index/type, and confirms `dvz_drp2_player` warns while still replaying
the recording through the semantic runtime. Focused validation:
`./build/testing/dvztest_drp2 drp2_recording` (`4/4`) and
`./build/testing/dvz_drp2_player --fast /tmp/dvz_drp2_recording_raw_fallback.dvzr`.

Third follow-up DVZR slice on 2026-05-15: app-window recording is now available through
`dvz_app_window_record_start()` / `dvz_app_window_record_stop()`. The app draw path appends
successfully emitted scene DRP2 streams to a linear recorder and writes a one-time synthetic
playback target setup stream so borrowed canvas-target recordings can replay through the semantic
runtime. Focused validation: `just build`,
`./build/testing/dvztest_scene test_app_offscreen_records_dvzr_frames`,
`./build/testing/dvztest_scene test_frame_plan_emit_scene_core_visuals_record_portable_dvzr`,
`./build/testing/dvztest_drp2 drp2_recording` (`4/4`), and
`./build/testing/dvz_drp2_player --fast /tmp/dvz_app_offscreen_recording.dvzr`.

Fourth follow-up DVZR slice on 2026-05-15: `examples/c/record_scene_dvzr.c` now records a visible
offscreen point scene to `.dvzr`, saves the app-captured frame to
`record_scene_dvzr_original.png`, replays the recording through the real vklite DRP2 runtime, and
saves the replay target to `record_scene_dvzr_replay.png`. Synthetic app recording targets now
include `COPY_SRC` usage so replayed borrowed targets can be read back, and portable recording load
now restores SPIR-V payload sizes for actual Vulkan replay. Focused validation: `just build`,
`./build/examples/c/record_scene_dvzr` with byte-identical original/replay PNGs,
`./build/testing/dvztest_scene test_app_offscreen_records_dvzr_frames`,
`./build/testing/dvztest_drp2 drp2_recording` (`4/4`), and
`./build/testing/dvz_drp2_player --fast build/examples/c/record_scene_dvzr.dvzr`.

Fifth follow-up DVZR slice on 2026-05-15: app windows can now replay app-recorded `.dvzr` streams
directly into a live GLFW swapchain. Live replay attaches the current borrowed canvas frame under
the recorded app target id and filters the synthetic target `CreateTexture` command from the setup
frame, so recorded frame streams execute through the real vklite runtime and present normally.
`examples/c/replay_dvzr_glfw.c` opens a live replay window with paced, fast, loop, speed, and
bounded-frame modes. `examples/c/hello_mesh_glfw.c` now accepts `record`, `record=PATH`, or
`--record PATH`, so the existing rotating interactive cube can be recorded and then replayed.
Focused validation: `just build`,
`./build/examples/c/record_scene_dvzr`,
`./build/examples/c/replay_dvzr_glfw --fast --frames 2 build/examples/c/record_scene_dvzr.dvzr`,
`./build/examples/c/hello_mesh_glfw 3 record=/tmp/dvz_mesh_live_replay.dvzr`,
`./build/examples/c/replay_dvzr_glfw --fast --frames 4 /tmp/dvz_mesh_live_replay.dvzr`,
`./build/testing/dvz_drp2_player --fast /tmp/dvz_mesh_live_replay.dvzr`,
`./build/testing/dvztest_scene test_app_offscreen_records_dvzr_frames`, and
`./build/testing/dvztest_drp2 drp2_recording` (`4/4`).

Focused validation recorded before the latest `2026-05-13` follow-up commits:

1. `just spec-check`: last recorded pass remained `119/119` DRP2 fixtures; `52` fixture-runner
   tests passed.
2. `just test drp2`: last recorded pass remained `73/73`.
3. `just test scene`: passed `127/127` after finishing the current request-resolution cleanup
   pass. Point pick and image probe now resolve through GPU-backed auxiliary DRP2/readback
   execution only; request freshness is explicit and persistent per panel/request-kind scope, and
   image probes now use the same explicit recentering rule as point picking.
4. `git diff --check`: passed on the latest scene slices.

Recent revalidation after the trace and render-pass ordering follow-up includes `just test app`
(`28/28`) and `just test scene` (`141/141`). The latest smoke checks also covered
`hello_mesh_glfw` normal trace mode with colors enabled and disabled. User-reported smoke on
`2026-05-14` also covered the `hello_*` C examples successfully.

First focused hygiene slice on `2026-05-14`: DRP2 texture-layout validation now rejects overflowing
3D transfer byte sizes, and image probe plan assembly now checks position, texcoord, and texture
byte sizes before allocation/upload. Validation: `just build`, `just test drp2` (`81/81`),
`just test scene` (`142/142`), `just test app` (`28/28`), `git diff --check`, and
`clang-tidy -p build --quiet` on the touched DRP2/scene files.

Second focused hygiene slice on `2026-05-14`: borrowed frame-target depth attachments are now built
locally before being assigned to the target object, and previous borrowed depth attachments are
retired through the deferred-destroy queue keyed by the borrowed command buffer. Validation:
`just build`, `just test drp2` (`82/82`), `just test scene` (`142/142`), and `just test app`
(`28/28`), `clang-tidy -p build --quiet` on the touched DRP2 files, and bounded
`hello_mesh_glfw 60` smoke.

Third focused hygiene slice on `2026-05-14` (`5a6c0608`): consumed pick/probe result slots are
cleared after polling so queue storage does not retain stale panel pointers or payload data.
Validation: `git diff --check`, `just build`, `just test scene` (`143/143`), and `clang-tidy -p
build --quiet` on the touched scene files.

Fourth focused hygiene slice on `2026-05-14` (`142673bb`): scene test warning readiness was
improved by adding the missing render-pass-scope test prototype and replacing one direct
`memset()` in scene tests with `dvz_memset()`. Validation: `git diff --check`, `just build`,
`just test scene` (`143/143`), and `clang-tidy -p build src/scene/tests/scene_graph.c --quiet`.

Fifth focused hygiene slice on `2026-05-14` (`aee41d6b`): DRP2 vklite transient backend object
tables now trim destroyed tail slots after render/compute pass cleanup, explicit backend destroys,
and deferred borrowed-frame retirement setup. Validation: `git diff --check`, `just build`,
`just test drp2` (`83/83`), `just test scene` (`143/143`), and `clang-tidy -p build --quiet` on
the touched DRP2 files.

First WBOIT planning slice on `2026-05-15`: retained scene panel planning routes WBOIT visuals into
a transparent accumulation FramePlan node, keeps opaque/mask/ordinary blended visuals
in the opaque node, and appends a WBOIT resolve node when transparent visuals are present. Capability
validation now scans all render nodes rather than only the first one, so split render plans still
observe texture, scene-render, and WBOIT requirements. Focused validation: `just build` passed;
`./build/testing/dvztest_scene test_scene_visual_alpha_mode` passed the alpha-mode storage,
pass-split, and capability checks; adjacent panel-render filters passed; and `clang-tidy -p build
--quiet` on the touched scene files reported no new actionable diagnostics. Full `just test scene`
still reports eight pre-existing exact fixture mismatches in generated DRP2 JSON fixtures
(`scene_*_from_c` and WGSL scene fixture tests), and `just test drp2` reports the same five
`scene_*_from_c` fixture mismatches after passing the DRP2 runtime and multi-color render-pass
coverage.

Executable WBOIT scene/app slice on `2026-05-15`: `DVZ_ALPHA_BLENDED` is now the ordinary source-over
alpha path and `DVZ_ALPHA_WBOIT` is the explicit weighted blended OIT path. Scene lowering emits the
WBOIT accumulation/resolve DRP2 shape with scene-owned shaders, vklite records all passes into the
active borrowed frame command buffer, and transient depth/color transitions are synchronized for the
multi-pass app path. `examples/c/hello_mesh_wboit_glfw.c` now exercises an arcball mesh scene with
a single lit WBOIT transparent cube between opaque reference cards. The example also has a GUI panel
for live cube RGB/alpha,
ambient/diffuse, and light-direction tuning. Validation before this simplification: `just build`,
`./build/testing/dvztest_scene test_scene_visual_alpha_mode` (`6/6`),
`./build/testing/dvztest_drp2 test_drp2` (`86/86`), and
`./build/examples/c/hello_mesh_wboit_glfw 2` passed without validation output.

Follow-up WBOIT/depth diagnostic slice on `2026-05-15`: DRP2 streams now carry non-executable debug
labels for scene resource/object ids, and app full trace prints ids as `id(label)` where available.
This makes `DVZ_DRP2_TRACE=full DVZ_DRP2_TRACE_COLOR=0 ./build/examples/c/hello_mesh_wboit_glfw 2`
usable for comparing scene intent to DRP2 commands. The WBOIT example/regression now verifies that
fixed background primitives do not write depth, opaque unlit reference primitives do write depth,
WBOIT accumulation depth-tests without writing, and the resolve pass composites into `rt` without a
depth attachment. Focused validation: `cmake --build build --target dvztest_drp2 dvztest_scene
hello_mesh_wboit_glfw -j2`, `./build/testing/dvztest_drp2 test_drp2_stream_debug_labels`,
`./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`, and bounded labeled
trace smoke.

WBOIT visual diagnostic follow-up on `2026-05-15`: `hello_mesh_wboit_glfw` keeps the dark background
by default, has a GUI toggle for a light comparison background, and uses tuned cube/reference colors
so face overlap and front-card occlusion are easier to judge during live rotation.


## Immediate Task

The next work should stay implementation-focused and build on the current retained scene/runtime
path.

Read in this order:

1. this file for current ordering,
2. [../../spec/scene/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/README.md) for scene
   semantics,
3. [../done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
   for the active vertical-slice history,
4. [../done/SCENE_PICK_PROBE_EXECUTION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_PICK_PROBE_EXECUTION.md)
   for the current shipped request-resolution behavior and caveats,
5. [SCENE_PICK_PROBE_EXECUTION_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCENE_PICK_PROBE_EXECUTION_PLAN.md)
   for the original planned shape,
6. the current `scene` and `drp2` tests before broadening any API.

Deliver the next implementation slices in this order unless the user redirects:

1. Done: focused scene-test decomposition split `src/scene/tests/test_scene.c` into short
   domain-named files under `src/scene/tests/` while preserving the current test function names and
   `test_scene(TstSuite*)` as the single module entry point. `test_scene.c` is now the aggregator
   only, `test_scene.h` remains the shared declaration header, and the split files are
   `panzoom_arcball.c`, `frame_plan.c`, `frame_plan_emit.c`, `scene_graph.c`, `fields.c`,
   `interaction.c`, `pick_probe.c`, and `app.c`.
2. Done: finish and validate the native 3D pressure smoke around the existing
   `examples/c/hello_mesh_glfw.c`. That example already exercises an interactive mesh scene with
   arcball, depth, resize-through-app synchronization, and a frame callback through the scene ->
   DRP2 -> app boundary. Normal trace smoke now verifies changed-frame output, default colors,
   `NO_COLOR` / `DVZ_DRP2_TRACE_COLOR=0`, and resource setup before render-pass scopes. The
   paired offscreen `hello_mesh.c` capture path and the broader `hello_*` C smoke set have run
   successfully, so this lane is now recorded as validated rather than a reason to add a duplicate
   3D example.
3. Current docs slice: manual interactive smoke set is recorded in
   [../../docs/architecture/manual_scene_smoke.md](/home/cyrille/GIT/Viz/datoviz/docs/architecture/manual_scene_smoke.md).
   The live image-probe smoke now has `examples/c/hello_image_probe_glfw.c`, and the live
   partial texture-update smoke now has `examples/c/hello_texture_update_glfw.c`; the live
   multi-panel smoke now has `examples/c/hello_multi_panel_glfw.c`, plus
   `examples/c/hello_linked_panels_glfw.c` for linked panzoom propagation.
4. Current next: the hot-path hygiene pass has covered bounds checks, borrowed-depth ownership,
   stale result-slot cleanup, scene warning readiness, and DRP2 vklite transient object table
   trimming. Remaining review areas are trace/status hashing and string-buffer safety, plus a
   bounded live app smoke around request/runtime steady state.
5. Current DVZR next: decide whether `dvz_drp2_player` and `replay_dvzr_glfw` should stay developer
   executables or become installed CLI/app-level integration points, add image-diff or bounded
   live-window replay regression coverage if this becomes a CI lane, and broaden portable command
   coverage beyond the point/primitive/mesh/image baseline whenever a real scene/app stream reports
   raw fallbacks.
6. CUDA/CuPy external-memory interop priority: treat CUDA/CuPy-owned GPU pointer -> Vulkan import as
   unreliable on this branch. Prioritize the opposite direction for any new interop work: create and
   own the allocation in Vulkan, export it through external memory, import it into CUDA/CuPy, and
   synchronize cross-API access explicitly with external semaphores or timeline-compatible plumbing.
   Do not make NVIDIA CIG (`VK_NV_external_compute_queue` / CUDA-in-Graphics contexts) a dependency
   of this route; it is optional NVIDIA-specific scheduling plumbing, not required for Vulkan-owned
   external memory imported into CUDA/CuPy. The canonical registered CUDA interop smoke is now
   `src/vk/tests/test_memory.c:test_memory_cuda_1`, which exercises the preferred Vulkan-owned
   buffer -> CUDA import path, matches CUDA/Vulkan devices by UUID before creating the Vulkan
   device, and covers same-direction external timeline semaphore synchronization. The DRP2-level
   smoke `src/drp2/tests/test_drp2.c:test_drp2_runtime_vklite_draws_cuda_external_vertex_buffer`
   now verifies a CUDA-filled Vulkan-owned external vertex buffer registered through
   `dvz_drp2_runtime_register_external_buffer()`, rendered through the vklite runtime, and checked
   by texture readback. `test_memory_cuda_2` remains available for the later CUDA-owned allocation
   -> Vulkan import direction, but it should not drive the primary architecture. Next
   implementation step: define the Python/CuPy-facing exported handle + size/offset + semaphore
   metadata contract without adding a generic public binding API yet.
7. Early WebGPU feasibility spike: replay a tiny DRP2 subset for clear, static point/primitive/image,
   then depth. Keep it contract-pressure only; do not fork scene semantics.
8. Rendered colorbar/text/annotation realization, reusing the current scene -> DRP2 path after the
   native 3D and manual-smoke gaps are clearer.
9. Picking payload widening after the hardened slice: richer ids, mesh targets, and less ad-hoc RGBA
   payload encoding.
10. WBOIT follow-up slice: add any missing offscreen WBOIT readback/capture coverage, and tighten
    DRP2 validation around pipeline color-target formats
    versus render-pass attachment formats. Use
    [WBOIT_MESH_INTERACTIVE_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/WBOIT_MESH_INTERACTIVE_PLAN.md)
    as the implementation checklist.
11. SSAO planning note: early scene-level SSAO should follow the active scene -> FramePlan -> DRP2
    -> vklite path and reuse the WBOIT-style multi-pass resource pattern. The implementation plan
    is recorded in [SCENE_SSAO_IMPLEMENTATION_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCENE_SSAO_IMPLEMENTATION_PLAN.md).

Implementation-level checklists for these lanes are recorded in
[../../docs/tasks/2026-05-13-next-implementation-priorities/NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/docs/tasks/2026-05-13-next-implementation-priorities/NEXT_STEPS.md).

Sidecar design slice recorded on 2026-05-13:

1. Visual attribute sources and constant-value optimization are specified in
   [../../spec/scene/pipeline/ATTRIBUTE_SOURCES.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/pipeline/ATTRIBUTE_SOURCES.md).
2. The implementation pickup note is
   [../../docs/tasks/2026-05-13-visual-attribute-sources/NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/docs/tasks/2026-05-13-visual-attribute-sources/NEXT_STEPS.md).
3. This is not yet active code. The first recommended slice is constant point `size` via
   `dvz_visual_set_value(point, "size", &size)` while retaining the current dense per-item fallback.


## Scope Guardrails

For the immediate implementation pass:

1. Do not add a generic public binding API yet; keep public setters typed.
2. Do not invent a second mesh renderer path; reuse the current scene -> DRP2 -> runtime flow.
3. Prefer scene-owned reusable resources over visual-private upload helpers.
4. Keep examples and focused tests in lockstep with each retained slice.
5. Treat declared-but-unimplemented public functions as a priority: either implement them narrowly
   or mark/document the gap before depending on them.
6. For CUDA/CuPy interop, do not build new architecture around importing CUDA-owned allocations into
   Vulkan. Prefer Vulkan-owned exportable resources that CUDA/CuPy imports through external-memory
   handles. Keep NVIDIA CIG optional and vendor-specific rather than part of the required
   external-memory contract.


## Roadmap After The Immediate Pass

After the immediate native 3D/manual-smoke/safety passes, proceed in this order unless the user redirects:

1. Browser/WebGPU feasibility: replay a narrow DRP2 subset for point, primitive, image, and minimal
   mesh/depth scenes.
2. Transparency architecture: explicit WBOIT-style scene mode through frame plan, DRP2, runtime, and
   capability fallback.
3. Broader figure features: axes, lines/segments, rendered text/labels, colorbars, richer
   annotations, picking refinements, and additional visual families.
4. Larger code organization cleanup once the active API seams stabilize enough to avoid churn.


## Validation Defaults

For documentation-only passes:

1. run `git diff --check`,
2. inspect `git status --short`,
3. do not run the graphics suite unless code or generated fixtures changed.

For scene/DRP2 code changes:

1. run `just build`,
2. run the narrowest relevant `just test <filter>`,
3. use Vulkan validation smoke tests for changes touching `vk`, `vklite`, `canvas`, `scene`,
   `drp2`, command buffers, frame lifetimes, render targets, swapchains, or synchronization.


## Request Slice Status

The first end-to-end request path is now in better shape than the original plan snapshot:

1. image probe no longer falls back to CPU-side texture sampling; misses now stay misses,
2. auxiliary readback execution now resets DRP2 runtime state before each synthetic request stream,
   which avoids `HELLO/REPLY` semantic-state collisions across multiple requests on one runtime,
3. request freshness now stays explicit after polling, so late stale GPU results cannot reappear
   once a newer panel-local request scope has already been claimed,
4. image probes now use the same explicit request recentering rule as point picking instead of an
   implicit fixed-pixel assumption,
5. focused scene coverage now includes:
   - successful combined pick+probe resolution,
   - per-quadrant image probe position checks against a non-uniform texture,
   - transparent GPU probe miss,
   - forced GPU readback failure miss,
   - late-result rejection after newer pick/probe results were already polled,
   - consumed pick/probe result slot cleanup after polling.

Batching was considered after the freshness cleanup and explicitly deferred for now:

1. current hover-style traffic is already coalesced to the newest unresolved request per
   panel/kind scope before execution,
2. that coalescing sharply reduces the payoff of batching for ordinary one-panel hover traffic,
3. compatible batching may still become worthwhile for multi-panel or tool-driven request bursts,
   but it is not the current priority,
4. unless profiling shows real churn from the one-stream-per-request path, move up-stack instead of
   broadening the request executor now.


## Completed Context

Completed implementation records:

1. [../done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
2. [../done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
3. [../done/CONTROLLER_TRANSFORM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/CONTROLLER_TRANSFORM_DESIGN.md)
4. [../done/SCENE_PICK_PROBE_EXECUTION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_PICK_PROBE_EXECUTION.md)

Backlog context:

1. [../later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)
2. [../later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md)
