# Scene Proposals

This directory contains active scene design proposals and spec addenda that are not yet fully
absorbed into the specialized normative spec files.

These files used to live in `spec/scene/decisions/`. They are not purely historical decisions:
many contain API shape, implementation constraints, normative rules, and promotion notes. Keeping
them here makes that staging status explicit.


## Authority Model

Use this authority order inside the scene spec:

1. specialized normative spec files are the primary source of implementation-facing rules,
2. proposals clarify or extend the current planning baseline when a specialized spec has not yet
   absorbed the topic,
3. if a proposal and a specialized spec disagree, update the specialized spec and keep a short note
   in the proposal,
4. examples remain informative pressure tests and do not override proposals or normative specs,
5. historical records in `../decisions/` explain rationale but do not carry current normative
   authority on their own.


## Promotion Targets

Use this map when turning proposals into implementation-ready spec changes.

1. `PICKING_DESIGN.md`, `SELECTION_HIGHLIGHT_DESIGN.md`, `INTERACTION_API_DESIGN.md`, and
   `PROBE_READOUT_DESIGN.md` promote into `../interaction/PICKING.md`,
   `../interaction/SELECTION.md`, `../interaction/CONTROLLERS.md`,
   `../interaction/EVENT_CALLBACKS.md`, and `../api/API_SURFACE.md`.
2. `ANNOTATION_MEASUREMENT_DESIGN.md`, `ANNOTATION_TEXT_SCALE_API.md`,
   `COLORBAR_COLORMAP_DESIGN.md`, `AXES_DOMAIN_DESIGN.md`, and `TEXT_DESIGN.md` promote into
   `../semantics/ANNOTATIONS.md`, `../semantics/SCALES.md`,
   `../semantics/LEGENDS_AND_COLORBARS.md`, `../semantics/AXES.md`, `../semantics/TEXT.md`,
   and `../api/API_SURFACE.md`.
3. `MESH_API_DESIGN.md`, `MESH_SHADING_DESIGN.md`, `MATERIAL_LIGHTING_API.md`,
   `PARTICLE_SYSTEM_DESIGN.md`, `TRANSPARENCY_WBOIT_DESIGN.md`, `VOLUME_DESIGN.md`, and
   `RAY_TRACING_FORWARD_COMPAT.md` promote into `../visuals/MESH.md`,
   `../visuals/VOLUME.md`, `../semantics/LIGHTING.md`, `../semantics/TRANSPARENCY.md`,
   `../semantics/VISUAL_CONTRACT.md`, `../pipeline/FRAME_PLAN.md`, and
   `../core/RUNTIME_BOUNDARY.md`.
4. `RESOURCE_UPDATE_DESIGN.md`, `SCIENTIFIC_COORDINATE_NORMALIZATION.md`,
   `ASSET_BOUNDARY_DESIGN.md`, `CAPABILITY_FALLBACK_DESIGN.md`,
   `SAMPLED_FIELD_API_DESIGN.md`,
   `TRANSFORM_CONTROLLER_DESIGN.md`, and `UI_BACKEND_INTEGRATION.md` promote into
   `../pipeline/RESOURCE_MODEL.md`, `../pipeline/INVALIDATION_AND_CACHING.md`,
   `../pipeline/TRANSFORM_PIPELINE.md`, `../validation/ADAPTATION.md`,
   `../validation/VALIDATION.md`, `../visuals/IMAGE.md`, `../visuals/VOLUME.md`,
   `../api/API_SURFACE.md`, and `../integration/EXTERNAL_UI.md`.
5. `GEOM_DESIGN.md` promotes into `../semantics/GEOMETRY_UTILITIES.md` and any future public
   `geom` module header plan.
6. `HIGH_PRIORITY_SPEC_DECISIONS.md` is a consolidation checkpoint. Do not use it as the final
   authority when a more focused proposal or specialized spec exists.


## Proposal Index

Interaction and event behavior:

1. [INTERACTION_API_DESIGN.md](INTERACTION_API_DESIGN.md)
2. [PICKING_DESIGN.md](PICKING_DESIGN.md)
3. [PROBE_READOUT_DESIGN.md](PROBE_READOUT_DESIGN.md)
4. [SELECTION_HIGHLIGHT_DESIGN.md](SELECTION_HIGHLIGHT_DESIGN.md)
5. [TRANSFORM_CONTROLLER_DESIGN.md](TRANSFORM_CONTROLLER_DESIGN.md)

Text, axes, scales, annotations, and explanatory objects:

1. [ANNOTATION_MEASUREMENT_DESIGN.md](ANNOTATION_MEASUREMENT_DESIGN.md)
2. [ANNOTATION_TEXT_SCALE_API.md](ANNOTATION_TEXT_SCALE_API.md)
3. [AXES_DOMAIN_DESIGN.md](AXES_DOMAIN_DESIGN.md)
4. [COLORBAR_COLORMAP_DESIGN.md](COLORBAR_COLORMAP_DESIGN.md)
5. [TEXT_DESIGN.md](TEXT_DESIGN.md)

Visual families, material, lighting, and render modes:

1. [MESH_API_DESIGN.md](MESH_API_DESIGN.md)
2. [MESH_SHADING_DESIGN.md](MESH_SHADING_DESIGN.md)
3. [MATERIAL_LIGHTING_API.md](MATERIAL_LIGHTING_API.md)
4. [PARTICLE_SYSTEM_DESIGN.md](PARTICLE_SYSTEM_DESIGN.md)
5. [TRANSPARENCY_WBOIT_DESIGN.md](TRANSPARENCY_WBOIT_DESIGN.md)
6. [VOLUME_DESIGN.md](VOLUME_DESIGN.md)
7. [RAY_TRACING_FORWARD_COMPAT.md](RAY_TRACING_FORWARD_COMPAT.md)

Resources, transforms, geometry, validation, and integration:

1. [ASSET_BOUNDARY_DESIGN.md](ASSET_BOUNDARY_DESIGN.md)
2. [CAPABILITY_FALLBACK_DESIGN.md](CAPABILITY_FALLBACK_DESIGN.md)
3. [GEOM_DESIGN.md](GEOM_DESIGN.md)
4. [RESOURCE_UPDATE_DESIGN.md](RESOURCE_UPDATE_DESIGN.md)
5. [SAMPLED_FIELD_API_DESIGN.md](SAMPLED_FIELD_API_DESIGN.md)
6. [SCIENTIFIC_COORDINATE_NORMALIZATION.md](SCIENTIFIC_COORDINATE_NORMALIZATION.md)
7. [UI_BACKEND_INTEGRATION.md](UI_BACKEND_INTEGRATION.md)

Cross-cutting checkpoint:

1. [HIGH_PRIORITY_SPEC_DECISIONS.md](HIGH_PRIORITY_SPEC_DECISIONS.md)


## Immediate Use

For the next public API pass, read these first:

1. [../api/API_SURFACE.md](../api/API_SURFACE.md)
2. [INTERACTION_API_DESIGN.md](INTERACTION_API_DESIGN.md)
3. [ANNOTATION_TEXT_SCALE_API.md](ANNOTATION_TEXT_SCALE_API.md)
4. [PICKING_DESIGN.md](PICKING_DESIGN.md)
5. [PROBE_READOUT_DESIGN.md](PROBE_READOUT_DESIGN.md)
6. [COLORBAR_COLORMAP_DESIGN.md](COLORBAR_COLORMAP_DESIGN.md)
