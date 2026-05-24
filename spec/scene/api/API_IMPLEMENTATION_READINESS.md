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
| Visual families | pixel, point, primitive, path, image, mesh, sphere, volume, glyph constructors | active for those families | active for retained first slices, including WBOIT/depth-peel, EDL, SSAO/G-buffer where eligible; glyph path renders atlas-backed text | point pick and image probe only | richer marker/segment/glyph semantics, errorbar, boxplot, richer path/image/volume features |
| Sampled fields/scales | `DvzSampledField`, scale, colormap, colorbar APIs | fields/scales/colorbars retain state | image and volume consume fields; image/volume colormap bindings are active | image probe returns a basic value payload | labels/categorical fields, richer probe payloads, rendered colorbar ticks/labels |
| Interaction/selection | policies, pick/probe queues, selection/link APIs | active bookkeeping and tests | request processing executes through app/runtime for point/image | narrow point/image GPU readback | broader mesh/object/sphere/volume picking and rendered selection highlights |
| Text/annotations | font, text, annotation APIs | active bookkeeping and lifecycle tests; semantic `DvzText` migration pending | first rendered glyph/text path active; label annotations use text lowering | no | semantic text API migration, data/world placement, colorbar/readout integration, shaping, diagnostics, glyph/text picking |

The implementation-ready packets for the retained and explanatory-object rows are:

1. [../slices/TEXT_RENDERING_SLICE.md](../slices/TEXT_RENDERING_SLICE.md),
2. [../slices/ANNOTATION_LABEL_SLICE.md](../slices/ANNOTATION_LABEL_SLICE.md),
3. [../slices/COLORBAR_RENDERING_SLICE.md](../slices/COLORBAR_RENDERING_SLICE.md),
4. [../slices/LEGEND_SLICE.md](../slices/LEGEND_SLICE.md).

Remaining implementation work should focus on:

1. broadening pick/probe coverage beyond the first point/image DRP2 readback paths,
2. promoting semantic `DvzText` before adding more text placement behavior to the visual-backed API,
3. deciding whether mesh needs a separate public geometry resource beyond current scene buffers,
4. extending volume/sampled-field probe results beyond the current image-oriented payload,
5. implementing rendered selection highlights once selection/link bookkeeping has a stable visual
   target.


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
