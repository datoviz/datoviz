# Scene API Implementation Readiness

This document summarizes the current spec-to-implementation boundary for the public scene API surface.


## Status

Status: informative implementation checklist, updated for the active v0.4 scene/app slice.

The normative sources remain:

1. [API_SURFACE.md](API_SURFACE.md),
2. [../semantics/TEXT.md](../semantics/TEXT.md),
3. [../semantics/SCALES.md](../semantics/SCALES.md),
4. [../semantics/LEGENDS_AND_COLORBARS.md](../semantics/LEGENDS_AND_COLORBARS.md),
5. [../interaction/PICKING.md](../interaction/PICKING.md),
6. [../interaction/SELECTION.md](../interaction/SELECTION.md),
7. [../visuals/MESH.md](../visuals/MESH.md),
8. [../visuals/VOLUME.md](../visuals/VOLUME.md),
9. [../headers/scene_api.h](../headers/scene_api.h) for API groups it already spells.


## Ready Boundaries

These boundaries are now explicit enough to start public API drafting:

1. The active construction path is `dvz_scene()` -> `dvz_figure()` -> `dvz_app()` ->
   `DvzAppWindow`. App windows may be Datoviz-owned GLFW/offscreen windows or hosted windows
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
7. `SampledField` is the active shared scene-owned regular-grid resource for image paths and future
   volume/probe/readout consumers.
8. The active rendering path is scene -> `FramePlan` -> `DvzDrp2CommandStream` ->
   `DvzDrp2Runtime` -> vklite/canvas/app. Scene code emits backend-agnostic DRP2 work and does
   not own swapchains, command-buffer lifetimes, or native host event loops.
9. App capture is active through `DvzAppWindow`/canvas PNG capture. DRP2 linear `.dvzr`
   recording and replay are active app-window capabilities.


## Implementation Work Still Needed

The installed headers now spell the first versions of scene, figure, app-window, interaction,
scale/colorbar, text/annotation, and sampled-field APIs. Remaining implementation work should focus
on:

1. hardening `DvzInteractionPolicy`, request/freshness handling, deterministic pick/probe result
   queues, and panel binding,
2. implementing `DvzSelection`, `DvzLinkChannel`, stable link-key storage, and selection mutation
   rules,
3. broadening pick/probe coverage beyond the first point/image DRP2 readback paths,
4. implementing `DvzFont`, `DvzText`, and `DvzAnnotation` retained-object lifecycle and invalidation,
5. rendering colorbar ticks/labels and text/annotation geometry after retained bookkeeping exists,
6. deciding whether mesh needs a separate public geometry resource beyond current scene buffers,
7. extending volume/sampled-field probe results when volume slices become active.


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
