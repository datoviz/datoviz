# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT GUIDE`
> - **Updated on:** `2026-05-07`
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
3. Current examples cover `hello_point`, `hello_scatter`, `hello_triangle`, `hello_texture`, `raw_triangle`, and `raw_triangle_drp2`.
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

### 1. Finish the point-based scene slice properly — DONE

1. `_attr_item_size` is now type-specific: `dvz_point` accepts `position/color/size`,
   `dvz_primitive` accepts `position/color`, `dvz_image` accepts `position/texcoords`. Cross-type
   attribute names (e.g. "texcoords" on a point visual) are rejected with a clear error message
   naming the accepted set. Tests: `test_scene_point_rejects_texcoords_attribute`,
   `test_scene_primitive_rejects_size_attribute`, `test_scene_image_rejects_size_attribute`.
2. Visuals with no `position` data are detected at emit time, warned, and skipped cleanly
   (emit returns a valid stream; the visual renders nothing). Test:
   `test_scene_emit_warns_visual_with_no_position`.
3. Offscreen smoke tests expanded: `test_app_offscreen_image_has_nonblank_pixels` (red pixel
   check for dvz_image), `test_app_offscreen_image_retained_render_second_frame` (texture
   data retained across frame 1 → frame 2 without re-upload).
4. The borrowed-pointer lifetime contract was already documented in `include/datoviz/scene.h`
   on `dvz_figure_emit` / `dvz_figure_emit_ex`.

### 2. dvz_primitive — DONE

A topology-parametric `primitive` visual family is now in place. `dvz_primitive(scene, topology,
flags)` accepts `POINT_LIST`, `LINE_LIST`, `LINE_STRIP`, `TRIANGLE_LIST`, `TRIANGLE_STRIP` (no
`TRIANGLE_FAN`, per `spec/scene/visuals/PRIMITIVE.md`). Built-in pass-through pos+color shaders,
2-binding pipeline, topology forwarded to `VkPipelineInputAssemblyStateCreateInfo`. Scene tests
cover triangle-list and line-strip emit; `examples/c/hello_triangle.c` saves a colored triangle
PNG identical to `raw_triangle.c`. Heavier mesh concerns (indexing, normals, lighting) still
belong to a later `dvz_mesh` family.

### 3. Add a minimal image/texture visual — DONE

`dvz_image(scene, flags)` is in place as the second non-point family. Attributes: `"position"`
(four clip-space corners, TRIANGLE_STRIP order), `"texcoords"` (four UV pairs), texture supplied
via `dvz_visual_set_texture(visual, pixels, w, h)`. Sampler and bind group emission live in
`src/scene/converter.c`. Scene tests cover CPU-side emit and app-layer pixel readback
(`test_scene_image_emit`, `test_app_offscreen_image_has_nonblank_pixels`). Example:
`examples/c/hello_texture.c`.

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

1. `hello_point` and `hello_scatter`: high-level scene/app path with `dvz_point`.
2. `hello_triangle`: `dvz_primitive` with `TRIANGLE_LIST` topology.
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
