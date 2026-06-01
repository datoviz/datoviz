# Scene Example Planning

> **Status:** active planning
> **Updated on:** 2026-05-30
> **Scope:** worked example specs, release staging, gallery priorities, and current support gaps
> **Purpose:** keep one source of truth for which examples matter, when they matter, and what still
> blocks them.

This file replaces the previous split between north-star, release-staging, prioritization, and gap
reports. Use it as the planning entry point. Keep detailed scenario shape in `scenarios/`, compact
fixture coverage in [FIXTURES.md](FIXTURES.md), cross-repository placement in
[ORGANIZATION.md](ORGANIZATION.md), scenario ID lookup in [CATALOG.md](CATALOG.md), and shared
cache/API policy in [POLICIES.md](POLICIES.md).


## Gallery Thesis

The v0.4 gallery should make one argument quickly:

**Datoviz is a fast, modern, scientific GPU visualization engine for dense data, interactive
scenes, native applications, and portable rendering backends.**

Examples should feel like credible scientific scenes first, with code and reference links attached
afterward. Minimal examples remain executable truth; showcases are allowed to be composed and
editorial as long as their release status is explicit.


## Staging Vocabulary

| Stage | Meaning |
| --- | --- |
| `v0.4 required` | Must exist as a polished C example, release fixture, or release narrative item. |
| `v0.4 experimental` | In v0.4 scope, but explicitly partial, stretch, or backend-limited. |
| `v0.4 fixture-only` | Deterministic validation example; not a public gallery promise. |
| `v0.5` | Important next release target after v0.4 semantics are stable. |
| `later` | Strategic/domain showcase or broad pressure test beyond v0.5. |
| `external/GSP` | Belongs primarily in GSP/VisPy2/Matplotlib, with Datoviz keeping only low-level fixtures. |

Each example may also carry a readiness label:

| Readiness | Meaning |
| --- | --- |
| `ready-now` | Can be implemented or polished with the current active stack. |
| `needs-rc1-proof` | First slice exists, but needs a runnable example, fixture, or screenshot smoke. |
| `partial-now` | A reduced version can run today, but the full staged promise needs more work. |
| `blocked-by-v0.4-critical-path` | Required for v0.4 but waiting on a feature-freeze item. |
| `blocked-by-v0.5-feature` | Should not be forced into v0.4. |


## Current Capability Snapshot

The active scene stack already covers retained figures and panels, per-panel viewport/scissor,
offscreen and GLFW presentation, app callbacks, capture, retained resources, sampled fields,
point/pixel/marker/primitive/mesh/path/image/volume/sphere/text visuals, panzoom/arcball/fly/
turntable controllers, EDL/SSAO/MSAA/WBOIT/depth-peeling-shaped passes, colorbars, legends,
annotations, scale bars, and first GPU-backed point/marker/image/labels/volume request paths.

The main remaining polish or feature gaps are:

1. release proof for text, axes, colorbars, legends, annotations, and scale bars;
2. fixture/gallery proof for retained textured mesh, now that UVs, mesh-bound textures, texture
   shader variant, and material integration are implemented;
3. vector visuals so wind, flow, and track examples stop relying on primitive triangles;
4. richer picking/probe payloads for marker exact hit tests, mesh regions, paths, labels, text, and
   volume ray hits;
5. large-data policies for ring buffers, visible ranges, LOD, sparse updates, and long live loops;
6. scene-level custom material/compute resources for Mandelbrot, Gray-Scott, particles, and custom
   postprocess work;
7. deterministic gallery capture conventions, asset/cache helpers, and raw binding smoke coverage.


## v0.4 Required Set

| Scenario | Readiness | Required slice | Decision |
| --- | --- | --- | --- |
| `point_2d` | `ready-now` | retained scene, point visual, panzoom/offscreen | Keep as the smallest retained-scene smoke. |
| `path_axes_2d` | `needs-rc1-proof` | path, rendered text, 2D axes/ticks | First axes/text regression target. |
| `linked_panels_axes_panzoom` | `needs-rc1-proof` | grid/layout, linked panzoom, axes | Proves linked panels are real, not only layout. |
| `linked_panels_probe_colorbar` | `needs-rc1-proof` | image probe, colorbar, annotation/readout, linked state | Main 2D explanatory-object pressure test. |
| `marker_picking` | `needs-rc1-proof` | marker visual, item pick, selection highlight | Required if marker remains public v0.4 visual family. |
| `sphere_impostor` | `ready-now` | sphere visual, lighting/depth | Small 3D visual-polish proof. |
| `volume` | `needs-rc1-proof` | 3D sampled field, slice/render, capture | Keeps volume rendering and export covered. |
| `scale_bar` | `needs-rc1-proof` | retained scale bars, labels, panzoom/domain updates | Narrow RC1 fixture; richer layout can follow. |
| `image_probe` | `ready-now` | scalar image probe, custom LUT colormap, colorbar, live readout | Focused public image-query proof lives in `examples/c/features/image_probe.c`; broader linked-panel pressure remains separate. |
| `protein_arcball_viewer` | `partial-now` | mesh, sphere, materials, SSAO/MSAA, GUI, arcball | Flagship native C showcase; defer labels/picking/molecular surface if needed. |
| `showcase_wind_field` | `needs-rc1-proof` | image field, vector visual, paths, panzoom, colorbar | Best near-term 2D showcase; vector visual should replace primitive arrows. |
| `showcase_gpu_particle_smoke` | `ready-now` | scene compute, shared storage/vertex buffers, blended points | Public experimental compute-to-graphics showcase lives in `examples/c/showcases/gpu_particle_smoke.c`. |
| `textured_terrain_or_planet` | `ready-now` | retained textured mesh, UVs, texture sampling, lighting, capture | Required textured-mesh proof lives in `examples/c/showcases/textured_planet.c`: Earth/Mars UV sphere with real sampled textures and procedural fallbacks. Mars DEM terrain analysis remains v0.5/later. |
| `brain_volume_mesh` | `partial-now` | volume, transparent mesh, GUI, arcball | Narrow Allen/IBL brain slice for v0.4; full atlas explorer is v0.5. |
| `dense_point_cloud_edl` | `ready-now` | large points/pixels, EDL, fly/camera | Use LiDAR or synthetic dense cloud as performance/showcase proof. |


## v0.4 Experimental Set

| Scenario | Required slice | Decision |
| --- | --- | --- |
| `webgpu_browser_subset` | DRP2 WebGPU subset, WGSL, browser runtime | Point, primitive, image, and basic mesh only; visible experimental status. |
| `animation_video_export` | frame callbacks, deterministic capture/video | Keep simple; defer transition/camera-path helper polish. |
| `splat_cloud` | retained splat visual, blend/depth policy, deterministic capture | Include only if the visual lands cleanly; not a release blocker. |
| `cpu_fluid_or_particles` | dynamic image/point/path updates, panzoom, colorbar | CPU-side stretch only; GPU compute remains later. |
| `dense_streaming_2d` | path/pixel updates, axes/text, linked x panzoom | Pick one DAQ/physiology example if time allows. |
| `spatial_omics` | large point/pixel, basic selection, color mapping | Useful stress/example lane; napari integration remains external. |
| `mouse_brain_slice` | image/volume slice, colorbar, GUI controls | Narrow slice can be v0.4 experimental; full atlas exploration is v0.5. |


## v0.5 Set

| Scenario | Missing feature driver | Decision |
| --- | --- | --- |
| `mesh_selection_link` | mesh face/region picking, linked highlight styling | API sketch remains useful after marker/image selection lands. |
| `mouse_brain_atlas_explorer` | region picking, selection-driven mesh styling, linked 2D/3D panels | Full explorer is beyond v0.4. |
| `streaming_signal_workbench` | dense traces, ring buffers, many-trace layout, overlays | Merge physiology and DAQ into one workbench track. |
| `toy_dicom_viewer` | shared 3D texture slices, crosshairs, window/level, slice dragging | Good v0.5 medical-viewer target. |
| `market_microstructure` | bars/candles, LOD, tooltips, crosshair, streaming policy | Requires dashboard and large-data policies. |
| `embedding_explorers` | image thumbnail LOD, label LOD, search/query sidecar, overlay cards | Keep PD12M/Wikipedia as paired dashboard showcases. |
| `napari_labels_orthoslices_clipping` | label GPU probing, shared 3D texture slices, richer volume clipping | Datoviz proves primitives; full napari workflow remains external. |
| `tracks_tractography_vectors` | vector visual, packed ragged paths, per-track identity, selection | Combine napari tracks and diffusion tractography into one v0.5 lane. |
| `galaxy_labels` | large star cache, label overlay, marker/sprite polish | Keep C version engine-focused. |
| `wind_projections` | projection transforms, vector visual, graticule/coastline helpers | v0.4 wind field is the subset. |
| `textured_surface_full_workflow` | terrain/planet/Mars asset cache, camera-path polish, optional overlays | Follows the v0.4 `textured_planet.c` retained textured-mesh proof; real Mars DEM terrain remains outside the v0.4 slice. |
| `finite_element_stress` | mesh scalar fields, selection, colorbar, isolines | Applied mesh/field target. |
| `crystal_phonon` | sphere/mesh animation, selection, labels | Materials-science showcase after sphere/text/animation polish. |


## Later Set

| Scenario | Reason |
| --- | --- |
| `compute_and_custom_shaders` | Mandelbrot, Gray-Scott, and GPU particles need scene-level compute/custom material resources. |
| `tokamak_hep_field_lines` | Needs field-line paths/tubes, complex event geometry, vector fields, labels, picking, and domain data policy. |
| `geo_trajectories_regions_events` | Migration, choropleth, earthquakes, and flight tracks need geographic transforms, topology helpers, timelines, and LOD. |
| `many_labels_and_splats_full` | Needs label LOD/collision or full Gaussian-splat asset pipelines beyond first slices. |
| `advanced_runtime_export_diagnostics` | Multi-window/fullscreen/HiDPI, high-resolution/transparent export, batch/server capture, remote/cloud, and visual diagnostics need dedicated design. |
| `large_data_strategy_gallery` | Density rendering, progressive refinement, tile streaming, GPU instancing, and out-of-core policies need explicit resource semantics. |


## Feature Priority

1. Text, axes, annotations, legends, colorbars, and scale bars.
2. Retained textured-mesh proof capture.
3. Vector visual.
4. Large point/pixel/path partial-update policy.
5. Selection and picking beyond points/images.
6. Scene-level custom material and compute resources.
7. Textured sphere/cubemap asset lane.


## Near-Term Gallery And Example Pressure

Recommended release-proof order:

1. gallery proof pass for protein, LiDAR, brain, labels, textured mesh or terrain/planet, colorbar
   and legend paths, and capture validation;
2. vector visual polish with a wind-field showcase;
3. label probe hardening under transforms, larger fields, latest-request-wins hover, signed ids,
   high unsigned ids, and background misses;
4. explanatory layout proof combining axes, continuous colorbar, categorical legend, scale bar, and
   panel reserves;
5. splat showcase only if release-proof lanes stay stable.

Add a `bezier_curve_path` example when geometry helpers are ready: quadratic and cubic Bezier
controls tessellated on CPU to ordinary `dvz_path()` data, control polygons through `dvz_segment()`,
control points through `dvz_marker()` or `dvz_point()`, multiple curves through subpaths, and
optional controls for tessellation quality, stroke width, join mode, and overlay visibility.


## Pickup Order

1. `point_2d`
2. `path_axes_2d`
3. `linked_panels_axes_panzoom`
4. `scale_bar`
5. `image_probe`
6. `linked_panels_probe_colorbar`
7. `marker_picking`
8. `volume`
9. `protein_arcball_viewer`
10. `showcase_wind_field`
11. `showcase_gpu_particle_smoke`
12. `textured_terrain_or_planet`
13. `brain_volume_mesh`
14. `dense_point_cloud_edl`
15. `webgpu_browser_subset`


## Promotion Rule

Worked scenario files are informative pressure tests. A scenario becomes a release commitment only
when it appears in the staged tables above or in [FIXTURES.md](FIXTURES.md). When an individual
scenario exposes a reusable rule, move that rule to [POLICIES.md](POLICIES.md), [STYLE.md](STYLE.md),
[TECHNIQUES.md](TECHNIQUES.md), or the canonical scene spec instead of copying it into more
scenario files.
