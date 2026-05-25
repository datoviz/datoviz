# Scene Pipeline Specs

This directory contains resource, transform, invalidation, frame-plan, and lifecycle contracts.

Use these files when changing scene-side resource ownership, update granularity, data normalization,
frame planning, or runtime handoff.


## Files

1. [RESOURCE_MODEL.md](RESOURCE_MODEL.md): logical resources, ownership, and data ingestion policy.
2. [ATTRIBUTE_SOURCES.md](ATTRIBUTE_SOURCES.md): per-attribute data granularity and mutability hints.
3. [TRANSFORM_PIPELINE.md](TRANSFORM_PIPELINE.md): normalization, panel transforms, and CPU precision.
4. [INVALIDATION_AND_CACHING.md](INVALIDATION_AND_CACHING.md): dirty scopes, reuse, redraw, and uploads.
5. [FRAME_PLAN.md](FRAME_PLAN.md): canonical producer-side frame artifact.
6. [FRAME_PLAN_SERIALIZATION.md](FRAME_PLAN_SERIALIZATION.md): debug and fixture shape for frame plans.
7. [FRAME_LIFECYCLE.md](FRAME_LIFECYCLE.md): update, build, emit, and runtime handoff flow.
8. [DRAW_RESOURCE_VALIDATION_PLAN.md](DRAW_RESOURCE_VALIDATION_PLAN.md): staged hardening plan for
   validating draw counts against bound resource contents.


## Active Proposal Inputs

1. [../proposals/active/ASSET_BOUNDARY_DESIGN.md](../proposals/active/ASSET_BOUNDARY_DESIGN.md)
2. [../proposals/promoted/RESOURCE_UPDATE_DESIGN.md](../proposals/promoted/RESOURCE_UPDATE_DESIGN.md)
3. [../proposals/active/SCIENTIFIC_COORDINATE_NORMALIZATION.md](../proposals/active/SCIENTIFIC_COORDINATE_NORMALIZATION.md)
4. [../proposals/active/TRANSFORM_CONTROLLER_DESIGN.md](../proposals/active/TRANSFORM_CONTROLLER_DESIGN.md)
