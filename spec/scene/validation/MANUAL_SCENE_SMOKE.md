# Manual Scene Smoke Matrix

This checklist records the manual validation surface for the active v0.4 scene -> DRP2 -> app
path. It complements focused tests; it should stay short enough to run before broadening scene,
request, or app behavior.

Latest recorded smoke note: on `2026-05-14`, the previous C example smoke set and
`techniques/image_probe 300` were reported to run successfully. OS/GPU/backend details were not
recorded; add them here if a later run exposes a platform-specific anomaly.

Run from the repository root after a successful build:

```bash
just build
```

For Vulkan/GLFW runs, use the same unsandboxed environment expected by the repo graphics stack. On
macOS, prefer:

```bash
direnv exec . just example-c <name>
direnv exec . ./build/examples/c/<name>
```


## Interactive Cases

### 2D panzoom

Command:

```bash
./build/examples/c/visuals/point
```

Expected behavior: three points render. Left-drag pans, right-drag or scroll zooms,
double-click resets. Resize keeps the panel background and points coherent.

Automated coverage: `test_panzoom_create_reset`, `test_panzoom_pan_shift`,
`test_panzoom_zoom_wheel`, `test_panzoom_double_click_resets`, `test_panzoom_mvp_identity`.

Current gap: no automated live GLFW gesture test.


### Hover point picking

Command:

```bash
./build/examples/c/techniques/pick_hover
```

Expected behavior: a point grid renders. Moving the cursor over points grows the frontmost hovered
point. Panzoom still works while hover requests continue across frames.

Automated coverage: `test_scene_process_pick_probe_requests`, `test_scene_point_pick_quadrants`,
`test_scene_process_requests_coalesces_pending_picks_before_execution`,
`test_scene_pick_request_same_id_rejects_late_result_after_newer_poll`.

Current gap: manual check is still needed for real pointer coordinates and visible hover feedback.


### 3D mesh arcball/depth

Command:

```bash
./build/examples/c/visuals/mesh
```

Expected behavior: a lit cube renders with correct depth ordering. Idle rotation advances through
the frame callback. Dragging rotates with arcball without fighting idle motion. Resize keeps the
cube framed.

Automated coverage: `test_app_offscreen_lit_primitive_depth_orders_overlap`,
`test_app_offscreen_mesh_renders_nonblank`, `test_app_offscreen_rotated_mesh_depth_orders_faces`,
`test_app_offscreen_camera_arcball_mesh_renders_cube`, and the `test_arcball_*` group.

Current gap: live capture/readback is not part of this GLFW example; offscreen `visuals/mesh.c`
covers capture.


### 3D mesh WBOIT transparency

Command:

```bash
./build/examples/c/techniques/wboit
```

Bounded validation smoke:

```bash
./build/examples/c/techniques/wboit 2
```

Expected behavior: one lit, transparent teal WBOIT cube renders on a dark background between opaque
reference cards. The large rear card should remain visible through the cube, while the narrow front
strip should occlude the cube where it is in front. The cube should show face-to-face lighting
variation and visible overlap rather than one flat solid color. A GUI overlay named `WBOIT cube`
exposes a Light background toggle plus live Red, Green, Blue, Alpha, Ambient, Diffuse, and Light
X/Y/Z sliders for visual tuning. Idle rotation advances through the scene clock, arcball drag
remains interactive, and Vulkan validation stays quiet for the bounded smoke.

For command inspection, run the bounded smoke with full labeled DRP2 trace:

```bash
DVZ_DRP2_TRACE=full DVZ_DRP2_TRACE_COLOR=0 ./build/examples/c/techniques/wboit 2
```

The trace should show scene labels on ids. The opaque pass targets `rt` with `depth=yes`; the fixed
background pipeline has `write=no compare=7` (`ALWAYS`); the opaque reference pipeline has
`write=yes compare=3` (`LESS_OR_EQUAL`); the WBOIT accumulation pass targets `_wboit_accum_*` with
`depth=yes write=no`; and the resolve pass targets `rt` with `depth=no`.

Automated coverage: `test_scene_visual_alpha_mode_standard_blend`,
`test_scene_visual_alpha_mode_emits_wboit_drp2`,
`test_scene_visual_alpha_mode_wboit_glsl_executes`,
`test_drp2_wboit_accumulation_resolve_stream`, and DRP2 runtime multi-color render-pass tests.

Current gap: visual polish and offscreen WBOIT capture/readback coverage are still minimal.


### App trace/status

Command:

```bash
DVZ_DRP2_TRACE=normal ./build/examples/c/visuals/mesh 120
```

Expected behavior: changed frames print a full command stream, unchanged repeated frames stay to a
compact status line, and the live example still renders and exits after the bounded frame count.
Trace colors are enabled by default; use `NO_COLOR=1` or `DVZ_DRP2_TRACE_COLOR=0` to disable trace
colors, and `DVZ_DRP2_TRACE_COLOR=1` to force them. The general Datoviz logger follows the same
runtime policy with `DVZ_LOG_COLOR=0` and `DVZ_LOG_COLOR=1`.

The printed stream should keep resource/object commands outside the render-pass scope: creation,
upload, bind-group, shader, and pipeline commands appear before `BeginCommandEncoder` /
`BeginRenderPass`, while `Set*` and `Draw*` commands appear inside the pass.

Automated coverage: `test_app_trace_*`, `test_app_status_line_combines_trace_and_fps`.
Ordering coverage: `test_scene_render_pass_scope_excludes_resource_commands`.

Current gap: manual terminal inspection is still the easiest way to catch noisy trace output.


## Offscreen And Bounded Cases

### Basic point capture

Command:

```bash
./build/examples/c/visuals/point
```

Expected behavior: saves `visuals/point.png`; image is nonblank and contains the three colored
points.

Automated coverage: `test_app_offscreen_has_nonblank_pixels`.

Current gap: manual PNG inspection only; no example-level verifier.


### Image texture capture

Command:

```bash
./build/examples/c/visuals/image
```

Expected behavior: saves `visuals/image.png`; procedural image is visible on a quad.

Automated coverage: `test_app_offscreen_image_has_nonblank_pixels`,
`test_app_offscreen_image_retained_render_second_frame`.

Current gap: no manual partial-field-update example yet.


### Sampled field capture

Command:

```bash
./build/examples/c/visuals/image
```

Expected behavior: saves `visuals/image.png`; scalar field appears with the bound colormap.

Automated coverage: `test_scene_image_visual_binds_colormap_scale`,
`test_scene_image_scalar_texture_uses_bound_scale`,
`test_app_offscreen_image_field_partial_update_changes_region`.

Current gap: no manual partial-field-update example yet.


### 3D mesh capture

Command:

```bash
./build/examples/c/visuals/mesh
```

Expected behavior: saves `visuals/mesh.png`; cube is visible with perspective, arcball transform,
shading, and depth.

Automated coverage: `test_app_offscreen_mesh_renders_nonblank`,
`test_app_offscreen_rotated_mesh_depth_orders_faces`,
`test_app_offscreen_camera_arcball_mesh_renders_cube`.

Current gap: separate from live GLFW arcball smoke.


### Multi-panel render

Command:

```bash
./build/examples/c/techniques/multi_panel 300
```

Expected behavior: four panels render in their own quadrants with separate backgrounds and visual
content. Pan/zoom gestures affect only the panel under the cursor, preserving viewport/scissor
isolation across the other panels.

Automated coverage: `test_app_offscreen_two_panel_points_light_both_halves`,
`test_scene_multiple_panels_multiple_point_visuals_emit`,
`test_scene_multi_panel_glsl_emits_viewport_scissor_commands`.

Current gap: manual gesture inspection is still needed for live per-panel controller routing.


### Linked panel panzoom

Command:

```bash
./build/examples/c/techniques/linked_panels 300
```

Expected behavior: the two top panels render different point grids but share pan/zoom state.
Pan or wheel-zoom either top panel and the other top panel follows; the bottom panel remains
independent. The terminal prints whether a linked or independent panel changed.

Automated coverage: `test_panel_panzoom_getter`,
`test_panzoom_viewport_filters_pointer_events`,
`test_scene_multi_panel_glsl_emits_viewport_scissor_commands`.

Current gap: manual gesture inspection is still needed to confirm live linked-panel propagation.


### Partial texture update

Command:

```bash
./build/examples/c/visuals/image 300
```

Expected behavior: a scalar sampled field renders as a colormapped image. A bright square patch
moves around the field, and the terminal prints the updated subregion every few frames.

Automated coverage: `test_app_offscreen_image_field_partial_update_changes_region`,
`test_scene_image_field_partial_update_emits_texture_subregion`,
`test_scene_shared_field_mixed_full_and_partial_uploads`.

Current gap: manual inspection is still needed to confirm the visible live patch motion.


### Image probe request

Command:

```bash
./build/examples/c/techniques/image_probe 300
```

Expected behavior: a non-uniform image renders. Moving the cursor across quadrants prints changing
RGBA probe values from the live app frame loop. Moving outside the image prints misses when the
request resolves outside the textured quad.

Automated coverage: `test_scene_image_probe_respects_panel_request_position`,
`test_scene_image_probe_transparent_pixel_misses`,
`test_scene_image_probe_gpu_readback_failure_misses`,
`test_scene_process_requests_coalesces_pending_probes_before_execution`.

Current gap: manual pointer movement is still needed to confirm visible live coordinates and
terminal output together.


## Recommended Manual Pass

For a normal scene/app smoke pass, run:

```bash
just build
just test test_app_offscreen_lit_primitive_depth_orders_overlap
just test test_scene_process_pick_probe_requests
./build/examples/c/visuals/point
./build/examples/c/visuals/point
./build/examples/c/visuals/primitive
./build/examples/c/visuals/path
./build/examples/c/visuals/image
./build/examples/c/visuals/image
./build/examples/c/visuals/mesh
./build/examples/c/visuals/point
./build/examples/c/techniques/pick_hover
./build/examples/c/visuals/mesh
./build/examples/c/techniques/wboit 2
```

Record OS, GPU, backend, command, observed behavior, and whether Vulkan validation layers were
enabled for any anomaly. Convert deterministic manual failures into focused tests before adding new
visual families or broadening the public scene API.
