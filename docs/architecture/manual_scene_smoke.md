# Manual Scene Smoke Matrix

This checklist records the manual validation surface for the active v0.4 scene -> DRP2 -> app
path. It complements focused tests; it should stay short enough to run before broadening scene,
request, or app behavior.

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
./build/examples/c/hello_point_glfw
```

Expected behavior: three points render. Left-drag pans, right-drag or scroll zooms,
double-click resets. Resize keeps the panel background and points coherent.

Automated coverage: `test_panzoom_create_reset`, `test_panzoom_pan_shift`,
`test_panzoom_zoom_wheel`, `test_panzoom_double_click_resets`, `test_panzoom_mvp_identity`.

Current gap: no automated live GLFW gesture test.


### Hover point picking

Command:

```bash
./build/examples/c/hello_pick_hover_glfw
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
./build/examples/c/hello_mesh_glfw
```

Expected behavior: a lit cube renders with correct depth ordering. Idle rotation advances through
the frame callback. Dragging rotates with arcball without fighting idle motion. Resize keeps the
cube framed.

Automated coverage: `test_app_offscreen_lit_primitive_depth_orders_overlap`,
`test_app_offscreen_mesh_renders_nonblank`, `test_app_offscreen_rotated_mesh_depth_orders_faces`,
`test_app_offscreen_camera_arcball_mesh_renders_cube`, and the `test_arcball_*` group.

Current gap: live capture/readback is not part of this GLFW example; offscreen `hello_mesh.c`
covers capture.


### App trace/status

Command:

```bash
DVZ_DRP2_TRACE=normal ./build/examples/c/hello_mesh_glfw 120
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
./build/examples/c/hello_point
```

Expected behavior: saves `hello_point.png`; image is nonblank and contains the three colored
points.

Automated coverage: `test_app_offscreen_has_nonblank_pixels`.

Current gap: manual PNG inspection only; no example-level verifier.


### Image texture capture

Command:

```bash
./build/examples/c/hello_texture
```

Expected behavior: saves `hello_texture.png`; procedural image is visible on a quad.

Automated coverage: `test_app_offscreen_image_has_nonblank_pixels`,
`test_app_offscreen_image_retained_render_second_frame`.

Current gap: no interactive image probe example yet.


### Sampled field capture

Command:

```bash
./build/examples/c/hello_field
```

Expected behavior: saves `hello_field.png`; scalar field appears with the bound colormap.

Automated coverage: `test_scene_image_visual_binds_colormap_scale`,
`test_scene_image_scalar_texture_uses_bound_scale`,
`test_app_offscreen_image_field_partial_update_changes_region`.

Current gap: no manual partial-field-update example yet.


### 3D mesh capture

Command:

```bash
./build/examples/c/hello_mesh
```

Expected behavior: saves `hello_mesh.png`; cube is visible with perspective, arcball transform,
shading, and depth.

Automated coverage: `test_app_offscreen_mesh_renders_nonblank`,
`test_app_offscreen_rotated_mesh_depth_orders_faces`,
`test_app_offscreen_camera_arcball_mesh_renders_cube`.

Current gap: separate from live GLFW arcball smoke.


### Multi-panel render

Command:

```bash
just test test_app_offscreen_two_panel_points_light_both_halves
```

Expected behavior: left and right panel contents render into their own halves with
viewport/scissor isolation.

Automated coverage: `test_app_offscreen_two_panel_points_light_both_halves`,
`test_scene_multiple_panels_multiple_point_visuals_emit`,
`test_scene_multi_panel_glsl_emits_viewport_scissor_commands`.

Current gap: no manual multi-panel GLFW example yet.


### Partial texture update

Command:

```bash
just test test_app_offscreen_image_field_partial_update_changes_region
```

Expected behavior: a field subregion update changes only the expected rendered region on the next
frame.

Automated coverage: `test_app_offscreen_image_field_partial_update_changes_region`,
`test_scene_image_field_partial_update_emits_texture_subregion`,
`test_scene_shared_field_mixed_full_and_partial_uploads`.

Current gap: no manual partial-texture-update example yet.


### Image probe request

Command:

```bash
just test test_scene_image_probe_respects_panel_request_position
```

Expected behavior: probe results follow panel-local request coordinates and reject transparent or
failed readback cases.

Automated coverage: `test_scene_image_probe_respects_panel_request_position`,
`test_scene_image_probe_transparent_pixel_misses`,
`test_scene_image_probe_gpu_readback_failure_misses`,
`test_scene_process_requests_coalesces_pending_probes_before_execution`.

Current gap: no manual image-probe GLFW example yet.


## Recommended Manual Pass

For a normal scene/app smoke pass, run:

```bash
just build
just test test_app_offscreen_lit_primitive_depth_orders_overlap
just test test_scene_process_pick_probe_requests
./build/examples/c/hello_point_glfw
./build/examples/c/hello_pick_hover_glfw
./build/examples/c/hello_mesh_glfw
```

Record OS, GPU, backend, command, observed behavior, and whether Vulkan validation layers were
enabled for any anomaly. Convert deterministic manual failures into focused tests before adding new
visual families or broadening the public scene API.
