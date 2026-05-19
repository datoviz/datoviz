# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT DISPATCH`
> - **Updated on:** `2026-05-19`
> - **Purpose:** keep the current v0.4 work queue short and point longer history elsewhere.

Start here, then follow the linked specs or completed records only for the subsystem being edited.


## Current Position

The active stack is `scene` -> `drp2` -> `vklite`/`canvas`, with `app` providing the small
presentation layer. The low-level graphics modules (`vk`, `vklite`, `canvas`, `stream`, `video`,
and `window`) are the runtime foundation; do not create parallel presentation, frame-stream, or
Vulkan wrapper paths.

Implemented scene coverage includes retained point, pixel, marker, primitive, mesh, path/segment,
image, volume, sphere, sampled-field, material, controller, pick/probe, annotation/text
bookkeeping, graph-backed technique, and app/offscreen/GLFW slices. Treat these as active code, not
future scaffolding.

The serial C test-runner modernization baseline is complete. Historical notes live in
[../done/TEST_RUNNER_MODERNIZATION.md](../done/TEST_RUNNER_MODERNIZATION.md); remaining scheduling
work lives in [../soon/TEST_RUNNER_SCHEDULING.md](../soon/TEST_RUNNER_SCHEDULING.md).


## Read First

1. [APP_FRAME_SCHEDULING_REFACTOR.md](APP_FRAME_SCHEDULING_REFACTOR.md) before changing the
   built-in app loop, frame pacing, window backend waiting, request-frame wakeups, or immediate
   present CPU behavior.
2. [../../spec/scene/README.md](../../spec/scene/README.md) before changing scene semantics,
   public scene API shape, frame planning, interaction, annotations, scales, visual families, or
   runtime boundaries.
3. [../../spec/scene/api/API_SURFACE.md](../../spec/scene/api/API_SURFACE.md) before changing
   public scene API shape or adding scene object families.
4. [../../spec/drp2/README.md](../../spec/drp2/README.md) and
   [../../spec/drp2/AGENT_SPEC_PHASE.md](../../spec/drp2/AGENT_SPEC_PHASE.md) before touching
   DRP2 commands, fixtures, schema docs, or DRP2-emitting scene code.
5. [IMPLEMENTATION.md](IMPLEMENTATION.md) for the current
   lane inventory and parallel-work prompts.
6. [RELEASE.md](RELEASE.md) for post-feature-completion
   API review, documentation, bindings, gallery, packaging, and release-candidate work.


## Active Priorities

1. **App frame scheduling:** make `dvz_app_run(app, 0)` event-aware and optionally capped while
   preserving explicit continuous/immediate-present benchmark behavior. Start with
   [APP_FRAME_SCHEDULING_REFACTOR.md](APP_FRAME_SCHEDULING_REFACTOR.md).
2. **Text/colorbar realization:** finish rendered text, labels, annotations, colorbar ramps, and
   MSDF/SDF/bitmap parity through the existing scene -> frame-plan -> DRP2 path.
3. **WebGPU/WGSL parity:** pressure the active DRP2 subset with point, primitive, image, minimal
   mesh/depth, marker, segment/path stroke, sphere, volume, and capability-gated advanced passes.
4. **DVZR portability:** keep `dvz_drp2_player`, `replay_dvzr_glfw`, app recording hooks, and raw
   fallback diagnostics aligned with real scene/app streams; broaden portable commands only when
   real recordings expose raw fallback gaps.
5. **Runtime and technique hardening:** continue focused fixes for descriptor/resource lifetimes,
   graph-backed transparency, MSAA, EDL, SSAO, volume occlusion, scene occlusion, and deterministic
   offscreen readback/capture.
6. **Material and shader ABI polish:** keep shader ABI checks green; refine the standard material
   look, family-specific material policy, and GLSL/WGSL parity without turning v0.4 into a full PBR
   pass.
7. **Picking and selection payloads:** widen point/marker/image pick/probe payloads, highlight
   state, linked-panel request propagation, and explicit deferrals for mesh/path/volume picking.
8. **Examples and gallery pressure:** keep C examples, manual smoke notes, gallery harnesses,
   screenshots, and video/capture paths exercising already-implemented features.
9. **CUDA/CuPy external-memory contract:** prefer Vulkan-owned exportable resources imported into
   CUDA/CuPy with explicit external-memory and semaphore metadata. Do not make CUDA-owned
   allocation import or NVIDIA CIG the primary architecture.
10. **Runner scheduling follow-up:** process-level sharding, CI orchestration, optional
   thread-safe workers, and remaining ad-hoc skip migration belong to
   [../soon/TEST_RUNNER_SCHEDULING.md](../soon/TEST_RUNNER_SCHEDULING.md).


## Useful Current Records

1. [../done/SCENE_DRP2_IMPLEMENTATION.md](../done/SCENE_DRP2_IMPLEMENTATION.md): completed first
   scene -> DRP2 -> runtime slice.
2. [../done/DRP2_SCENE_SAFETY.md](../done/DRP2_SCENE_SAFETY.md): ownership and safety cleanup
   history.
3. [../done/SCENE_PICK_PROBE_EXECUTION.md](../done/SCENE_PICK_PROBE_EXECUTION.md): shipped
   request-resolution behavior and caveats.
4. [../done/DRP2_DESCRIPTOR_REFRESH_PLAN.md](../done/DRP2_DESCRIPTOR_REFRESH_PLAN.md): completed
   descriptor refresh invariant.
5. [../done/WBOIT_MESH_INTERACTIVE_PLAN.md](../done/WBOIT_MESH_INTERACTIVE_PLAN.md): WBOIT
   implementation record and follow-up checklist.
6. [../soon/SCENE_EXAMPLE_PRIORITIZATION.md](../soon/SCENE_EXAMPLE_PRIORITIZATION.md): ranked
   scene/example priorities.
7. [../../docs/tasks/2026-05-13-next-implementation-priorities/NEXT_STEPS.md](../../docs/tasks/2026-05-13-next-implementation-priorities/NEXT_STEPS.md):
   detailed implementation-lane notes from the May 13 planning pass.
8. [../../docs/architecture/manual_scene_smoke.md](../../docs/architecture/manual_scene_smoke.md):
   manual interactive scene smoke coverage.


## Scope Guardrails

1. Stabilize active modules first; keep inactive scaffolding such as `color`, `wasm`, broad
   text/gui layers, and renderer/client layers untouched unless explicitly requested.
2. Do not add a generic public binding API yet; keep public setters typed.
3. Do not invent a second mesh renderer, presentation layer, frame stream, or Vulkan wrapper path.
4. Prefer scene-owned reusable resources over visual-private upload helpers.
5. Keep examples and focused tests in lockstep with each retained slice.
6. Treat declared-but-unimplemented public functions as a priority: implement them narrowly or
   document the gap before depending on them.
7. Preserve immediate presentation paths for run-as-fast-as-possible benchmarks through explicit
   continuous scheduling; do not make immediate present imply an unconditional static-scene spin.


## Validation Defaults

For documentation-only passes:

1. run `git diff --check`,
2. inspect `git status --short`,
3. do not run graphics tests unless code or generated fixtures changed.

For scene/DRP2/runtime code changes:

1. run `just build`,
2. run the narrowest relevant `just test <filter>` or focused runner,
3. run `just spec-check` for DRP2 schema/fixture/portable-command changes,
4. use Vulkan validation smoke tests for changes touching `vk`, `vklite`, `canvas`, `scene`,
   `drp2`, command buffers, frame lifetimes, render targets, swapchains, or synchronization.

GLFW tests in the unified process must not call raw `glfwTerminate()` after Datoviz initializes
GLFW; `backend_glfw.c` owns process lifetime. Use subprocess-style tests or focused executables
when true init/terminate isolation is required.
