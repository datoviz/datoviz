# Scene Proposals

This directory contains scene design proposals and spec addenda organized by current authority and
maturity.

These files used to live in `spec/scene/decisions/`. They are not purely historical decisions:
many contain API shape, implementation constraints, normative rules, and promotion notes. Keeping
them under `proposals/` makes that staging status explicit, while the subdirectories separate active
work from promoted rationale, future roadmaps, and historical checkpoints.


## Directory Map

1. [active](active/README.md): current proposal-stage design addenda that still influence v0.4
   implementation or API work.
2. [promoted](promoted/README.md): mostly or partially absorbed rationale notes. Treat the
   specialized spec files as primary authority for these topics.
3. [future](future/README.md): v0.5+, exploratory, or pressure-test roadmaps. These should not
   block v0.4 implementation unless a current plan explicitly pulls one forward.
4. [history](history/README.md): historical consolidation notes kept for rationale only.


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


## Metadata Convention

Proposal files should start with an `Execution Status` block before the title:

1. `Status` names maturity and authority, not just topic,
2. `Updated on` records the latest semantic review date,
3. `Purpose` explains why the file remains in the proposal corpus,
4. optional fields such as `Scope`, `Primary gap`, or `Primary pressure tests` may follow when they
   make ownership clearer.


## Promotion Status

Some mature proposals are now mostly represented by specialized specs. Keep those proposals as
design rationale and backlog notes; do not treat them as the primary rule source when the listed
specialized spec exists.

Mostly promoted notes now live in [promoted](promoted/README.md):

1. [TRANSPARENCY_WBOIT_DESIGN.md](promoted/TRANSPARENCY_WBOIT_DESIGN.md) ->
   `../semantics/TRANSPARENCY.md`, with frame-plan and
   adaptation details owned by `../pipeline/FRAME_PLAN.md` and `../validation/ADAPTATION.md`.
2. [SAMPLED_FIELD_API_DESIGN.md](promoted/SAMPLED_FIELD_API_DESIGN.md) ->
   `../pipeline/RESOURCE_MODEL.md`,
   `../visuals/IMAGE.md`, `../visuals/VOLUME.md`, and `../api/API_SURFACE.md`.
3. [RESOURCE_UPDATE_DESIGN.md](promoted/RESOURCE_UPDATE_DESIGN.md) ->
   `../pipeline/RESOURCE_MODEL.md` and
   `../pipeline/INVALIDATION_AND_CACHING.md`.
4. [PICKING_DESIGN.md](promoted/PICKING_DESIGN.md) ->
   `../interaction/PICKING.md`, visual-family specs, and
   `../api/API_SURFACE.md`.
5. [PROBE_READOUT_DESIGN.md](promoted/PROBE_READOUT_DESIGN.md) ->
   `../interaction/PICKING.md`,
   `../semantics/ANNOTATIONS.md`, `../visuals/IMAGE.md`, `../visuals/VOLUME.md`, and
   `../api/API_SURFACE.md`.
6. [PANEL_RESERVE_AND_COLORBAR_PLACEMENT.md](promoted/PANEL_RESERVE_AND_COLORBAR_PLACEMENT.md) ->
   `../core/PANEL_LAYOUT.md`, `../semantics/LEGENDS_AND_COLORBARS.md`, and
   `../api/API_SURFACE.md`.
7. [UI_BACKEND_INTEGRATION.md](promoted/UI_BACKEND_INTEGRATION.md) ->
   `../integration/EXTERNAL_UI.md`,
   `../integration/HOSTED_BACKENDS.md`, `../pipeline/FRAME_LIFECYCLE.md`, and
   `../core/RUNTIME_BOUNDARY.md`.

When changing active behavior for these topics, update the specialized spec first and add only a
short cross-reference or remaining-work note in the proposal.

Implementation-ready work packets now live in `../slices/`. Use those files for concrete code
slices once the specialized specs and proposals already agree on behavior.


## Promotion Targets

Use this map when turning proposals into implementation-ready spec changes.

1. [PICKING_DESIGN.md](promoted/PICKING_DESIGN.md),
   [SELECTION_HIGHLIGHT_DESIGN.md](active/SELECTION_HIGHLIGHT_DESIGN.md),
   [INTERACTION_API_DESIGN.md](active/INTERACTION_API_DESIGN.md),
   [ASYNC_CALLBACKS.md](active/ASYNC_CALLBACKS.md), and
   [PROBE_READOUT_DESIGN.md](promoted/PROBE_READOUT_DESIGN.md) promote into
   `../interaction/PICKING.md`,
   `../interaction/SELECTION.md`, `../interaction/CONTROLLERS.md`,
   `../interaction/EVENT_CALLBACKS.md`, and `../api/API_SURFACE.md`.
   [CONTROLLER_INSPECTORS_AND_GIZMOS.md](active/CONTROLLER_INSPECTORS_AND_GIZMOS.md) promotes into
   `../integration/EXTERNAL_UI.md`,
   `../interaction/CONTROLLERS.md`, `../semantics/GEOMETRY_UTILITIES.md`,
   [GEOM_DESIGN.md](active/GEOM_DESIGN.md), and `../visuals/MESH.md`.
2. [ANNOTATION_MEASUREMENT_DESIGN.md](active/ANNOTATION_MEASUREMENT_DESIGN.md),
   [ANNOTATION_TEXT_SCALE_API.md](active/ANNOTATION_TEXT_SCALE_API.md),
   [COLORBAR_COLORMAP_DESIGN.md](active/COLORBAR_COLORMAP_DESIGN.md),
   [AXES_DOMAIN_DESIGN.md](active/AXES_DOMAIN_DESIGN.md),
   [TEXT_DESIGN.md](active/TEXT_DESIGN.md), and
   [SCREEN_SPACE_OVERLAY_LAYOUT.md](active/SCREEN_SPACE_OVERLAY_LAYOUT.md) promote into
   `../semantics/ANNOTATIONS.md`, `../semantics/SCALES.md`,
   `../semantics/LEGENDS_AND_COLORBARS.md`, `../semantics/AXES.md`, `../semantics/TEXT.md`,
   `../pipeline/FRAME_PLAN.md`, and `../api/API_SURFACE.md`.
3. [MESH_API_DESIGN.md](active/MESH_API_DESIGN.md),
   [MESH_SHADING_DESIGN.md](active/MESH_SHADING_DESIGN.md),
   [MATERIAL_LIGHTING_API.md](active/MATERIAL_LIGHTING_API.md),
   [PARTICLE_SYSTEM_DESIGN.md](active/PARTICLE_SYSTEM_DESIGN.md),
   [SCREEN_SPACE_EFFECTS_DESIGN.md](active/SCREEN_SPACE_EFFECTS_DESIGN.md),
   [TRANSPARENCY_WBOIT_DESIGN.md](promoted/TRANSPARENCY_WBOIT_DESIGN.md),
   [VOLUME_DESIGN.md](active/VOLUME_DESIGN.md), and
   [RAY_TRACING_FORWARD_COMPAT.md](active/RAY_TRACING_FORWARD_COMPAT.md) promote into
   `../visuals/MESH.md`,
   `../visuals/VOLUME.md`, `../semantics/LIGHTING.md`, `../semantics/TRANSPARENCY.md`,
   `../semantics/VISUAL_CONTRACT.md`, `../pipeline/FRAME_PLAN.md`, and
   `../core/RUNTIME_BOUNDARY.md`.
4. [RESOURCE_UPDATE_DESIGN.md](promoted/RESOURCE_UPDATE_DESIGN.md),
   [SCIENTIFIC_COORDINATE_NORMALIZATION.md](active/SCIENTIFIC_COORDINATE_NORMALIZATION.md),
   [ASSET_BOUNDARY_DESIGN.md](active/ASSET_BOUNDARY_DESIGN.md),
   [CAPABILITY_FALLBACK_DESIGN.md](active/CAPABILITY_FALLBACK_DESIGN.md),
   [SAMPLED_FIELD_API_DESIGN.md](promoted/SAMPLED_FIELD_API_DESIGN.md),
   [TRANSFORM_CONTROLLER_DESIGN.md](active/TRANSFORM_CONTROLLER_DESIGN.md), and
   [UI_BACKEND_INTEGRATION.md](promoted/UI_BACKEND_INTEGRATION.md) promote into
   `../pipeline/RESOURCE_MODEL.md`, `../pipeline/INVALIDATION_AND_CACHING.md`,
   `../pipeline/TRANSFORM_PIPELINE.md`, `../validation/ADAPTATION.md`,
   `../validation/VALIDATION.md`, `../visuals/IMAGE.md`, `../visuals/VOLUME.md`,
   `../api/API_SURFACE.md`, and `../integration/EXTERNAL_UI.md`.
5. [GEOM_DESIGN.md](active/GEOM_DESIGN.md) promotes into
   `../semantics/GEOMETRY_UTILITIES.md` and any future public `geom` module header plan.
6. [HIGH_PRIORITY_SPEC_DECISIONS.md](history/HIGH_PRIORITY_SPEC_DECISIONS.md) is a consolidation
   checkpoint. Do not use it as the final authority when a more focused proposal or specialized
   spec exists.


## Proposal Index

Active proposal-stage notes:

1. [Interaction and events](active/README.md#interaction-and-events)
2. [Text, axes, scales, annotations, and overlays](active/README.md#text-axes-scales-annotations-and-overlays)
3. [Visual families, material, lighting, and render modes](active/README.md#visual-families-material-lighting-and-render-modes)
4. [Resources, transforms, geometry, validation, and integration](active/README.md#resources-transforms-geometry-validation-and-integration)

Promoted rationale notes:

1. [promoted/README.md](promoted/README.md)

Future and exploratory notes:

1. [future/README.md](future/README.md)

Historical checkpoints:

1. [history/README.md](history/README.md)


## Immediate Use

For the next public API pass, read these first:

1. [../api/API_SURFACE.md](../api/API_SURFACE.md)
2. [../slices/README.md](../slices/README.md)
3. [../slices/TEXT_RENDERING_SLICE.md](../slices/TEXT_RENDERING_SLICE.md)
4. [../slices/ANNOTATION_LABEL_SLICE.md](../slices/ANNOTATION_LABEL_SLICE.md)
5. [../slices/COLORBAR_RENDERING_SLICE.md](../slices/COLORBAR_RENDERING_SLICE.md)
6. [INTERACTION_API_DESIGN.md](active/INTERACTION_API_DESIGN.md)
7. [ANNOTATION_TEXT_SCALE_API.md](active/ANNOTATION_TEXT_SCALE_API.md)
8. [PICKING_DESIGN.md](promoted/PICKING_DESIGN.md)
9. [PROBE_READOUT_DESIGN.md](promoted/PROBE_READOUT_DESIGN.md)
10. [COLORBAR_COLORMAP_DESIGN.md](active/COLORBAR_COLORMAP_DESIGN.md)
