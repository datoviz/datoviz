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
6. [TRANSPARENCY_WBOIT_DESIGN.md](TRANSPARENCY_WBOIT_DESIGN.md) ->
   `../../semantics/TRANSPARENCY.md`, `../../pipeline/FRAME_PLAN.md`, and
   `../../validation/ADAPTATION.md`.
7. [UI_BACKEND_INTEGRATION.md](UI_BACKEND_INTEGRATION.md) ->
   `../../integration/EXTERNAL_UI.md`, `../../integration/HOSTED_BACKENDS.md`,
   `../../pipeline/FRAME_LIFECYCLE.md`, and `../../core/RUNTIME_BOUNDARY.md`.
