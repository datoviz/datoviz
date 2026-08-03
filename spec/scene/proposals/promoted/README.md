# Promoted Scene Proposal Notes

These files are mostly or partially represented by specialized specs. Keep them short: status,
decision record, canonical links, unresolved choices, and any API or precision notes not yet
represented elsewhere. Update the specialized spec first when changing active behavior.


## Authority Note

The files in this directory are not primary implementation-facing specs. They preserve historical
requirements, API sketches, and tradeoffs only where those details have not moved into the
specialized `spec/scene/` documents listed below.

1. [PICKING_DESIGN.md](PICKING_DESIGN.md) -> `../../interaction/PICKING.md`, visual-family specs,
   and `../../api/API_SURFACE.md`.
2. [PROBE_READOUT_DESIGN.md](PROBE_READOUT_DESIGN.md) -> `../../interaction/PICKING.md`,
   `../../semantics/ANNOTATIONS.md`, image/volume specs, and `../../api/API_SURFACE.md`.
3. [RESOURCE_UPDATE_DESIGN.md](RESOURCE_UPDATE_DESIGN.md) -> `../../pipeline/RESOURCE_MODEL.md`
   and `../../pipeline/INVALIDATION_AND_CACHING.md`.
4. [SAMPLED_FIELD_API_DESIGN.md](SAMPLED_FIELD_API_DESIGN.md) ->
   `../../pipeline/RESOURCE_MODEL.md`, image/volume specs, and `../../api/API_SURFACE.md`.
5. [PANEL_RESERVE_AND_COLORBAR_PLACEMENT.md](PANEL_RESERVE_AND_COLORBAR_PLACEMENT.md) ->
   `../../core/PANEL_LAYOUT.md`, `../../semantics/LEGENDS_AND_COLORBARS.md`, and
   `../../api/API_SURFACE.md`.
6. [PANEL_CONTENT_PADDING.md](PANEL_CONTENT_PADDING.md) -> `../../core/PANEL_LAYOUT.md`.
7. [EQUAL_ASPECT_PANEL_VIEW.md](EQUAL_ASPECT_PANEL_VIEW.md) -> `../../core/PANEL_LAYOUT.md`,
   `../../pipeline/TRANSFORM_PIPELINE.md`, `../../interaction/CONTROLLERS.md`,
   `../../semantics/AXES.md`, and `../../api/API_SURFACE.md`.
8. [TRANSPARENCY_WBOIT_DESIGN.md](TRANSPARENCY_WBOIT_DESIGN.md) ->
   `../../semantics/TRANSPARENCY.md`, `../../pipeline/FRAME_PLAN.md`, and
   `../../validation/ADAPTATION.md`.
9. [UI_BACKEND_INTEGRATION.md](UI_BACKEND_INTEGRATION.md) ->
   `../../integration/EXTERNAL_UI.md`, `../../integration/HOSTED_BACKENDS.md`,
   `../../pipeline/FRAME_LIFECYCLE.md`, and `../../core/RUNTIME_BOUNDARY.md`.
10. [INTERACTION_API_DESIGN.md](INTERACTION_API_DESIGN.md),
   [GPU_PROBE_READBACK_ARCHITECTURE.md](GPU_PROBE_READBACK_ARCHITECTURE.md),
   [SELECTION_HIGHLIGHT_DESIGN.md](SELECTION_HIGHLIGHT_DESIGN.md), and
   [TRANSFORM_CONTROLLER_DESIGN.md](TRANSFORM_CONTROLLER_DESIGN.md) ->
   `../../interaction/PANEL_QUERY.md`, `../../interaction/GPU_QUERY_SYSTEM.md`,
   `../../interaction/SELECTION.md`, `../../interaction/CONTROLLERS.md`, and
   `../../api/API_SURFACE.md`.
11. [ANNOTATION_TEXT_SCALE_API.md](ANNOTATION_TEXT_SCALE_API.md),
    [ANNOTATION_MEASUREMENT_DESIGN.md](ANNOTATION_MEASUREMENT_DESIGN.md),
    [AXES_DOMAIN_DESIGN.md](AXES_DOMAIN_DESIGN.md),
    [COLORBAR_COLORMAP_DESIGN.md](COLORBAR_COLORMAP_DESIGN.md), and
    [TEXT_DESIGN.md](TEXT_DESIGN.md) ->
    `../../semantics/TEXT.md`, `../../semantics/SCALES.md`,
    `../../semantics/LEGENDS_AND_COLORBARS.md`, `../../semantics/AXES.md`,
    `../../semantics/ANNOTATIONS.md`, and `../../api/API_SURFACE.md`.
12. [GEOM_DESIGN.md](GEOM_DESIGN.md) -> `../../semantics/GEOMETRY_UTILITIES.md`.
13. [LABELS_VISUAL_DESIGN.md](LABELS_VISUAL_DESIGN.md) ->
    `../../visuals/LABELS.md`, `../../semantics/SCALES.md`,
    `../../semantics/LEGENDS_AND_COLORBARS.md`, and
    `../../interaction/GPU_QUERY_SYSTEM.md`.
14. [MESH_API_DESIGN.md](MESH_API_DESIGN.md),
    [MESH_SHADING_DESIGN.md](MESH_SHADING_DESIGN.md), and
    [VOLUME_DESIGN.md](VOLUME_DESIGN.md) ->
    `../../visuals/MESH.md`, `../../visuals/VOLUME.md`, `../../semantics/LIGHTING.md`,
    `../../semantics/TRANSPARENCY.md`, and `../../pipeline/FRAME_PLAN.md`.
15. [SCREEN_SPACE_EFFECTS_DESIGN.md](SCREEN_SPACE_EFFECTS_DESIGN.md) ->
    `../../semantics/EFFECTS.md` and `../../implementation/GRAPH_TECHNIQUES.md`.
16. [RENDER_CONTRACT_RESOLVER.md](RENDER_CONTRACT_RESOLVER.md) and [RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md](RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md) -> `../../pipeline/FRAME_PLAN.md`, `../../implementation/GRAPH_TECHNIQUES.md`, `../../implementation/OCCLUSION_EFFECTS.md`, `../../implementation/TRANSPARENCY_MSAA.md`, `../../semantics/EFFECTS.md`, and the DRP2 specifications.
17. [VISUAL_ITEM_RANGES.md](VISUAL_ITEM_RANGES.md) -> installed scene headers, visual state/lowering, and query tests; the retained point-range slice is implemented.
18. [COMPUTE_GRAPHICS_INTEROP.md](COMPUTE_GRAPHICS_INTEROP.md) -> the DRP2 synchronization contract, scene compute API, WebGPU fixtures, and the experimental GPU particle showcase; broader compute and CUDA facilities remain outside the portable slice.
