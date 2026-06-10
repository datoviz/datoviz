# WASM/WebGPU Parity Plan

Status: active v0.4 RC plan. Updated: 2026-06-10.

This file defines the route from the current experimental WASM/WebGPU subset to "good enough" v0.4
RC browser support. It is not a full Vulkan/WebGPU parity promise.


## Goal

Datoviz should support write-once scene examples:

```text
portable C scene scenario
  -> native host: app/window/capture/Vulkan
  -> browser host: WASM scene ABI -> DRP2 packets -> WebGPU canvas
```

The parity boundary is the scene and DRP2 contract. Browser JavaScript may load WASM, normalize
browser input, execute DRP2 packets, surface diagnostics, and manage the canvas. It must not
reimplement scene construction, visual state, animation, picking, selection, query/probe, or data
semantics.


## Current State

Current source-of-truth files:

1. `examples/c/MANIFEST.yaml`: example classification and live routes.
2. `examples/webgpu/live_examples.js`: public live route registry.
3. `examples/webgpu/COMPAT.md`: evidence ledger.
4. `docs/reference/webgpu-subset.md`: public supported subset.
5. `docs/examples/webgpu-matrix.md`: generated public matrix.

As of 2026-06-10:

1. Manifest counts: `65 webgpu-live`, `17 webgpu-planned`, `11 webgpu-deferred`,
   `13 native-only`, and `8` lab-only entries without WebGPU status.
2. Live routes cover basic scene, timer animation, triangulation, builtin shapes 2D/3D, isolines,
   animation tracks, OBJ loading, picking, pixel/sphere/mesh selection, image probe, compute buffer
   animation, GPU particle smoke, standalone visual-family routes, panel single/grid/multi/linked
   basics, panzoom, axes, text block, overlay card, guide lines/spans, bars/bands, sampled-field
   and colormap-scale routes, panel background, fly/turntable/orbit controller examples, colorbar,
   scale bars, categorical legend, annotation readout, linked probe/colorbar, scientific plotting,
   vector, wind field, polygon composite, linked panels with axes, scale-bar measurement workflow,
   surface-grid showcase, U.S. state choropleth, retained data update/visibility routes,
   depth-test route, and material/lighting routes.
3. The browser runtime consumes artifact-backed split DRP2 setup/update/frame packets. JSON is
   debug/fixture-only.
4. Query/readback is intentionally narrow: point/marker picking, point hover/selection,
   pixel/sphere/mesh selection, and one sampled image probe.
5. Compute-to-render is proven by `feature_compute_buffer_animation` and the browser
   `showcase_gpu_particle_smoke` route, which uses a `32k` browser particle budget.

Current browser-supported building blocks:

| Area | Current support |
| --- | --- |
| Transport | artifact-backed split DRP2 packets, borrowed spans copied/executed before release |
| Visuals | point, pixel, basic marker, segment/path cap-join subset, primitive, RGBA8/scalar image routes, glyph/text, labels, basic/textured/material mesh, sphere, vector lowered through segment/path |
| Layout/annotations | axes/ticks/grid labels, multi/linked panels, colorbar, scale bar, categorical legend, annotation readout, text block, overlay card, guide lines/spans, bars/bands |
| Interaction | panzoom, arcball, fly, turntable, orbit-camera examples, frame callbacks, narrow async query/readback |
| Dynamic retained state | same-shape visual data updates, partial buffer updates, visual visibility changes |
| Depth state | basic point depth-test on/off comparison |
| Material/lighting | material-backed mesh and lit sphere examples |
| Compute | scene buffers, storage buffers, dispatch, `ResourceBarrier`, compute-to-vertex reuse |
| Runtime | WebGPU fixture runner, semantic negative diagnostics, retained browser runtime lifecycle |


## Non-Goals For v0.4

Do not block v0.4 on browser equivalents for:

1. GLFW, native app loops, native swapchains, Vulkan/vklite handles, Qt/PyQt hosting, GUI/debug
   panels, native video encoders, CUDA/Vulkan interop, or platform packaging diagnostics;
2. full native query parity, volume ray-hit picking, advanced postprocess parity, publication
   export, stable JS/TS bindings, or custom browser shader APIs;
3. high-level plotting APIs owned by GSP/VisPy2.


## Guardrails

1. Promoted browser examples must reuse the same canonical C example or portable C scenario as the
   native route.
2. WebGPU must execute DRP2 packets; do not add visual-family-specific browser render paths.
3. Unsupported requirements must fail with deterministic diagnostics.
4. WGSL is required for browser-supported shader paths. Native may keep GLSL/SPIR-V.
5. Scene APIs must not expose Vulkan, WebGPU, browser, GLFW, or app host types.
6. Artifact packet spans are borrowed; JavaScript must copy or execute them before artifact release.
7. A feature is not `webgpu-live` until native/scene evidence, WASM smoke, browser evidence, docs,
   manifest status, and diagnostics are updated.


## RC Good-Enough Bar

WebGPU support is good enough for v0.4 RC when:

1. `showcase_gpu_particle_smoke` stays live in browser WebGPU with its documented `32k` particle
   budget, or an explicit native/browser compute blocker is recorded in `examples/webgpu/COMPAT.md`;
2. request/query/readback remains live for picking, selection, and one image probe;
3. representative live examples exist for every current browser visual family;
4. panel, panzoom, axes, text, image, colorbar, scale-bar, legend, readout, and simple composition
   coverage is broad enough that the generated WebGPU matrix does not look cherry-picked;
5. remaining public examples are honestly classified as `webgpu-planned`, `webgpu-deferred`, or
   `native-only`, with requirement tags explaining why;
6. browser JavaScript remains host glue only.


## Promotion Order

### 1. Compute Particle Proof

Current: `showcase_gpu_particle_smoke` is the live browser compute particle route at `32k`
particles. Keep native Vulkan evidence and any known headless WebGPU instance-loss skips recorded
honestly.

### 2. Simple Visual-Family Batch

Current: standalone browser routes exist for every already-current family:

`point_2d`, `visual_pixel`, `visual_marker`, `visual_primitive`, `visual_segment`, `visual_path`,
`visual_image`, `visual_mesh`, `sphere_impostor`, `visual_text`, `visual_glyph`, and
`visual_labels`.

Keep these routes in smoke coverage while moving promotion effort to composed examples.

### 3. Panel, Annotation, And Controller Basics

Current: panel, annotation, and requested controller examples that compose current primitives are
live:

`feature_panel_single`, `feature_panel_grid`, `feature_panel_multi`, `feature_panel_linked`,
`feature_panzoom`, `path_axes_2d`, `feature_axis_labels`, `feature_text_block`,
`feature_overlay_card`, `feature_guide_lines`, `feature_guide_spans`, `feature_bars_bands`,
`feature_controller_fly`, `feature_controller_turntable`, and
`feature_controller_orbit_camera`.

Keep these routes in smoke coverage while finishing the remaining composed/data-backed routes.

### 4. Composed Showcase Pass

Current: synthetic composed showcase routes that reuse current primitives are live:

`composite_polygon`, `linked_panels_axes_panzoom`, `scalebar_measurement_workflow`,
`showcase_surface_grid`, and `us_state_choropleth`.

Remaining planned composed/data-backed routes:

`textured_terrain_or_planet`, `protein_arcball_viewer`, and `showcase_embedding_atlas` if data
packaging, query/readback, and overlay behavior are stable enough.

### 5. Explicit Deferrals

Keep these out of the RC browser push unless they become release blockers:

native GUI/capture, video/export, GLFW/app, raw vklite/DRP2 examples, CUDA, dense point-cloud GUI
workflows, volume-heavy examples, splats, marker symbol-set atlas parity, and postprocess
techniques.


## Capability Matrix

| Area | RC status | Next action |
| --- | --- | --- |
| Core visual families | current | promote standalone gallery examples for every current family |
| Annotations/layout | current/rc-target | promote composed showcases |
| Query/readback | current narrow slice | keep picking/selection/probe evidence; defer full query parity |
| Compute | compact proof and particle route current | keep particle budget/evidence current |
| Controllers | panzoom/arcball/fly/turntable/orbit examples current | keep broader native controller parity deferred |
| Volume/splat/postprocess | deferred | keep diagnostics explicit; do not expand before RC unless required |


## Promotion Checklist

Use one commit per example or capability cluster.

Required for a `webgpu-live` promotion:

1. reuse/export the canonical C example or portable C scenario;
2. register it in `src/wasm/scene_api.c` and `src/wasm/CMakeLists.txt` if needed;
3. add or verify `examples/webgpu/live_examples.js`;
4. update `examples/c/MANIFEST.yaml` and regenerate generated gallery docs when status changes;
5. add targeted stream-shape coverage in `tools/wasm_scene_smoke.mjs`;
6. add `tools/webgpu_browser_smoke.mjs` coverage only for new capabilities or release blockers;
7. record evidence or honest skips in `examples/webgpu/COMPAT.md`;
8. update `docs/reference/webgpu-subset.md` only when the public supported subset changes;
9. run the validation commands below and finish with `git diff --check`.


## Validation

Documentation-only:

```bash
git diff --check
git status --short
```

Promotion checkpoint:

```bash
node --check <touched-js>
python3 tools/check_example_manifests.py
just wasm-scene-smoke
just webgpu-browser-smoke
git diff --check
```

Native/DRP2 checks when relevant:

```bash
just webgpu-fixture-preflight
just webgpu-runner-smoke
just drp2-fixtures
./build/testing/dvztest scene/frame-plan-emit/drp2_compute_assisted
./build/testing/dvztest scene/scene-graph/compute_point_position_buffer_emits_drp2
```

Native GPU evidence when Vulkan is available:

```bash
direnv exec . ./build/testing/dvztest scene/frame-plan-emit/runtime_compute_two_frames_glsl_executes
direnv exec . just example-c showcases/gpu_particle_smoke 120
```

Headless Chrome/Dawn may skip live-route rendering with the known external WebGPU instance-loss
diagnostic before scene rendering. Record that as a skip, not as visual proof.


## References

1. [WEBGPU_WASM.md](WEBGPU_WASM.md)
2. [WEBGPU_EXAMPLE_CONTINUATION_PLAN.md](WEBGPU_EXAMPLE_CONTINUATION_PLAN.md)
3. [../examples/PORTABLE_SCENARIO_RUNNER.md](../examples/PORTABLE_SCENARIO_RUNNER.md)
4. [../../drp2/PACKETS.md](../../drp2/PACKETS.md)
5. [../../drp2/roadmap/WEBGPU.md](../../drp2/roadmap/WEBGPU.md)
6. [../../../docs/reference/webgpu-subset.md](../../../docs/reference/webgpu-subset.md)
7. [../../../examples/webgpu/COMPAT.md](../../../examples/webgpu/COMPAT.md)
