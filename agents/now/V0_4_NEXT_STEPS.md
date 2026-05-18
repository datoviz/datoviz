# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT GUIDE`
> - **Updated on:** `2026-05-18`
> - **Purpose:** give future agents the practical next steps after the first scene -> DRP2 ->
>   vklite/canvas slice.

For ranked scene/example priorities and the capability matrix that connects examples to core
feature work, see
[`SCENE_EXAMPLE_PRIORITIZATION.md`](../soon/SCENE_EXAMPLE_PRIORITIZATION.md).

For the current C test-runner audit and staged modernization plan toward explicit grouping,
resource metadata, and safe future parallelism, see
[`TEST_RUNNER_MODERNIZATION.md`](TEST_RUNNER_MODERNIZATION.md).


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
3. Built-in visual families currently implemented are `point`, `pixel`, `marker`, `primitive`,
   `mesh`, path/segment, `image`, `volume`, and `sphere`.
4. Panel controllers are live: panzoom, arcball, fly, and turntable feed per-panel transforms.
5. Per-panel viewport/scissor and offscreen multi-panel preservation work through the emitted DRP2
   path.
6. Retained sampled fields, scales, scene buffers, material uniforms, volume state, and visual
   resource bindings now share the scene -> frame-plan -> DRP2 binding path.
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
multi-pass app path. `examples/c/techniques/wboit.c` now exercises an arcball mesh scene with
a single lit WBOIT transparent cube between opaque reference cards. The example also has a GUI panel
for live cube RGB/alpha,
ambient/diffuse, and light-direction tuning. Validation before this simplification: `just build`,
`./build/testing/dvztest_scene test_scene_visual_alpha_mode` (`6/6`),
`./build/testing/dvztest_drp2 test_drp2` (`86/86`), and
`./build/examples/c/techniques/wboit 2` passed without validation output.

Follow-up WBOIT/depth diagnostic slice on `2026-05-15`: DRP2 streams now carry non-executable debug
labels for scene resource/object ids, and app full trace prints ids as `id(label)` where available.
This makes `DVZ_DRP2_TRACE=full DVZ_DRP2_TRACE_COLOR=0 ./build/examples/c/techniques/wboit 2`
usable for comparing scene intent to DRP2 commands. The WBOIT example/regression now verifies that
fixed background primitives do not write depth, opaque unlit reference primitives do write depth,
WBOIT accumulation depth-tests without writing, and the resolve pass composites into `rt` without a
depth attachment. Focused validation: `cmake --build build --target dvztest_drp2 dvztest_scene
techniques/wboit -j2`, `./build/testing/dvztest_drp2 test_drp2_stream_debug_labels`,
`./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`, and bounded labeled
trace smoke.

WBOIT visual diagnostic follow-up on `2026-05-15`: `techniques/wboit` keeps the dark background
by default, has a GUI toggle for a light comparison background, and uses tuned cube/reference colors
so face overlap and front-card occlusion are easier to judge during live rotation.

WBOIT resize follow-up on `2026-05-16` (`e18cca96`): `techniques/wboit` exposed the same stale
sampled-descriptor class previously seen in the depth-peeling resize path. WBOIT resolve bind-group
emission now fingerprints the sampled accumulation/weight targets and target extent, so resize
rebuilds the descriptor set instead of sampling destroyed image views. Validation: `git diff
--check`, `just build`, `just test test_scene_visual_alpha_mode_emits_wboit_drp2`,
`just test test_scene_visual_alpha_mode_wboit_glsl_executes`,
`./build/examples/c/techniques/wboit 60`, and `just test scene` (`203/203`). This is a
tactical per-technique guardrail. The preferred generic fix is to make DRP2/vklite refresh
dependent bind-group descriptors whenever a stable resource id is recreated; see
[DRP2_DESCRIPTOR_REFRESH_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_DESCRIPTOR_REFRESH_PLAN.md).

Descriptor-refresh/runtime smoke follow-up on `2026-05-16`: the DRP2/vklite runtime now has GPU
execution coverage for descriptor refresh after recreating uniform buffers, storage buffers, and
samplers (`482d1f64`). The app path also has a retained offscreen resize smoke over one reused
runtime with mixed mesh and image visuals (`343a2cf2`).

Hot-path trace/request follow-up on `2026-05-16`: app trace fingerprinting now hashes recently added
stable semantic fields, including texture formats, pipeline color/raster/blend state, render-pass
attachment ops, dynamic offsets, and bounded fixed-size labels (`4b411390`). The app test suite now
includes a bounded offscreen pick/probe steady-state smoke that queues requests before repeated
frames, exercises the real app-owned runtime/request executor, and verifies pending/result slots are
cleared after every frame (`5a97417e`). Validation: `cmake --build build --target dvztest`,
`cmake --build build --target dvztest_scene`,
`./build/testing/dvztest_scene test_app_offscreen_pick_probe_request_steady_state` (`1/1`),
`just test app` (`48/48`), `just test scene` (`206/206`), `just spec-check` (`123/123` DRP2
fixtures, `35/35` WebGPU preflight fixtures, `52` fixture-runner tests, and `7` schema/generation
tests), and `git diff --check`.

EDL scene-technique slice on `2026-05-16`: panel-local Eye-Dome Lighting is now available through
`dvz_panel_set_edl()`. Scene emission lowers eligible opaque point/pixel and depth-writing visuals
into a graph-declared color/depth intermediate plus a fullscreen EDL resolve pass. The DRP2 vklite
runtime now transitions sampled depth bindings with the depth-read layout expected by depth
descriptors, which keeps both EDL and existing volume depth sampling validation-clean. The example
`examples/c/hello_points_edl_glfw.c` shows a dense point cloud with an ImGui control panel for
enabling/disabling EDL and tuning radius, strength, and depth scale. Validation: `just build`,
`just test drp2` (`119/119`), `just test scene` (`212/212`),
`just test test_app_offscreen_points_edl_renders`, bounded smoke
`./build/examples/c/hello_points_edl_glfw 1`, `git diff --check`, and `clang-tidy -p build --quiet`
on the touched DRP2/scene/example files.

Scene techniques/materials follow-up on `2026-05-16`: the architecture lane now has the first
material-backed effect and graph-backed technique foundations. Landed commits include technique
builder extraction (`bc36100e`), internal material state (`6b85c209`), lit primitive depth cueing
(`bbb17c87`), visual pass capabilities (`af19c693`), G-buffer graph declaration (`310b3cb9`),
planning-doc refresh (`ec1d3c84`), and opt-in G-buffer runtime lowering (`f58cec92`). The current
G-buffer slice emits normal/depth targets for eligible primitive/mesh visuals with normals through
the existing scene -> FramePlan graph -> DRP2 -> vklite path, guarded by an internal opt-in flag.
Validation for `f58cec92`: `just build`, `just test test_scene_gbuffer_runtime_lowering`,
`just test scene` (`210/210`), `just test drp2` (`119/119`), and `git diff --check`.

Scene technique activation follow-up on `2026-05-16`: the temporary scene-level G-buffer boolean was
replaced by internal scene/panel `DvzSceneTechniqueState` (`0068f1e2`). G-buffer remains default-off,
and the focused runtime regression now verifies both unchanged default planning and panel-local
opt-in. Validation: `just build`, `just test test_scene_gbuffer_runtime_lowering`, `just test scene`
(`210/210`), `just test drp2` (`119/119`), `clang-tidy -p build --quiet` on the touched scene files,
and `git diff --check`.

Follow-up implementation through `2026-05-17`: the descriptor-refresh lane is now a runtime
invariant rather than a per-technique workaround. Recreated stable DRP2 texture, buffer, and sampler
ids refresh dependent vklite bind-group descriptors, and WBOIT/depth-peeling resize paths no longer
need extent fingerprints for sampled bind-group freshness. The scene techniques/materials lane has
advanced through public `DvzMaterialDesc`, shared material shader evaluation, EDL, SSAO, sphere
impostors, and graph-backed MSAA. The shader-ABI lane now has `_scene_shader_abi.h`, shared
GLSL/WGSL common ABI helpers, centralized runtime bind layout ordering, centralized visual vertex
attribute descriptor writes, consolidated depth-state selection, and `just shader-abi-check`.

Render-contract follow-up on `2026-05-18`: the remaining user-reported `just tests` failure,
`test_app_offscreen_volume_slice_scene_occlusion_dimming`, is fixed in the focused branch state.
The volume-slice scene-occlusion shader now relies on the scene-occlusion shader variant plus the
sampled depth/hidden-alpha parameters instead of an extra `params.w` enable gate, and render runtime
multi-draw emission now invalidates cached set 1 and set 2 bind groups when the pipeline changes.
Focused validation passed for the failing app test, nearby volume/scene-occlusion tests, the generic
volume-slice scene-occlusion structural test, `direnv exec . just test
test_app_offscreen_volume_slice_scene_occlusion_dimming`, `direnv exec . just test
test_scene_volume_slice_uses_generic_scene_occlusion`, and `git diff --check`. Broader full-suite
revalidation remains the next confidence step before treating this as a full green baseline.

Retained scene-occlusion toggle follow-up on `2026-05-18` (`ef324c1e`): the brain showcase
regression from clicking `Show atlas mesh` is covered by
`test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle`. The regression fixture renders a
volume/slice scene, then toggles a hidden mesh visible as a scene occluder while the slice samples
scene occlusion. The test failed first with `DRP2 sampled bind group misses graph read resource`,
matching the showcase failure. The contract checker now infers sampled graph-read coverage from
persistent bind-group label dependency ids when a retained frame reuses a bind group created in an
earlier stream. Focused validation passed for the new regression and adjacent volume/scene-occlusion
tests. User-reported validation after the fix covered the original `./build/examples/c/showcase/brain`
GUI repro, focused scene/app filters, `just test scene`, and the full `just tests` run successfully.


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
5. [SCENE_PICK_PROBE_EXECUTION_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_PICK_PROBE_EXECUTION_PLAN.md)
   for the original planned shape,
6. [IMAGE_PICKING_RECOVERY_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/IMAGE_PICKING_RECOVERY_PLAN.md)
   before touching the current image probe / segmentation-label hover path,
7. the current `scene` and `drp2` tests before broadening any API.

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
4. Done: the hot-path hygiene pass has covered bounds checks, borrowed-depth ownership, stale
   result-slot cleanup, scene warning readiness, DRP2 vklite transient object table trimming,
   trace/status hashing and string-buffer safety, and a bounded app smoke around request/runtime
   steady state.
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
10. WBOIT follow-up slice: DRP2 validation now rejects unsupported non-depth pipeline color-target
    formats and has explicit negative coverage for pipeline/render-pass color-target count,
    nonzero-index format, depth-presence, and depth/color class mismatches. Remaining work is any
    missing offscreen WBOIT readback/capture coverage. Use
    [WBOIT_MESH_INTERACTIVE_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/WBOIT_MESH_INTERACTIVE_PLAN.md)
    as the implementation checklist.
11. Done: DRP2/vklite descriptor refresh is implemented. Future work should treat it as a runtime
    invariant and add focused coverage only when a new resource kind or backend handle lifetime can
    stale existing bind groups. The completion record is
    [DRP2_DESCRIPTOR_REFRESH_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_DESCRIPTOR_REFRESH_PLAN.md).
12. Scene shader ABI / WGSL parity: keep `just shader-abi-check` green whenever shader files,
    bind layouts, vertex attributes, material/image/volume bindings, or pipeline keys move. The
    remaining portable shader work is now specific parity lanes: marker, segment/path stroke,
    sphere, volume, and capability-gated advanced passes. Use
    [VISUAL_SHADER_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/implementation/VISUAL_SHADER_REFACTOR.md)
    as the active checklist.
13. Scene techniques/materials polish: the architecture is implemented through the current
    material, EDL, SSAO, sphere, and MSAA slices. Remaining work is narrower: improve the standard
    material look without turning it into full PBR, decide family-specific material policy for
    point/pixel/image/volume, add material-aware G-buffer fields only when a concrete effect needs
    them, and update examples/docs that still present primitive-specific shading as primary.
14. Multi-pass graph / transparency follow-up: keep WBOIT, depth peeling, blended volume, G-buffer,
    EDL, SSAO, SSAO blur, scene occlusion, and MSAA on the shared FramePlan graph path. Render-role
    pass policy is now centralized in the technique layer; the next transparency work should add any
    missing offscreen readback/capture coverage and continue explicit resource-access/layout-transition
    cleanup before adding another transparency mode.
15. Sphere and dense-particle follow-up: sphere is now a standalone retained visual with material
    lighting, antialiased silhouettes, raycast mode, G-buffer output, and SSAO coverage. Remaining
    sphere work belongs in targeted follow-ups such as texture/equirectangular mapping, render-mode
    quality tuning, and WGSL parity rather than in the original "add sphere" lane.

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
2. Transparency architecture: internal FramePlan graph skeleton, required DRP2 attachment/access
   upgrades, graph-backed WBOIT lowering, then the next transparency technique through the same
   scene -> FramePlan -> DRP2 -> runtime path.
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

GLFW validation note after the `2026-05-17` lifetime cleanup:

1. in the unified `dvztest` process, tests must not call raw `glfwTerminate()` after initializing
   GLFW through Datoviz; `backend_glfw.c` owns GLFW process lifetime,
2. GLFW-using fixtures should still isolate per-test resources by destroying all `GLFWwindow`,
   Vulkan surface, `DvzWindow`, `DvzCanvas`, and app/runtime objects they create,
3. future cleanup should reset process-global GLFW window hints with `glfwDefaultWindowHints()`
   before every Datoviz-backed or raw GLFW test window creation, then explicitly set the required
   hints such as `GLFW_CLIENT_API`, visibility, and resizability,
4. true init/terminate isolation belongs in subprocess-style GLFW tests or separate focused
   executables, not in the shared-process runner.


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
