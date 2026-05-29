# Scene API Implementation Readiness

This document summarizes the current spec-to-implementation boundary for the public scene API surface.


## Status

Status: informative implementation checklist, updated for the active v0.4 scene/app slice.

The normative sources remain:

1. [API_SURFACE.md](API_SURFACE.md),
2. [WASM_PORTABILITY.md](WASM_PORTABILITY.md),
3. [../semantics/TEXT.md](../semantics/TEXT.md),
4. [../semantics/SCALES.md](../semantics/SCALES.md),
5. [../semantics/LEGENDS_AND_COLORBARS.md](../semantics/LEGENDS_AND_COLORBARS.md),
6. [../interaction/PICKING.md](../interaction/PICKING.md),
7. [../interaction/SELECTION.md](../interaction/SELECTION.md),
8. [../visuals/MESH.md](../visuals/MESH.md),
9. [../visuals/VOLUME.md](../visuals/VOLUME.md),
10. installed headers under `include/datoviz/scene*.h` for active public names and signatures.


## Ready Boundaries

These boundaries are now explicit enough to start public API drafting:

1. The active construction path is `dvz_scene()` -> `dvz_figure()` -> `dvz_app()` ->
   `DvzView`. Views may be Datoviz-owned GLFW/offscreen windows or hosted windows
   backed by an external surface.
2. Text is a retained semantic object (`DvzText`) or annotation object that may lower to `glyph`
   visual contributions. `glyph` remains the visual-family contract for shaped glyph runs.
3. Mesh visuals reference scene-owned mesh geometry resources. Vertex/index data belongs to the
   reusable resource, while visual instances own material, transform, visibility, and picking policy.
4. Volume slice probe/readout picking is in scope for v0.4. DVR/MIP ray-cast identity remains
   deferred.
5. Picking results must expose scene identity and freshness/request identity; backend ids are never
   the only public meaning.
6. Scales and colormaps are scene-owned semantic objects. Colorbars are explanatory objects bound to
   scales; they do not own the scale or colormap.
7. `SampledField` is the active shared scene-owned regular-grid resource for image and volume paths
   and future broader probe/readout consumers.
8. The active rendering path is scene -> `FramePlan` -> `DvzDrp2CommandStream` ->
   `DvzDrp2Runtime` -> vklite/canvas/app. Scene code emits backend-agnostic DRP2 work and does
   not own swapchains, command-buffer lifetimes, or native host event loops.
9. App capture is active through `DvzView`/canvas PNG capture. DRP2 linear `.dvzr`
   recording and replay are active view capabilities.


## Implementation Status Snapshot

The installed headers now spell the first versions of scene, figure, view, interaction,
scale/colorbar, text/annotation, sampled-field, material, technique, and visual-family APIs. The
active implementation status is:

| Area | Public API | Retained state | Native rendering / execution | GPU request/readback | Remaining gaps |
|---|---|---|---|---|---|
| Core scene/app | scene, figure, panel, view, emit, capture, DVZR recording/replay | active | active scene -> FramePlan -> DRP2 -> vklite/canvas/app path | frame capture and runtime readbacks are used by tests | installed CLI boundary for DVZR replay remains a product decision |
| Visual families | pixel, point, marker, primitive, segment/path, image, labels, mesh, sphere, volume, glyph constructors; semantic polygon composites | active for those families | active for retained first slices, including WBOIT/depth-peel, EDL, SSAO/G-buffer where eligible; glyph path renders atlas-backed text; labels render integer fields | item queries exist for point-like, stroke, primitive, image, mesh, sphere, and volume proxy targets; image, labels, and volume sample queries have GPU paths | errorbar, boxplot, vector/arrow, splat, exact marker/path semantics, labels query pressure tests, and richer path/image/volume features |
| Sampled fields/scales | `DvzSampledField`, scale, colormap, colorbar, legend, and labels APIs | fields/scales/colorbars/legends/labels retain state | image, labels, and volume consume fields; image/volume colormap bindings, continuous colorbars, categorical legends, and integer label rendering are active | image queries return RGBA/scalar/category-like payloads; volume slice queries now use GPU-rendered scalar and label payloads | richer query payloads, shared colorbar/legend layout, 3D label slices, and broader labels query pressure tests |
| Interaction/selection | policies, panel query queues, selection/link APIs | active bookkeeping and tests | query processing executes through app/runtime for point-like, stroke, primitive, image, mesh, sphere, and volume proxy targets | broad item-query readback plus image/labels/volume sample query first slices | richer mesh face/region, path, label, text, and volume ray-hit identities plus broader rendered selection highlights |
| Text/annotations | font, text, annotation APIs | active semantic `DvzText` state, bookkeeping, and lifecycle tests | first rendered glyph/text path active; label annotations use text lowering | no | data/world placement, readout integration, shaping, diagnostics, glyph/text picking |

The implementation-ready packets for the retained and explanatory-object rows are:

1. [../slices/TEXT_RENDERING_SLICE.md](../slices/TEXT_RENDERING_SLICE.md),
2. [../slices/ANNOTATION_LABEL_SLICE.md](../slices/ANNOTATION_LABEL_SLICE.md),
3. [../slices/COLORBAR_RENDERING_SLICE.md](../slices/COLORBAR_RENDERING_SLICE.md),
4. [../slices/LEGEND_SLICE.md](../slices/LEGEND_SLICE.md).

Remaining implementation work should focus on:

1. broadening query payload quality beyond the first broad item and sampled-value readback paths,
2. hardening semantic `DvzText` placement before adding broader text-dependent behavior,
3. deciding whether mesh needs a separate public geometry resource beyond current scene buffers,
4. extending volume/sampled-field query results beyond the current image-oriented payload,
5. implementing rendered selection highlights once selection/link bookkeeping has a stable visual
   target.


## Implementation Note: Query Status

As of 2026-05-30, the native scene query executor implements GPU-backed item queries for point,
pixel, marker, sphere, segment/stroke, path, primitive, mesh, image, and volume proxy visuals. It
also implements GPU-backed image, labels, and volume slice sample queries. The implemented executor
is narrower than the public capability and target model: native execution currently covers the
first item/pixel/sample targets, while the API names object, vertex, face, group, text, and
annotation-style targets for future expansion.

Glyph/text identity queries are still unimplemented. `DvzQueryHitPolicy` is queued but not yet
materially applied by native execution, and richer payload fields such as `instance_id`,
`data_position`, mesh face/region identity, image texel identity, and volume ray/sample identity still
need backing implementation before the API should document them as available.


## Non-Blocking Follow-Up

These items should not block the first header pass, but they should stay visible:

1. a dedicated public atlas resource handle can wait until the first text implementation needs it,
2. glyph-level text picking is deferred,
3. per-character glyph orientation is deferred,
4. volume DVR/MIP ray-cast picking is deferred,
5. PBR material expansion beyond reserved mesh/sphere fields is deferred,
6. multi-scene resource sharing and advanced custom-visual dirty tracking are deferred,
7. complete multi-format sampled-field runtime support may lag the first public descriptor set as
   long as unsupported formats fail validation explicitly.
