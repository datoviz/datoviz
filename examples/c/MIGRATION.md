# C Example Migration Table

This table classifies existing v0.4-dev examples for the final example overhaul. It is a working
index, not a completed move list. Follow `../../spec/scene/examples/EXECUTION.md` before moving or
removing any source file.


## Required Release Proofs

| Scenario ID | Current source | Target source | Action |
| --- | --- | --- | --- |
| `point_2d` | `visuals/point.c` | `visuals/point.c` | Polished in place as the reference baseline. |
| `path_axes_2d` | `features/axes_2d.c` | `features/axes_2d.c` | Added as a deterministic path plus axes baseline; keep old scatter axes as a lab for now. |
| `linked_panels_axes_panzoom` | `features/panel_linked_axes.c` | `features/panel_linked_axes.c` | Added as a deterministic two-panel shared-X panzoom plus axes baseline; keep old linked panels as a lab for now. |
| `scale_bar` | `features/scalebar.c` | `features/scalebar.c` | Added as the polished public scale-bar proof; old demo remains in `lab/` and minimal variant remains in `regression/`. |
| `image_probe` | `techniques/image_probe.c` | `features/image_probe.c` | Added as the polished public image-probe/colorbar/readout proof; old live-query smoke remains under `techniques/`. |
| `linked_panels_probe_colorbar` | `techniques/image_probe.c` | `features/panel_probe_colorbar.c` | Compose image probe, colorbar, readout, and linked state. |
| `marker_picking` | `techniques/pick_hover.c` | `features/pick_marker.c` | Promote once marker-specific pick/selection state is proven. |
| `volume` | `visuals/volume.c` | `visuals/volume.c` | Polish in place as the visual baseline. |


## Required Showcases

| Scenario ID | Current source | Target source | Action |
| --- | --- | --- | --- |
| `protein_arcball_viewer` | `showcase/protein.c` | `showcases/protein_arcball_viewer.c` | Promote after style, capture, and bounded smoke pass. |
| `showcase_wind_field` | none | `showcases/wind_field.c` | Add new 2D field showcase. Use primitive arrows until vector visual is final. |
| `textured_terrain_or_planet` | `visuals/textured_mesh.c` | `showcases/textured_terrain_or_planet.c` | Add after retained textured mesh is release-ready; visual baseline remains separate. |
| `brain_volume_mesh` | `showcase/brain.c`, `showcase/ibl_brain.c` | `showcases/brain_volume_mesh.c` | Pick the narrower release-proof path; keep the other as lab/regression if useful. |
| `dense_point_cloud_edl` | `showcase/lidar.c` | `showcases/dense_point_cloud_edl.c` | Promote as performance/showcase proof. |


## Existing Visual Baselines

| Current source | Target role | Action |
| --- | --- | --- |
| `visuals/pixel.c` | visual baseline | Polish in place. |
| `visuals/marker.c` | visual baseline | Polish in place; keep picking in feature lane. |
| `visuals/primitive.c` | visual baseline | Polish in place. |
| `visuals/segment.c` | visual baseline | Polish in place. |
| `visuals/path.c` | visual baseline | Polish in place; axes belong in feature lane. |
| `visuals/mesh.c` | visual baseline | Polish in place. |
| `visuals/image.c` | visual baseline | Polish in place; probing/colorbar belong in feature lane. |
| `visuals/sphere.c` | visual baseline | Polish in place and map to `sphere_impostor` if needed. |
| `visuals/text.c` | visual baseline or feature proof | Decide after text API documentation settles. |
| `visuals/colorbar.c` | feature proof | Move to `features/colorbar.c` when promoted. |
| `visuals/polygon.c` | visual baseline or deferred | Keep only if polygon remains public v0.4 release surface. |
| `visuals/vector.c` | visual baseline or experimental | Keep explicit status until vector visual is release-ready. |
| `visuals/instanced_cubes.c` | stress or technique | De-index from public visual baseline unless retained as stress. |
| `visuals/surface_grid.c` | visual/showcase input | Reclassify when textured mesh and surface story is final. |
| `visuals/surface_grid_overlays.c` | feature/showcase input | Reclassify after overlay/layout proof is selected. |


## Existing Technique And Feature Candidates

| Current source | Target role | Action |
| --- | --- | --- |
| `techniques/depth_cue.c` | technique | Keep under techniques. |
| `techniques/depth_peel.c` | technique | Keep under techniques. |
| `techniques/edl.c` | technique | Keep under techniques; showcase EDL belongs in `dense_point_cloud_edl`. |
| `techniques/wboit.c` | technique | Keep under techniques. |
| `techniques/overlay_card.c` | feature | Move to `features/overlay_card.c` if public. |
| `techniques/overlay_rich_card.c` | feature or experimental | Move only if public rich-card example is kept. |
| `techniques/rich_text_block.c` | feature | Move to `features/text_block.c` when text-block API is final. |
| `techniques/grid_layout.c` | feature | Move to `features/panel_grid.c`. |
| `techniques/multi_panel.c` | feature | Move to `features/panel_multi.c`. |
| `techniques/arcball_gizmo.c` | feature or experimental | Move only if gizmo is public; otherwise keep diagnostic. |
| `techniques/bounds_overlay.c` | regression or diagnostic | Keep non-gallery unless it becomes a public feature proof. |
| `techniques/scheduler_lab.c` | lab/diagnostic | Keep non-gallery. |
| `techniques/gui_viewport.c` | runtime/integration | Move to runtime or keep as integration example. |
| `techniques/gui_multi_viewport.c` | runtime/integration | Move to runtime or keep as integration example. |


## Low-Level And Tooling

| Current source | Target role | Action |
| --- | --- | --- |
| `tools/raw_triangle.c` | advanced | Move to `advanced/` or `drp2/` if documented. |
| `tools/raw_triangle_drp2.c` | advanced | Move to `advanced/` or `drp2/` if documented. |
| `tools/record_dvzr.c` | runtime/advanced | Keep as recording proof, not public visual gallery. |
| `tools/replay_dvzr.c` | runtime/advanced | Keep as replay proof, not public visual gallery. |
| `tools/export_*_wgsl.c` | advanced/WebGPU fixture tooling | Keep non-gallery unless linked from WebGPU docs. |
| `tools/frame_plan_graph_debug.c` | diagnostic | Keep non-gallery. |
| `tools/text_msdf_*` | diagnostic/lab | Keep non-gallery until text backend diagnostics are public. |
| `tools/hosted_glfw_smoke.c` | runtime | Move to `runtime/hosted_glfw_smoke.c` when indexed. |


## Stress Examples

| Current source | Target role | Action |
| --- | --- | --- |
| `stress/point_stress.c` | stress | Preserved from the former high-count `visuals/point.c` workbench. |
