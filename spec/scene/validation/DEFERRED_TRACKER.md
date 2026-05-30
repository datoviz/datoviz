# Deferred Items Tracker

This document consolidates all explicitly deferred items across the scene and DRP2 specs.
It is informative — the authoritative deferral note lives in the originating document.

Items are grouped by target milestone.

Status note on 2026-05-25: this tracker no longer lists capabilities that have landed in the
active scene/app slice, including retained point/primitive/mesh/path/image/pixel/sphere/volume
visuals, semantic text and rendered glyphs, retained sampled fields, rendered continuous colorbars,
WBOIT/depth-peel/EDL/SSAO/G-buffer execution slices, app/offscreen/GLFW presentation, DVZR app
recording/replay, point/marker/pixel pick readback, basic image-probe readback, and basic
volume-slice probe/readout behavior. Deferred entries below are gaps beyond those active slices.


## DRP2 2.1

These items are deferred at the DRP2 protocol level.

| Item | Source |
|---|---|
| Level 3 output conformance (cross-backend pixel parity) | `spec/drp2/CONFORMANCE.md` |
| GPU execution harness for Level 2 mechanical verification | `spec/drp2/CONFORMANCE.md` |
| Remaining deferred DRP2 commands (lower priority) | `spec/drp2/schema/DEFERRED.md` |


## v0.4+

These items are in scope for v0.4 but deferred beyond the initial implementation target.

| Item | Source |
|---|---|
| Per-panel render scale for image export | `spec/scene/export/IMAGE_EXPORT.md` |
| Lanczos and high-quality downsampling filters for export | `spec/scene/export/IMAGE_EXPORT.md` |
| GPU compute pre-pass for non-linear projection (e.g., geographic) | `spec/scene/semantics/NONLINEAR_TRANSFORMS.md` |
| Non-linear transform interaction with axes | `spec/scene/semantics/NONLINEAR_TRANSFORMS.md` |
| Path dashes, first-class closed-path API, path/subpath identity picking, SVG import, data-space stroke width, and WGSL parity beyond the active stroked path/caps/joins/subpath slice | `spec/scene/visuals/PATH.md` |
| Future tube visual: impostor tubes, mesh tubes, ribbons, curve-id picking, and tube-specific pass policy | `spec/scene/visuals/TUBE.md` |
| `volume` multiplane / MPR render mode | `spec/scene/visuals/VOLUME.md` |
| Richer volume transfer functions and isosurface controls beyond the active first slice | `spec/scene/visuals/VOLUME.md` |
| Volume DVR/MIP ray-cast picking identity | `spec/scene/interaction/PICKING.md` |
| Automatic LOD for mesh simplification | `spec/scene/semantics/GEOMETRY_UTILITIES.md` |
| Concave (alpha-shape) hull computation | `spec/scene/semantics/GEOMETRY_UTILITIES.md` |
| Annotation shape generation (msdfgen integration) | `spec/scene/semantics/GEOMETRY_UTILITIES.md` |
| Advanced text shaping, fallback chains, equation layout, diagnostics, and data/world placement polish beyond the active rendered text path | `spec/scene/semantics/TEXT.md`, `spec/scene/visuals/GLYPH.md` |
| Glyph-level, substring-level, and text-object picking | `spec/scene/semantics/TEXT.md`, `spec/scene/visuals/GLYPH.md` |
| Non-label rendered annotations, rich readout UI, guides, and callout geometry beyond the active label and scale-bar slices | `spec/scene/semantics/ANNOTATIONS.md` |
| Shared colorbar/legend layout, interactive scale/range editing, and richer legend composition beyond the active continuous colorbar and categorical legend slices | `spec/scene/semantics/LEGENDS_AND_COLORBARS.md` |
| Labels GPU probing, 3D label slices, GPU-only label resources, and large sparse-id pressure tests beyond the active 2D integer-label rendering slice | `spec/scene/proposals/promoted/LABELS_VISUAL_DESIGN.md` |
| Richer probe payloads beyond basic point identity and image RGBA/value readback | `spec/scene/proposals/promoted/PROBE_READOUT_DESIGN.md` |
| Richer mesh face/region, path/subpath, label, text, volume ray-hit, object, and grouped-family picking beyond the active broad item-pick slices | `spec/scene/interaction/PICKING.md` |
| Scene-level animation timeline coordination | `spec/scene/interaction/ANIMATION.md` |
| Multi-scene GPU resource sharing across threads | `spec/scene/integration/THREAD_SAFETY.md` |
| Polar axis geometry (circular gridlines, radial labels) | `spec/scene/pipeline/TRANSFORM_PIPELINE.md` |
| Log/symlog/datetime axes and explicit screen/panel/data/world/NDC coordinate-system gallery examples beyond the active linear 2D axes slice | `spec/scene/examples/PLANNING.md`, `spec/scene/pipeline/TRANSFORM_PIPELINE.md` |
| Map tile loading and LOD management | `spec/scene/pipeline/TRANSFORM_PIPELINE.md` |
| Per-item material / PBR lighting for sphere | `spec/scene/visuals/SPHERE.md` |
| Full PBR material model beyond current material/Phong/depth-cue slices | `spec/scene/semantics/LIGHTING.md`, `spec/scene/proposals/active/MATERIAL_LIGHTING_API.md` |
| Selection state synchronization across scenes | `spec/scene/interaction/SELECTION.md` |
| Selection/item-state highlighting beyond point/pixel/marker: splat, image, mesh/path/volume/text, per-visual style overrides, richer effects | `spec/scene/interaction/SELECTION.md`, `spec/scene/proposals/promoted/SELECTION_HIGHLIGHT_DESIGN.md` |
| Mesh scalar colormap mode, automatic normal generation, edge overlay, isolines, shape-builder integration, face/region picking, normal maps, multi-texture materials, and PBR beyond the v0.4-required retained diffuse-texture slice | `spec/scene/visuals/MESH.md` |
| Full Gaussian-splat pipeline beyond the possible v0.4 experimental retained splat visual: trained asset formats, differentiable rendering, out-of-core splat scenes, advanced splat LOD, and production splat asset tooling | `spec/scene/examples/PLANNING.md`, `spec/scene/examples/PLANNING.md` |
| Custom visual dirty-tracking optimization | `spec/scene/integration/CUSTOM_VISUALS.md` |
| Per-character glyph orientation | `spec/scene/visuals/GLYPH.md` |
| Vector export tile-based rendering and SVG-level effects | `spec/scene/export/VECTOR_EXPORT.md` |
| Python/CuPy/host adapter API layer over the low-level interop/runtime hooks | `spec/scene/integration/CUPY_CUDA_INTEROP.md`, `spec/scene/integration/HOSTED_BACKENDS.md` |
| Native multi-window/fullscreen/HiDPI showcase variants, high-resolution and transparent-background export workflows, batch/server-side rendering, remote/cloud/thin-client viewers, and public visual-diagnostics gallery examples | `spec/scene/examples/PLANNING.md`, `spec/scene/examples/FIXTURES.md` |
| Large-data rendering strategy gallery: density rendering, progressive refinement, tile streaming, GPU instancing, and out-of-core resource policies beyond current point/pixel/path update proofs | `spec/scene/examples/PLANNING.md`, `spec/scene/examples/PLANNING.md` |
| Full exact OIT beyond active WBOIT and depth-peeling modes | `spec/scene/semantics/TRANSPARENCY.md` |
| `wiggle` promotion to a full visual family | `spec/scene/semantics/VISUAL_FAMILIES.md` |


## No Target Set

These items have no assigned milestone and are indefinitely deferred.

| Item | Source |
|---|---|
| Hardware ray tracing capability class | `spec/scene/validation/ADAPTATION.md` |
| `RayTraceNode` plan node kind | `spec/scene/pipeline/FRAME_PLAN.md` |
