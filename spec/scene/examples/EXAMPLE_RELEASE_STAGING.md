# Scene Example Release Staging

> **Status:** Active planning
> **Updated on:** 2026-05-27
> **Scope:** `spec/scene/examples`, current `examples/c`, and v0.4/v0.5/later feature staging
> **Purpose:** classify examples by release target and make feature dependencies explicit.

This file is the staging table for Datoviz examples. It answers:

1. which examples should be implemented or polished for v0.4,
2. which examples are useful v0.4 fixtures but not gallery/showcase commitments,
3. which examples should move to v0.5 or later because they need deferred features,
4. which examples belong primarily to external GSP/VisPy2 or Matplotlib-backed workflows.

Read this together with:

1. [`EXAMPLE_NORTH_STAR.md`](EXAMPLE_NORTH_STAR.md) for the aspirational gallery and showcase
   direction before filtering by current capability or release commitments.
2. [`FEATURE_FIXTURE_MATRIX.md`](FEATURE_FIXTURE_MATRIX.md) for one-feature fixture ideas,
3. [`EXAMPLE_ORGANIZATION.md`](EXAMPLE_ORGANIZATION.md) for repository/API-layer ownership,
4. [`EXAMPLE_PRIORITIZATION.md`](EXAMPLE_PRIORITIZATION.md)
   for visual-payoff ranking,
5. [`../../../agents/now/STATUS.md`](../../../agents/now/STATUS.md)
   for the active C implementation critical path.
6. [`../../api/PYTHON_GSP_SCOPE.md`](../../api/PYTHON_GSP_SCOPE.md)
   for the Datoviz, raw `ctypes`, GSP, and VisPy2 ownership split.


## Staging Vocabulary

| Stage | Meaning |
|---|---|
| `v0.4 required` | Must exist as a polished C example, release fixture, or release narrative item. |
| `v0.4 experimental` | In v0.4 scope, but explicitly partial or backend-limited. |
| `v0.4 fixture-only` | Small deterministic validation example; not a gallery/showcase promise. |
| `v0.5` | Important next release target after v0.4 core semantics are stable. |
| `later` | Strategic/domain showcase or broad feature pressure test beyond v0.5. |
| `external/GSP` | Belongs primarily in GSP/VisPy2/Matplotlib, though Datoviz may keep low-level fixtures. |

The release target is about the **example promise**, not whether individual low-level pieces already
exist. An example may use implemented features and still be staged for v0.5 if the full workflow
needs deferred semantics, data policies, or API cleanup.

Language ownership is part of the staging decision. Datoviz v0.4 examples are C-first because the
v0.4 release surface is the native renderer/runtime and C API. Raw Python examples in this
repository should stay close to generated `ctypes` smoke coverage. Pythonic plotting, notebook,
dashboard, backend-comparison, and high-level scientific workflow examples belong primarily in
GSP/VisPy2.

Each example also has a separate **current readiness**:

| Readiness | Meaning |
|---|---|
| `ready-now` | Can be implemented or polished with the current active stack. |
| `needs-rc1-proof` | The first implementation slice exists, but the release still needs a runnable example, fixture, screenshot smoke, or explicit known-issue note. |
| `blocked-by-v0.4-critical-path` | Belongs in v0.4, but waits for WebGPU/WASM, raw `ctypes`, parity audit, public status cleanup, or another remaining feature-freeze item. |
| `partial-now` | A smaller version can run today, but the staged release promise needs more work. |
| `blocked-by-v0.5-feature` | Should not be forced into v0.4 because it needs a deferred feature. |

This distinction avoids a common confusion: `PATH_AXES_2D` is a `v0.4 required` release target, but
it is no longer evidence that axes are planning-only. It is now `needs-rc1-proof`: the active
linear axes/tick/label path should be exercised by a release fixture or example before RC1, while
label-layout polish can move to RC2 or v0.5.


## Feature Milestone Map

| Feature group | v0.4 target | v0.5 target | Later / external |
|---|---|---|---|
| Scene/app basics | Retained scene, figures, panels, offscreen/GLFW app, capture, frame callbacks. | Broader hosted/integration polish. | Application frameworks owned above Datoviz. |
| Core visuals | Point, pixel, marker, primitive, segment/path, image, mesh, retained textured mesh, sphere, volume, basic text visual hardening. | Vector/arrow visual, richer path grouping, textured sphere, stronger volume slices. | Tubes/ribbons, skybox/cubemap, advanced domain visuals. |
| Text/explanatory objects | Release-quality basic rendered text, 2D axes/ticks, continuous colorbars, categorical legends, labels/readouts. | Shared legends/colorbars, richer annotation/callout and label layout. | Full math/TeX, complex shaping, publication typography in GSP/Matplotlib. |
| Interaction | Point pick, image probe, marker pick, basic selection highlight, linked crosshair/probe. | Mesh/region, path, label, volume pick/probe; lasso/box selection. | Full dashboard interaction models in GSP/VisPy2. |
| Layout/linking | Grid/subplot layout, linked panzoom x/y, panel reservations. | Dashboard layout helpers, shared legends/colorbars. | Full application/dashboard framework in GSP/VisPy2. |
| Large data | Stable high-count point/pixel/path updates and basic partial updates. | LOD, ring buffers, visible-range policies, tiled images. | Out-of-core and distributed data policies. |
| Tracks/trajectories | Use existing path/segment/marker fallbacks only. | Polished tractography/track viewers over packed ragged paths, per-track identity, direction coloring, tail/time filtering, basic picking/selection, and high-quality thin-line rendering. | Future `tube` rendering, tube/ribbon geometry, million-streamline out-of-core collections, and domain-specific track resources. |
| Multi-pass techniques | EDL, SSAO, MSAA, WBOIT/depth-peeling slices, volume occlusion as native examples. | General scene framegraph resources and custom materials. | Advanced postprocess/effects library. |
| Compute/custom shaders | DRP2-level fixtures only. | Scene-level compute/custom visual/material API. | Large simulation/application frameworks. |
| WebGPU/WASM | Experimental point/primitive/image/basic mesh subset. | Broader scene subset and automated browser parity. | Full backend parity only if justified by demand. |
| Vector export | Out of Datoviz scope. | Out of Datoviz scope. | GSP/Matplotlib publication backend. |
| OO Python plotting | Out of Datoviz scope. | External GSP/VisPy2. | External GSP/VisPy2. |


## Repository And Language Ownership

Use the following split when deciding where an example should live:

| Example kind | Primary repository | Language |
|---|---|---|
| Core renderer and visual fixtures | Datoviz | C |
| Native app, controller, picking/probing, capture, and runtime examples | Datoviz | C |
| Renderer/performance showcase examples | Datoviz | C |
| DRP2, DVZR, WebGPU stream, and hosted-runtime validation | Datoviz | C or fixtures |
| Raw binding smoke examples | Datoviz | minimal Python `ctypes` |
| Pythonic plotting and scientific workflows | GSP/VisPy2 | Python |
| Backend comparison examples, including Datoviz and Matplotlib renderers | GSP/VisPy2 | Python |
| Publication/static Matplotlib fallback examples | GSP/VisPy2 | Python |

Do not move Datoviz C showcase examples to Python merely because GSP/VisPy2 will become the
Pythonic user surface. Datoviz keeps native C examples as release proof. Later GSP/VisPy2 examples
may mirror some showcases through a higher-level Python API, but those examples should be
counterparts rather than replacements.


## v0.4 Required Examples

These examples should be part of the v0.4 release narrative or required C validation.

| Example | Stage | Required feature slice | Decision |
|---|---|---|---|
| `core/POINT_2D.md` | `v0.4 required` / `ready-now` | retained scene, point visual, panzoom/offscreen | Keep as the smallest retained-scene smoke. |
| `core/PATH_AXES_2D.md` | `v0.4 required` / `needs-rc1-proof` | path, rendered text, 2D axes/ticks | Use as the first axes/text regression target. |
| `core/LINKED_PANELS_AXES_PANZOOM.md` | `v0.4 required` / `needs-rc1-proof` | grid/layout, linked panzoom, axes | Required to prove linked panels are real, not only layout. |
| `core/LINKED_PANELS_PROBE_COLORBAR.md` | `v0.4 required` / `needs-rc1-proof` | image probe, colorbar, annotation/readout, linked state | Main 2D explanatory-object pressure test. |
| `core/MARKER_PICKING.md` | `v0.4 required` / `needs-rc1-proof` | marker visual, bounding-box marker pick, selection highlight | Required if marker remains a supported v0.4 visual family; exact SDF picking can follow. |
| `core/SPHERE_IMPOSTOR.md` | `v0.4 required` / `ready-now` | sphere visual, lighting/depth basics | Small proof for sphere impostors and 3D visual polish. |
| `core/VOLUME_SLICE.md` | `v0.4 required` / `needs-rc1-proof` | 3D sampled field, volume slice, transfer/value range | Required to avoid volume regression from v0.3. |
| `core/VOLUME_OFFSCREEN.md` | `v0.4 required` / `needs-rc1-proof` | volume rendering plus deterministic capture | Required as the volume capture smoke. |
| `core/SCALEBAR_2D_3D.md` | `v0.4 required` / `needs-rc1-proof` | retained scale bars, labels, panzoom/domain updates | Keep a narrow fixture in RC1; leave update-churn optimization for RC2 unless it blocks examples. |
| `api/API_IMAGE_PROBE_PINNED_READOUT.md` | `v0.4 required` / `needs-rc1-proof` | image probe, pinned readout text | Keep as the API-level readout contract test. |
| `api/API_SCALE_COLORBAR_ANNOTATION.md` | `v0.4 required` / `needs-rc1-proof` | scales, colorbars, labels, annotations | Keep as the explanatory-object API contract test. |
| `api/API_SAMPLED_FIELD.md` | `v0.4 required` / `needs-rc1-proof` | sampled fields, image, labels, volume | Keep as the resource-sharing contract test. |
| `bio/PROTEIN_ARCBALL_VIEWER.md` | `v0.4 required` / `partial-now` | mesh, sphere, materials, SSAO/MSAA, GUI, arcball | Flagship native C showcase; defer labels/picking/molecular surface if needed. |
| `geo/SHOWCASE_WIND_FIELD.md` | `v0.4 required` / `needs-rc1-proof` | image field, arrows via primitives, paths, panzoom, colorbar | Best 2D showcase; vector visual can be v0.5 replacement. |
| Textured terrain / planet C showcase | `v0.4 required` / `blocked-by-v0.4-critical-path` | retained textured mesh, UVs, sampled-field texture binding, mesh texture shader, camera path, capture | Required v0.4 visual proof. Use Grand Canyon, Mars, or a deterministic terrain/planet fixture; do not substitute baked vertex colors. |
| `neuro/ALLEN_IBL_COORDINATE_MESH_VOLUME_PLAN.md` | `v0.4 required` / `partial-now` | volume, mesh transparency, GUI, arcball | Prefer a narrow volume + transparent atlas mesh slice. |
| LiDAR/dense point cloud C showcase | `v0.4 required` / `ready-now` | point/pixel, EDL, fly/camera, large buffer smoke | Current `examples/c/showcase/lidar.c` can be the concrete target. |


## v0.4 Experimental Examples

These examples should be present only with visible experimental status and a documented subset.

| Example | Stage | Required feature slice | Decision |
|---|---|---|---|
| WebGPU fixture dashboard / browser runner | `v0.4 experimental` | DRP2 WebGPU subset, WGSL, browser runtime | Keep examples under `examples/webgpu`; document unsupported commands/features. |
| WASM scene-to-WebGPU minimal page | `v0.4 experimental` | WASM scene subset, WGSL DRP2 stream transport | Target point, primitive, image, and maybe basic mesh only. |
| `core/ANIMATION_VIDEO_EXPORT.md` | `v0.4 experimental` | frame callbacks, deterministic capture/video | Keep simple animation/video path; defer transition/camera-path helper polish. |
| Splat / Gaussian-like point cloud showcase | `v0.4 experimental` | first-class retained splat visual, dense translucent point cloud, blend/depth policy, camera, capture | Include if the `splat` visual lands cleanly before v0.4; do not block release on it. |
| `physics/CFD_VORTICITY_ADVECTION.md` narrow CPU slice | `v0.4 experimental` | dynamic image field, dynamic point/path updates, panzoom, colorbar, probe | CPU-side fluid/particle advection may be a stretch showcase; GPU compute remains later. |
| `dashboards/STREAMING_DAQ_VIEWER.md` or `bio/PHYSIOLOGY_SIGNAL_WORKBENCH.md` | `v0.4 experimental` | path/pixel updates, axes/text, linked x panzoom | Pick one simple synthetic sustained-update example only if time allows; full workflow is v0.5. |
| `napari/DENSE_POINTS_SPATIAL_OMICS.md` | `v0.4 experimental` | large point/pixel, basic selection, color mapping | Good stress/example lane if data prep is ready; keep napari integration external. |
| `neuro/ALLEN_MOUSE_BRAIN_SLICE_EXAMPLE_PLAN.md` | `v0.4 experimental` | image/volume slice, colorbar, GUI controls | Useful if it stays narrow; full atlas exploration is v0.5. |


## v0.4 Fixture-Only Examples

These are important for validation but should not be marketed as full gallery workflows.

| Area | Stage | Examples / rows | Decision |
|---|---|---|---|
| One-visual fixtures | `v0.4 fixture-only` | point, pixel, marker, primitive, segment, path, image, mesh, sphere, volume, text | Keep tiny deterministic C examples for every supported visual. |
| Scene/app basics | `v0.4 fixture-only` | empty scene, resize, panel layout, background, offscreen, capture, GLFW, frame callback | Required as regression coverage. |
| Data/update fixtures | `v0.4 fixture-only` | initial attributes, partial range update, mutability hints, region update | Required to guard retained resource behavior. |
| Controllers | `v0.4 fixture-only` | panzoom, arcball, fly, turntable | Keep bounded/offscreen where possible. |
| Interaction | `v0.4 fixture-only` | point pick, image probe, request coalescing, selection bookkeeping, link channel | Promote marker picking to required if supported publicly. |
| Techniques | `v0.4 fixture-only` | depth test, alpha blend, WBOIT, depth peel, EDL, SSAO, MSAA, volume occlusion | Keep small, deterministic, and separate from showcases. |
| GUI | `v0.4 fixture-only` | overlay, callback, curated controls, viewport, multi-viewport, cimgui raw | Native GUI examples are engine/integration fixtures, not scene core. |
| Video | `v0.4 fixture-only` | config, offline encode, stream sink, offscreen scene capture | Keep backend-gated. |
| DRP2/DVZR/WebGPU streams | `v0.4 fixture-only` | emitted JSON, recording/replay, WGSL export tools | Required for protocol/runtime confidence. |
| Low-level `vk`/`vklite`/`canvas`/`stream` | `v0.4 fixture-only` | advanced examples in `FEATURE_FIXTURE_MATRIX.md` | Keep as power-user/backend fixtures. |


## v0.5 Examples

These should become concrete after v0.4 text/axes/colorbars/layout/selection and visual polish are
stable.

| Example | Stage | Missing feature driver | Decision |
|---|---|---|---|
| `api/API_MESH_SELECTION_LINK.md` | `v0.5` | mesh face/region picking, linked highlight styling | Useful after basic marker/image selection lands. |
| `core/MOUSE_BRAIN_ATLAS_EXPLORER.md` | `v0.5` | region picking, selection-driven mesh styling, linked 2D/3D panels | Full explorer is beyond v0.4; narrow brain showcase remains v0.4. |
| `bio/PHYSIOLOGY_SIGNAL_WORKBENCH.md` | `v0.5` | dense traces, axes/text, streaming update policy, linked x panzoom | Can provide the full version after a simpler v0.4 sustained-update demo. |
| `dashboards/STREAMING_DAQ_VIEWER.md` | `v0.5` | ring buffers, many-trace layout, overlays, pause/reset controls | Can provide the full version after a simpler v0.4 sustained-update demo. |
| `dashboards/TOY_DICOM_VIEWER.md` | `v0.5` | shared 3D texture slices, crosshairs, window/level, slice dragging | Good v0.5 medical-viewer target. |
| `dashboards/MARKET_MICROSTRUCTURE.md` | `v0.5` | bars/candles, LOD, tooltips, crosshair, streaming policy | Requires dashboard and large-data policies. |
| `dashboards/IMAGE_EMBEDDING_LOD.md` | `v0.5` | image sprite/thumbnail LOD, selection cards, cache policy | Keep PD12M preprocessing external; Datoviz proves rendering and interaction. |
| `dashboards/SEMANTIC_EMBEDDING_ATLAS.md` | `v0.5` | label LOD, search/query sidecar, selected-card overlays | Belongs with rendered text and dashboard policies. |
| `astronomy/GALAXY.md` | `v0.5` | marker/point sprite quality, label overlay, large dataset cache | Keep C version engine-focused; polished Python UX is external/GSP. |
| `napari/LARGE_LABELS_SEGMENTATION.md` | `v0.5` | label colormap path, selection, direct GPU categorical sampling | CPU RGBA fallback is acceptable before full label semantics. |
| `napari/MULTIVIEW_LINKED_ORTHOSLICES.md` | `v0.5` | shared 3D texture slices, linked crosshairs, volume/slice probes | Similar dependency to Toy DICOM. |
| `napari/VOLUME_CLIPPING_3D.md` | `v0.5` | richer volume clipping/probing and UI controls | Builds on v0.4 volume. |
| `napari/TRACKS_VECTORS_SHAPES_CELL_MOTION.md` | `v0.5` | vector visual, tracks/path identity, selection | Good vector/linked interaction target. |
| `neuro/DIFFUSION_TRACTOGRAPHY.md` | `v0.5` | packed ragged 3D streamlines, per-streamline identity, direction color, arcball, selection | Make this a good v0.5 showcase, not just a minimal line demo. Use high-quality thin streamlines first; keep tubes/ribbons/out-of-core for later. |
| 3D network / graph explorer | `v0.5` | graph layout/data helpers, edge/path identity, labels on demand, linked selection | Useful mixed-primitive and interaction showcase after path picking, labels, and selection styling mature. |
| `geo/GLOBAL_WIND_PROJECTIONS.md` | `v0.5` | projection transforms, vector visual, graticule/coastline helpers | `SHOWCASE_WIND_FIELD` is the v0.4 subset. |
| Full `geo/GRAND_CANYON_FLYOVER.md` | `v0.5` | terrain asset cache, camera-path polish, optional overlays | A narrow textured terrain proof is v0.4 required; full data/product workflow can follow. |
| Full `geo/EARTH.md` | `v0.5` | cubemap/skybox, equirectangular sphere texture if not covered by mesh, asset polish | Keep nonessential skybox and full globe workflow for later if needed. |
| Full `geo/MARS_TEXTURED_MESH_EXAMPLE_PLAN.md` | `v0.5` | GIS preprocessing, layer controls, probe/readout, asset cache | The retained textured-mesh slice is v0.4 required; full Mars terrain assessment can follow. |
| `engineering/FINITE_ELEMENT_STRESS_VIEWER.md` | `v0.5` | mesh scalar fields, selection, colorbar, maybe isolines | Good applied mesh/field target. |
| `materials/CRYSTAL_PHONON_EXPLORER.md` | `v0.5` | sphere/mesh, animation, selection, labels | Depends on polished sphere/text/animation. |


## Later Examples

These are valuable pressure tests, but they should not shape the v0.4 release gate.

| Example | Stage | Reason |
|---|---|---|
| `compute/GRAY_SCOTT.md` | `later` unless compute is pulled into v0.5 | Needs scene-level compute, ping-pong resources, storage textures, custom fullscreen render. |
| `compute/MANDELBROT.md` | `later` unless custom shaders are pulled into v0.5 | Needs scene custom material/fullscreen visual API and high-precision parameter policy. |
| `compute/PARTICLES.md` | `later` unless compute is pulled into v0.5 | Needs compute-written buffers, trails/accumulation, additive particle shaders. |
| Full Gaussian-splat pipeline | `later` | Trained splat asset loading, differentiable rendering, out-of-core splat scenes, advanced splat LOD, and production Gaussian-splat formats remain beyond the first experimental visual. |
| Tractography tube/ribbon and out-of-core extensions | `later` | Adds tube/ribbon geometry, advanced LOD, million-streamline residency, and domain-specific tract resources beyond the v0.5 viewer. |
| Full `physics/CFD_VORTICITY_ADVECTION.md` | `later` | GPU compute/advection, vector visual polish, diagnostics, larger datasets. A CPU-side v0.4 stretch can use dynamic image/point/path updates. |
| `physics/TOKAMAK_PLASMA_FIELD_LINES.md` | `later` | Needs field-line paths/tubes, vector fields, picking, domain data policies. |
| `physics/HEP_EVENT_DISPLAY.md` | `later` | Needs complex event geometry, picking, labels, domain interaction. |
| `geo/ANIMAL_MIGRATION_TRACKS.md` | `later` | Needs geographic transforms, trajectories, timeline/dashboard controls. |
| `geo/CHOROPLETH_GLOBE_EXAMPLE_PLAN.md` | `later` | Needs geographic projection/topology helpers and region picking. |
| `geo/EARTHQUAKE_AFTERSHOCK_EXPLORER.md` | `later` | Needs geographic transforms, temporal controls, selection/dashboard UI. |
| `geo/FLIGHT_TRAJECTORIES_DEMO_PLAN.md` | `later` | Needs globe/projection, trajectory paths, large-data/LOD policies. |
| `astronomy/ASTRONOMY_MANY_LABELS.md` | `later` | Needs label LOD/collision policy beyond v0.4 basic text hardening. |
| `napari/GPU_AI_SEGMENTATION_INTEROP.md` | `later` | Needs external GPU/AI interop and richer label/selection workflow. |
| Remote/cloud/thin-client viewer | `later` | Needs transport, session, latency, security, and deployment policy above the native runtime. Keep WebGPU/browser as the v0.4 portability proof. |
| Native multi-window/fullscreen/HiDPI showcase variants | `later` | Useful runtime confidence lanes, but v0.4 should keep them fixture-level unless release examples need them. |
| High-resolution, transparent-background, and server-side/batch export workflows | `later` | PNG capture exists, but export scale/alpha/background conventions, batch APIs, and server deployment policies need dedicated design. |
| Advanced axes and coordinate-system gallery | `later` | Log/symlog/datetime axes, explicit screen/panel/data/world/NDC demos, and multiple coordinate systems need focused semantics beyond linear v0.4 axes. |
| Visual diagnostics gallery | `later` | Coordinate grids, picking-id views, draw-call grouping, and GPU buffer-update visualization are valuable engine-user diagnostics but not public v0.4 showcase promises. |
| Large-data rendering strategy gallery | `later` | Density rendering, progressive refinement, tile streaming, GPU instancing, and out-of-core policies need explicit resource and LOD semantics. |


## External/GSP Primary Examples

These may have Datoviz C fixtures underneath, but their user-facing examples should live primarily
in GSP/VisPy2 or Matplotlib-backed workflows.

| Example family | Stage | Reason |
|---|---|---|
| High-level scatter/line/image plotting | `external/GSP` | User-facing OO/plotting ergonomics belong outside Datoviz C. |
| Publication-quality static figures | `external/GSP` | Vector/PDF/SVG export is GSP/Matplotlib scope. |
| Notebook/dashboard workflows | `external/GSP` | Python-native object ownership, callbacks, widgets, and data loading belong in VisPy2/GSP. |
| Rich data loaders and preprocessing galleries | `external/GSP` | Datoviz can keep small prep scripts for C showcases, but not own full Python UX. |
| Full napari application examples | `external/GSP` | Datoviz should provide rendering primitives and fixtures; napari integration belongs above it. |


## Current `examples/c` Mapping

This section maps existing runnable C examples to release staging. It is intentionally coarser than
the worked-spec tables above; exact fixture names remain in
[`FEATURE_FIXTURE_MATRIX.md`](FEATURE_FIXTURE_MATRIX.md).

| Path / family | Stage | Decision |
|---|---|---|
| `examples/c/visuals/point.c`, `pixel.c`, `marker.c`, `primitive.c`, `segment.c`, `path.c`, `mesh.c`, `polygon.c`, `image.c`, `volume.c`, `sphere.c` | `v0.4 fixture-only` | Keep buildable as active visual-family and semantic-composite smoke examples. |
| `examples/c/visuals/text.c` | `v0.4 required` / `partial-now` | Existing basic rendered-text smoke; harden as the canonical v0.4 text example. |
| `examples/c/techniques/scatter_axes.c` | `v0.4 required` / `needs-rc1-proof` | Existing axes API/grid smoke; keep as the narrow linear axes/tick/label release proof. |
| `examples/c/techniques/linked_panels.c`, `multi_panel.c` | `v0.4 required` | Keep as layout and linked-panel smoke targets. |
| `examples/c/techniques/image_probe.c`, `pick_hover.c` | `v0.4 required` | Keep as request/readback interaction targets. |
| `examples/c/annotations/scalebar_minimal.c`, `scalebar_2d_3d.c` | `v0.4 required` / `needs-rc1-proof` | Keep at least one narrow scale-bar smoke in the RC1 proof set. |
| `examples/c/techniques/depth_cue.c`, `edl.c`, `depth_peel.c`, `wboit.c` | `v0.4 fixture-only` | Keep as technique regression examples; polished screenshots may be gallery material. |
| `examples/c/techniques/gui_viewport.c`, `gui_multi_viewport.c` | `v0.4 experimental` | Useful native GUI/runtime examples, but not scene-core release blockers. |
| `examples/c/showcase/protein.c` | `v0.4 required` | Main native C flagship candidate. |
| `examples/c/showcase/lidar.c` | `v0.4 required` | Dense point/EDL/performance showcase candidate. |
| `examples/c/showcase/brain.c`, `ibl_brain.c` | `v0.4 required` for narrow slice | Keep as volume/mesh/transparency showcase lane; defer full atlas interaction. |
| `examples/c/showcase/spatial_omics.c` | `v0.4 experimental` | Useful large point/pixel stress demo; polish can slip if text/selection are incomplete. |
| `examples/c/showcase/labels.c` | `v0.4 experimental` | First-class integer labels visual with categorical legend, shader-side selection/boundary styling, and probe-backed hover/click lookup. |
| `examples/c/tools/export_point_wgsl.c`, `export_primitive_wgsl.c`, `export_image_wgsl.c` | `v0.4 fixture-only` | WGSL/WebGPU stream generation fixtures. |
| `examples/c/tools/raw_triangle_drp2.c`, `record_dvzr.c`, `replay_dvzr.c`, `hosted_glfw_smoke.c` | `v0.4 fixture-only` | Protocol/runtime/hosted-boundary validation. |
| `examples/c/tools/raw_triangle.c` | `v0.4 experimental` | Low-level developer example, not scene release narrative. |


## Cross-Checks From Examples Back To Features

These decisions refine the feature roadmap:

1. **Text and axes are implemented first slices, not absent.** Basic text rendering exists in
   `examples/c/visuals/text.c`, and axes/grid API usage exists in
   `examples/c/techniques/scatter_axes.c`; the next RC1 need is release proof and honest status,
   not treating text/axes/colorbars as planning-only blockers. DPI, clipping, collision behavior,
   richer formatting, and gallery polish can move to RC2 unless a required example fails.
2. **Axes/colorbars should be v0.4 required, but narrow.** Linear 2D axes and continuous colorbars
   are enough for v0.4; log/date/categorical/geographic axes and rich legends can wait.
3. **Vector/arrow visual can slip to v0.5 only if `SHOWCASE_WIND_FIELD` uses primitive arrows in
   v0.4.** The example should still document the substitution so the semantic vector visual remains
   visible in the roadmap.
4. **Retained textured mesh is required for v0.4.** Terrain, planet, Mars, and textured scientific
   surface examples should use real mesh UVs and texture sampling; baked vertex colors are not an
   acceptable substitute for the release feature proof.
5. **Marker picking is the first picking expansion after point/image.** It is smaller than mesh or
   volume picking and protects scatter/selection use cases.
6. **Tracks and tractography should be a real v0.5 lane.** v0.4 can use path/segment fallbacks, but
   v0.5 should deliver polished track/tractography viewers with packed ragged paths,
   per-streamline identity, direction coloring, arcball navigation, basic selection, and
   high-quality thin-line rendering. The future `tube` visual, tube/ribbon geometry, and
   out-of-core million-streamline collections remain later extensions.
7. **A narrow brain/volume showcase belongs in v0.4, but full atlas explorer belongs in v0.5.** This
   avoids blocking v0.4 on region picking, UI trees, and linked 2D/3D atlas workflows.
8. **One dense streaming example is enough for v0.4.** Choose DAQ or physiology and keep it simple;
   reserve full ring-buffer/LOD policies for v0.5.
9. **CPU-side fluid/particle advection can be a v0.4 stretch; GPU particles remain later.** A
   vortex-street or particle-flow example can use dynamic image/point/path updates today, while
   compute-written render buffers need a proper scene compute/framegraph contract.
10. **Splats can join v0.4 only as experimental showcase scope.** If a first-class retained splat
   visual lands soon, add one dense Gaussian-like point-cloud showcase with deterministic capture.
   Do not make splats a feature-freeze blocker, and keep full Gaussian-splat pipelines later.
11. **Scale bars need an RC1 fixture and the v0.4 update-performance refactor.** The first retained
   scale-bar slices are active; live panzoom/domain updates should not rebuild glyph/text resources
   unless the formatted label or relevant style changed. Richer layout and exotic units can wait.
12. **Scene-level compute/custom shaders should not block v0.4.** Gray-Scott, Mandelbrot, and
   particles should wait until there is a proper scene-level resource/material API.
13. **WebGPU/WASM should be example-visible in v0.4, but explicitly experimental.** The example
   promise is a supported subset, not parity.


## Suggested v0.4 Example Pickup Order

1. `POINT_2D.md`
2. `PATH_AXES_2D.md`
3. `LINKED_PANELS_AXES_PANZOOM.md`
4. `SCALEBAR_2D_3D.md` or a minimal scale-bar C smoke
5. `API_SCALE_COLORBAR_ANNOTATION.md`
6. `API_IMAGE_PROBE_PINNED_READOUT.md`
7. `LINKED_PANELS_PROBE_COLORBAR.md`
8. `MARKER_PICKING.md`
9. `VOLUME_SLICE.md`
10. `PROTEIN_ARCBALL_VIEWER.md`
11. `SHOWCASE_WIND_FIELD.md`
12. retained textured terrain / planet showcase
13. narrow Allen/IBL volume + transparent mesh showcase
14. LiDAR/dense point cloud with EDL
15. WebGPU/WASM minimal browser subset

This order follows the release proof path: smallest retained scene -> explanatory objects and scale
bars -> linked panels and readouts -> picking/selection -> volume/3D/showcases -> experimental
browser path.
