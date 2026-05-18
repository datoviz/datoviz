# Promoted Scene Proposal Notes

These files are mostly or partially represented by specialized specs. Keep them as rationale,
backlog, and API-shape notes; update the specialized spec first when changing active behavior.


## Authority Note

The files in this directory are not primary implementation-facing specs. Their detailed sections
may preserve useful historical requirements, API sketches, and tradeoffs, but current rules belong
in the specialized `spec/scene/` documents listed below.

1. [PICKING_DESIGN.md](PICKING_DESIGN.md) -> `../../interaction/PICKING.md`, visual-family specs,
   and `../../api/API_SURFACE.md`.
2. [PROBE_READOUT_DESIGN.md](PROBE_READOUT_DESIGN.md) -> `../../interaction/PICKING.md`,
   `../../semantics/ANNOTATIONS.md`, image/volume specs, and `../../api/API_SURFACE.md`.
3. [RESOURCE_UPDATE_DESIGN.md](RESOURCE_UPDATE_DESIGN.md) -> `../../pipeline/RESOURCE_MODEL.md`
   and `../../pipeline/INVALIDATION_AND_CACHING.md`.
4. [SAMPLED_FIELD_API_DESIGN.md](SAMPLED_FIELD_API_DESIGN.md) ->
   `../../pipeline/RESOURCE_MODEL.md`, image/volume specs, and `../../api/API_SURFACE.md`.
5. [TRANSPARENCY_WBOIT_DESIGN.md](TRANSPARENCY_WBOIT_DESIGN.md) ->
   `../../semantics/TRANSPARENCY.md`, `../../pipeline/FRAME_PLAN.md`, and
   `../../validation/ADAPTATION.md`.
6. [UI_BACKEND_INTEGRATION.md](UI_BACKEND_INTEGRATION.md) ->
   `../../integration/EXTERNAL_UI.md`, `../../integration/HOSTED_BACKENDS.md`,
   `../../pipeline/FRAME_LIFECYCLE.md`, and `../../core/RUNTIME_BOUNDARY.md`.
