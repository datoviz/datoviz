# Scene Decision Records

This directory contains scene specification decision records that were previously mixed into
`agents/now/`.

They are now spec-side material. Future agents should treat them as design records that clarify or
extend the normative scene spec, then promote settled rules into the closest specialized spec file
when implementation starts.


## Status Model

Decision records have this authority:

1. A specialized normative spec file wins when it explicitly covers the same rule.
2. A decision record wins over older examples, planning prose, or agent handoff notes.
3. If a decision record and a specialized spec disagree, update the specialized spec and keep a
   short compatibility note in the decision record.
4. Once a decision record has been fully absorbed, move it to `agents/done/` only if it describes
   completed implementation work; otherwise keep it here as historical rationale.


## Promotion Targets

Use this map when turning decision records into implementation-ready spec changes.

1. `PICKING_DESIGN.md`, `SELECTION_HIGHLIGHT_DESIGN.md`, `INTERACTION_API_DESIGN.md`, and
   `PROBE_READOUT_DESIGN.md` promote into `interaction/PICKING.md`,
   `interaction/SELECTION.md`, `interaction/CONTROLLERS.md`, `interaction/EVENT_CALLBACKS.md`,
   and `api/API_SURFACE.md`.
2. `ANNOTATION_MEASUREMENT_DESIGN.md`, `ANNOTATION_TEXT_SCALE_API.md`,
   `COLORBAR_COLORMAP_DESIGN.md`, `AXES_DOMAIN_DESIGN.md`, and `TEXT_DESIGN.md` promote into
   `semantics/ANNOTATIONS.md`, `semantics/SCALES.md`, `semantics/LEGENDS_AND_COLORBARS.md`, `semantics/AXES.md`, `TEXT.md` if added, and
   `api/API_SURFACE.md`.
3. `MESH_API_DESIGN.md`, `MESH_SHADING_DESIGN.md`, `MATERIAL_LIGHTING_API.md`,
   `TRANSPARENCY_WBOIT_DESIGN.md`, `VOLUME_DESIGN.md`, and
   `RAY_TRACING_FORWARD_COMPAT.md` promote into `visuals/MESH.md`, `visuals/VOLUME.md`,
   `semantics/LIGHTING.md`, `semantics/TRANSPARENCY.md`, `semantics/VISUAL_CONTRACT.md`, and `core/RUNTIME_BOUNDARY.md`.
4. `RESOURCE_UPDATE_DESIGN.md`, `SCIENTIFIC_COORDINATE_NORMALIZATION.md`,
   `ASSET_BOUNDARY_DESIGN.md`, `CAPABILITY_FALLBACK_DESIGN.md`, `TRANSFORM_CONTROLLER_DESIGN.md`,
   and `UI_BACKEND_INTEGRATION.md` promote into `pipeline/RESOURCE_MODEL.md`,
   `pipeline/INVALIDATION_AND_CACHING.md`, `pipeline/TRANSFORM_PIPELINE.md`, `validation/ADAPTATION.md`,
   `validation/VALIDATION.md`, and `integration/EXTERNAL_UI.md`.
5. `GEOM_DESIGN.md` promotes into `semantics/GEOMETRY_UTILITIES.md` and any future public `geom` module
   header plan.
6. `HIGH_PRIORITY_SPEC_DECISIONS.md` is a consolidation checkpoint. Do not use it as the final
   authority when a more focused decision record or specialized spec now exists.


## Immediate Use

For the next public API pass, read these first:

1. [../api/API_SURFACE.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/api/API_SURFACE.md)
2. [INTERACTION_API_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/decisions/INTERACTION_API_DESIGN.md)
3. [ANNOTATION_TEXT_SCALE_API.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/decisions/ANNOTATION_TEXT_SCALE_API.md)
4. [PICKING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/decisions/PICKING_DESIGN.md)
5. [PROBE_READOUT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/decisions/PROBE_READOUT_DESIGN.md)
6. [COLORBAR_COLORMAP_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/decisions/COLORBAR_COLORMAP_DESIGN.md)
