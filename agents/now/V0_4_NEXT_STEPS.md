# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT GUIDE`
> - **Updated on:** `2026-05-06`
> - **Purpose:** orient near-term v0.4 work after the first scene -> DRP2 -> vklite/canvas slice.


## Current Position

Datoviz v0.4 is no longer only a low-level refactor branch.

Focused validation on `2026-05-06`:

1. `just spec-check`: `119/119` DRP2 fixtures passed; `52` fixture-runner tests passed.
2. `just test drp2`: `73/73` tests passed.
3. `just test scene`: `52/52` tests passed.
4. `git diff --check`: passed for this documentation update.

Recent follow-up validation on `2026-05-06`:

1. `just test scene`: passed after explicit clear-plan and retained-render updates.
2. `just test vklite_swapchain`: passed with automated present windows hidden by default.
3. `DVZ_TEST_VISIBLE=1 just test vklite_swapchain`: was terminated after hanging in visible
   compositor-dependent present execution; keep visible mode manual/debug-only.

The low-level stack is now the foundation:

1. `vk` owns low-level Vulkan instance/device/queue/memory primitives.
2. `vklite` owns higher-level Vulkan wrappers for buffers, images, commands, graphics, descriptors,
   compute, rendering, swapchain, and sync.
3. `canvas` owns frame acquisition, borrowed frame command buffers, swapchain/offscreen targets, and
   stream submission.
4. `stream` and sinks route frames to swapchain, offscreen, live image, and video consumers.

The active higher layer now exists:

1. `drp2` owns backend-agnostic command streams, JSON/debug serialization, validation, and the native
   vklite runtime.
2. `scene` owns early scene graph objects, capability snapshots, diagnostic reports, frame plans,
   DRP2 emission, and a minimal app/offscreen path.
3. Current examples cover `hello_point`, `hello_scatter`, `raw_triangle`, and `raw_triangle_drp2`.
4. Empty scene panels now emit explicit clear-only FramePlan nodes instead of relying on an empty render
   node convention, and the runtime path can render retained point buffers on later frames with no dirty
   uploads.
5. Focused scene tests now cover repeated partial point updates across emitted frames, multi-panel /
   multi-point visual emission in one figure, direct live-stream destroy guards, and per-panel runtime
   render regions in the emitted DRP2 path.
6. The app/offscreen scene path now preserves prior panel contents correctly across later `LOAD`
   render passes and validates a two-panel red/green offscreen render in the unified scene suite.
7. Automated GLFW canvas and vklite present tests create hidden windows by default; set
   `DVZ_TEST_VISIBLE=1` only for local visual debugging.


## Best Next Development Steps

### 1. Finish the point-based scene slice properly

Before adding many visual families, make the existing point path feel production-shaped:

1. make attribute names, data shapes, and error messages strict and documented,
2. expand deterministic offscreen capture smoke tests beyond the current retained-render point case,
3. keep the borrowed-pointer lifetime contract around emitted streams explicit.

### 2. Add the next minimal visual family

The best next visual is a small triangle/mesh visual, not a broad renderer.

Target API shape:

```c
DVZ_EXPORT DvzVisual* dvz_triangle(DvzScene* scene, uint32_t flags);
```

or, if the implementation naturally wants the durable family name:

```c
DVZ_EXPORT DvzVisual* dvz_mesh(DvzScene* scene, uint32_t flags);
```

Keep the first version narrow:

1. positions,
2. per-vertex color,
3. optional index buffer only if it does not complicate the first tests,
4. one pipeline shape,
5. one offscreen example.

Exit criteria:

1. focused scene tests,
2. DRP2 fixture or JSON assertion for the emitted stream,
3. a C example that saves an image,
4. no new scene dependency on Vulkan headers outside runtime-facing code.

### 3. Add a minimal image/texture visual after mesh

Once mesh/triangle exists, add image as the second non-point family.

Keep it constrained:

1. 2D RGBA8 texture upload,
2. a quad with generated positions/UVs,
3. one sampler mode,
4. one offscreen example,
5. a clear resource ownership story for CPU image bytes and runtime texture ids.

This will pressure-test DRP2 texture upload, texture views, samplers, bind groups, and scene-side
resource identity more usefully than adding more point variants.

### 4. Harden the scene/DRP2 runtime boundary

The next runtime work should be failure and lifetime oriented:

1. repeated frame-target attach/detach across frames,
2. runtime destroy after partial execution failure,
3. command-stream execution after scene-side data mutation is rejected or clearly documented,
4. GL shader compile failure paths in examples and runtime tests,
5. readback/capture paths with explicit layout and byte-size validation.

Use [done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
as the safety baseline.

### 5. Keep examples as the API pressure test

Near-term examples should stay small and honest:

1. `hello_point` and `hello_scatter`: high-level scene/app path.
2. `hello_triangle` or `hello_mesh`: next scene visual once implemented.
3. `hello_texture`: first texture/sampler scene visual once implemented.
4. `raw_triangle`: vklite into canvas for users who need low-level control.
5. `raw_triangle_drp2`: hand-written DRP2 for protocol/runtime developers.

Do not add examples that require broad axes/controllers/layout behavior until those APIs exist.


## Validation Defaults

For docs-only or spec-only changes:

```bash
git diff --check
just spec-check
```

For scene/DRP2 CPU-surface changes:

```bash
just build
just test drp2
just test scene
just spec-check
git diff --check
```

For changes touching borrowed canvas frames, command buffers, render targets, synchronization, or
presentation:

```bash
just build
just test drp2
just test scene
just test canvas
just test vk
just spec-check
git diff --check
```


## Do Not Reopen By Default

1. Do not reintroduce a parallel presentation path for scene.
2. Do not let scene own swapchain, command-buffer begin/end, sink submission, or Vulkan synchronization.
3. Do not expand DRP2 toward WebGPU transport before the native vklite runtime is more complete.
4. Do not activate dormant modules such as `color`, `wasm`, or broad renderer/client layers unless the
   task explicitly asks for them.
5. Do not carry v0.3 compatibility constraints into this branch.
