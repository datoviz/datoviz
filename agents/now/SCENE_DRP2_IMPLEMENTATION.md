# Scene/DRP2 Implementation Plan

> **Status:** `ACTIVE IMPLEMENTATION PLAN`
> **Created on:** `2026-04-28`
> **Purpose:** provide the concrete first implementation path for DRP2 and the future scene layer,
> using the existing `canvas`, `stream`/sink, and `vklite` stack instead of inventing a parallel
> graphics path.

This document is the implementation entry point.

The normative contract remains in:

1. `spec/drp2/`
2. `spec/scene/`
3. `spec/scene/headers/`

The decision history remains in `agents/now/SCENE_DRP2_SPEC_DECISIONS.md`.


## Goal

Implement the first scene-to-DRP2 path in small slices:

1. deterministic DRP2 command-stream data structures,
2. deterministic `FramePlan` debug/test serialization,
3. scene-to-DRP2 converter fixtures,
4. a Vulkan DRP2 runtime implemented on top of `vklite`,
5. canvas integration through the existing draw callback and stream/sink submission path.

The first working path should render simple built-in visuals and support offscreen/readback without
duplicating the canvas, stream, sink, swapchain, or vklite subsystems.


## Non-Goals

Do not implement these in the first slice:

1. a separate presentation system,
2. a second stream/sink framework,
3. direct scene ownership of `DvzCanvas`, swapchain, stream, or sink internals,
4. a Vulkan abstraction below or beside `vklite`,
5. custom visuals,
6. WBOIT,
7. volume, glyph, sphere, path, marker, statistical visual families,
8. browser/WebGPU runtime,
9. public installed production headers for scene/DRP2 before the module shape is proven.


## Existing Stack To Reuse

Use the active low-level stack as-is unless implementation exposes a concrete missing accessor.

1. `canvas`
   - owns window/surface/swapchain/offscreen frame resources,
   - exposes `dvz_canvas_set_draw_callback()`,
   - drives frame acquisition through `dvz_canvas_frame()`,
   - submits completed frames through `dvz_canvas_submit()`.
2. `stream`
   - owns sink attachment, start/update/submit/stop behavior,
   - routes submitted frames to swapchain, video, live-image, or offscreen sinks.
3. `video`
   - remains a sink attached by the canvas/application path, not by scene code.
4. `vk`
   - provides public low-level device, queue, memory, and surface primitives.
5. `vklite`
   - is the Vulkan wrapper used by the DRP2 runtime for buffers, images, shaders, descriptors,
     render passes/dynamic rendering, graphics pipelines, compute pipelines, command buffers, and
     submission.

Scene code may target runtime-facing abstractions, but the native runtime implementation should use
`vklite` and public `vk` APIs. It should not include `src/vk/_*.h`.


## Ownership Model

Keep ownership split as follows:

1. application owns `DvzCanvas`,
2. canvas owns stream/sinks and presentation/offscreen frame resources,
3. application creates `DvzRuntime` from existing low-level objects,
4. runtime owns DRP2 execution caches and backend resources it creates,
5. scene owns semantic scene objects, validation/adaptation state, and `FramePlan`,
6. scene emits `DvzDrp2CommandStream`,
7. runtime executes the command stream into the current canvas frame,
8. canvas submits the frame to the stream/sink path.

Per-frame target flow:

```text
dvz_canvas_frame(canvas)
  -> canvas acquires or prepares DvzStreamFrame
  -> canvas invokes draw callback
  -> application/runtime builds or receives scene FramePlan
  -> scene-to-DRP2 converter emits DvzDrp2CommandStream
  -> DRP2 Vulkan runtime records into frame->command_buffer or runtime-owned commands
  -> draw callback returns
dvz_canvas_submit(canvas)
  -> stream submits wait value to attached sinks
```

The scene never submits sinks directly.


## Module Bring-Up Order

### Phase 0 - Keep Specs Green

Before implementation slices:

1. run `just spec-check`,
2. keep `spec/drp2/COMMANDS.md`, schemas, fixtures, and `tools/drp2_fixture_runner.py` aligned,
3. use manual stale-term cleanup when editing specs.

Exit criteria:

1. `just spec-check` passes,
2. no implementation patch changes active spec semantics without updating fixtures or docs.


### Phase 1 - DRP2 Command Stream Core

Add a new DRP2 module only for backend-agnostic command data and validation helpers.

Initial files:

1. `include/datoviz/drp2.h`
2. `include/datoviz/drp2/*.h`
3. `src/drp2/`
4. `src/drp2/tests/`
5. `src/drp2/CMakeLists.txt`

Initial API shape:

```c
typedef struct DvzDrp2CommandStream DvzDrp2CommandStream;
typedef struct DvzDrp2Command DvzDrp2Command;

DvzDrp2CommandStream* dvz_drp2_stream(void);
void dvz_drp2_stream_destroy(DvzDrp2CommandStream* stream);
uint32_t dvz_drp2_stream_count(const DvzDrp2CommandStream* stream);
const DvzDrp2Command* dvz_drp2_stream_get(
    const DvzDrp2CommandStream* stream, uint32_t index);
```

Implementation rules:

1. use an owned append-only C object,
2. represent commands as tagged C structs/unions matching active DRP2 `2.0`,
3. make JSON serialization a debug/test path, not the primary in-memory model,
4. keep all types backend-agnostic,
5. do not include Vulkan or vklite headers in the public DRP2 command-stream API.

Tests:

1. stream create/destroy/count/get,
2. append representative resource, pass, draw, submit, and readback commands,
3. deterministic JSON emission for a small stream,
4. invalid index/null handling.

Validation:

1. `just build`
2. focused `drp2` tests once wired,
3. `just spec-check`


### Phase 2 - Scene FramePlan Test Surface

Add only the minimal scene producer surface needed for converter fixtures.

Initial files:

1. `src/scene/`
2. `src/scene/tests/`
3. `src/scene/CMakeLists.txt`
4. public draft headers only when the C API shape has a compiled implementation target.

Initial implementation scope:

1. `DvzCapabilitySnapshot` storage/copy helpers,
2. one diagnostic report type shared by validation, adaptation, planning, and runtime mapping,
3. opaque `DvzFramePlan`,
4. ordered node list with read/write sets,
5. JSON debug serialization matching `spec/scene/pipeline/FRAME_PLAN_SERIALIZATION.md`.

Minimum node kinds:

1. `UPLOAD`,
2. `COMPUTE`,
3. `RENDER`,
4. `COPY`,
5. `READBACK`.

Tests:

1. build and serialize a static one-panel plan,
2. build and serialize a dynamic update plan,
3. build and serialize picking readback and offscreen readback plans,
4. verify deterministic node ordering and stable logical ids.

Validation:

1. `just build`
2. focused scene tests once wired
3. `just spec-check`


### Phase 3 - Scene-To-DRP2 Converter Fixtures

Implement the converter before GPU execution.

Initial API shape:

```c
DvzDrp2CommandStream* dvz_frame_plan_emit_drp2(
    const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report);
```

Converter responsibilities:

1. allocate deterministic DRP2 logical ids,
2. map scene resource keys to DRP2 buffers/textures/samplers/views,
3. emit shader modules and pipelines from deterministic shader keys,
4. emit bind-group layouts and bind groups,
5. emit command encoders, passes, draw/dispatch/copy commands, and queue submit,
6. emit readback requests and route reply metadata to scene-visible request ids.

Modes:

1. fixture mode: self-contained stream with all required object creation,
2. runtime mode: persistent converter cache that omits already-live resources and pipelines.

Runtime emitter cache limitation:

1. The current runtime-mode `DvzFramePlanEmitter` is a vertical-slice prototype, not a production
   emitter contract.
2. Its boolean cache fields only track one shader, pipeline, bind-group layout, bind group,
   sampler, color target, and readback buffer per category.
3. Real `FramePlan` instances are expected to support multiple visuals, panels, resources,
   render pipelines, compute pipelines, bind groups, samplers, render targets, and readbacks.
4. Before building real scene objects on top of runtime-mode emission, replace the single-object
   booleans with keyed caches:
   - resources keyed by scene resource key plus usage and shape,
   - shaders keyed by shader key, stage, variant flags, and source format,
   - render pipelines keyed by visual family, vertex layout, attachment formats, blend/topology,
     and bind-group layouts,
   - compute pipelines keyed by shader, layout, and specialization state,
   - bind-group layouts keyed by binding schema,
   - bind groups keyed by layout plus resource, sampler, range, and offset bindings,
   - transient command encoders, passes, and submissions allocated from per-frame transient ids.
5. The current tests prove only one static, texture, or compute-assisted toy path across frames.
   They should not be interpreted as a one-object-per-type FramePlan constraint.

First converter fixtures:

1. static point or pixel,
2. dynamic buffer update,
3. image sampling,
4. point or pixel picking readback,
5. offscreen image readback,
6. compute-assisted path followed by render.

Tests:

1. each fixture emits deterministic DRP2 JSON,
2. emitted JSON validates with `tools/drp2_fixture_runner.py`,
3. emitted commands match expected command categories and resource ids,
4. converter diagnostics explain unsupported capability cases.

Validation:

1. converter unit tests,
2. `python3 tools/drp2_fixture_runner.py` on emitted streams,
3. `just spec-check`.


### Phase 4 - DRP2 Runtime Semantic Layer

Implement backend-agnostic runtime state before Vulkan execution.

Responsibilities:

1. object registry for DRP2 ids,
2. object kind and lifetime checks,
3. command stream state tracking,
4. submission/readback request tracking,
5. mapping from DRP2 error codes to scene diagnostics,
6. capability validation using `DvzCapabilitySnapshot`.

This layer should reuse the active fixture-runner semantics but should be C code, not a Python port
hidden in production.

Tests:

1. positive streams accepted,
2. representative negative streams rejected with expected phase/code,
3. use-after-destroy and duplicate-id failures,
4. queue-submit/readback state failures.

Validation:

1. focused DRP2 runtime semantic tests,
2. `just spec-check`.


### Phase 5 - Vulkan DRP2 Runtime On vklite

Implement native execution behind the DRP2 runtime using `vklite`.

Initial files:

1. `src/drp2/runtime_vklite*.c`
2. private runtime headers under `src/drp2/`
3. focused tests under `src/drp2/tests/`

Runtime mapping:

1. DRP2 buffers -> `vklite` buffer wrappers,
2. DRP2 textures/views -> `vklite` image/image-view wrappers,
3. DRP2 samplers -> `vklite` sampler wrappers,
4. DRP2 bind-group layouts/groups -> `vklite` slots/descriptors,
5. DRP2 shader modules -> `vklite` shader wrappers,
6. DRP2 render pipelines -> `vklite` graphics wrappers,
7. DRP2 compute pipelines -> `vklite` compute wrappers,
8. DRP2 command encoders/passes -> `vklite` command recording helpers.

Capability mapping:

1. query public `vk`/`vklite` device capabilities,
2. fill `DvzCapabilitySnapshot`,
3. advertise WGSL only when a WGSL path exists,
4. advertise GLSL/SPIR-V only when the native path can compile or ingest them,
5. derive WBOIT availability from lower-level fields, not from a boolean.

First execution target:

1. static point or pixel to an offscreen/canvas target,
2. clear plus simple draw,
3. copy/readback into a buffer,
4. no WBOIT, custom visuals, or multi-family shader registry yet.

Validation:

1. focused vklite-backed DRP2 runtime tests,
2. `direnv exec . just test vklite`,
3. `direnv exec . just test canvas`,
4. `just spec-check`.


### Phase 6 - Canvas Integration

Integrate the runtime with existing canvas frame ownership.

Rules:

1. canvas remains the presentation owner,
2. runtime may be attached to the canvas draw callback by the application,
3. runtime records into the current frame command buffer or wraps it through `vklite`,
4. runtime must respect `DvzStreamFrame` handles and dirty-handle metadata,
5. canvas remains responsible for stream/sink submission.

Public-facing sketch:

```c
DvzCanvas* canvas = dvz_canvas_create(&canvas_cfg);
DvzRuntime* runtime = dvz_runtime_create_for_canvas(canvas, &runtime_cfg);
DvzScene* scene = dvz_scene_create(&scene_cfg);
DvzRenderTarget* target = dvz_runtime_target_canvas(runtime, canvas);

dvz_figure_set_target(fig, target);
dvz_canvas_set_draw_callback(canvas, draw_scene, app);

while (running)
{
    if (dvz_canvas_frame(canvas) == DVZ_CANVAS_FRAME_READY)
        dvz_canvas_submit(canvas);
}
```

The exact function names may change during implementation, but the ownership must not.

Tests:

1. draw callback can submit one runtime-rendered frame,
2. swapchain sink path still works,
3. offscreen sink path still works,
4. video/live-image sinks remain canvas/stream concerns,
5. runtime teardown does not outlive borrowed canvas frame handles.

Validation:

1. `direnv exec . just test canvas`
2. focused DRP2 runtime tests
3. full `just test` when integration crosses modules


## CMake And Test Wiring

When modules are activated:

1. add `src/drp2/CMakeLists.txt` with `add_library(datoviz_drp2 OBJECT ...)`,
2. add `src/scene/CMakeLists.txt` only when compiled scene code exists,
3. add both modules to `src/CMakeLists.txt`,
4. link them into `datoviz`,
5. add tests under `src/drp2/tests/` and `src/scene/tests/`,
6. expose `test_drp2(TstSuite* suite)` and `test_scene(TstSuite* suite)`,
7. add them to `testing/dvztest.c`,
8. add focused runners only when they materially speed iteration.

Use `${DVZ_COMPILE_DEFINITIONS}`, `${PROJECT_SOURCE_DIR}/include`, and
`${PROJECT_SOURCE_DIR}/src/common` consistently.


## First Vertical Slice Definition

The first vertical slice is complete when:

1. a minimal scene fixture builds a `FramePlan`,
2. the converter emits a valid DRP2 command stream,
3. the DRP2 fixture runner accepts the emitted JSON,
4. the vklite runtime can execute an equivalent stream into an offscreen or canvas target,
5. canvas submits the resulting frame through the existing stream/sink path,
6. a readback or capture test proves bytes came through the existing runtime path.


## Validation Matrix

Use the narrowest loop that covers the slice:

| Slice | Validation |
|---|---|
| spec-only changes | `just spec-check` |
| DRP2 command stream | focused DRP2 tests, `just spec-check` |
| scene FramePlan/converter | focused scene tests, converter fixture validation, `just spec-check` |
| vklite runtime | focused DRP2 runtime tests, `direnv exec . just test vklite` |
| canvas integration | `direnv exec . just test canvas` |
| cross-module changes | `just build`, relevant focused tests, then `just test` when practical |

On Vulkan/presentation paths, use `direnv exec .` so the repository Vulkan environment is active.


## Open Implementation Risks

These are not spec blockers, but implementation should resolve them explicitly:

1. whether the runtime records directly into the canvas-provided `VkCommandBuffer` or records into
   runtime-owned command buffers and synchronizes with the canvas frame,
2. what public canvas/runtime accessor is needed to avoid reading canvas internals,
3. how GLSL-to-SPIR-V compilation is exposed for runtime use outside the existing vklite test-shader
   path,
4. how readback buffers are staged and retained until completion is consumed,
5. whether initial scene code lands as public installed API immediately or stays as compiled
   internal/draft headers until the first slice is stable.

Resolve these with narrow implementation decisions when each risk becomes concrete.
