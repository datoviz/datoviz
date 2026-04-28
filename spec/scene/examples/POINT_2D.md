# Example: Point In One 2D Panel

This example instantiates the simplest meaningful `point` scene path.


## Owning Specs

This example should be read against:

1. `../VISUAL_FAMILY_RULES.md` for the `point` family contract,
2. `../pipeline/RESOURCE_MODEL.md` for `ItemTable` and `StyleBlock`,
3. `../pipeline/TRANSFORM_PIPELINE.md` for normalization and panel transform staging,
4. `../pipeline/FRAME_PLAN.md` for the scene-level `FramePlan` shape.


## Scene Setup

1. one scene,
2. one 2D panel,
3. one `point` visual,
4. one shared numeric dataset in data coordinates,
5. one panel panzoom controller.


## Family And Variant

Family:

1. `point`

Variant axes:

1. standard point render path,
2. no picking,
3. no compute assistance.


## Resource Schema Instance

Scene-facing resources:

1. source `ItemTable` in `DataSpace` with per-item `x`, `y`, size, and color inputs,
2. derived normalized `ItemTable` in `VisualSpace`,
3. optional `StyleBlock` for family-wide defaults.


## Transform Pipeline

1. source point positions exist in `DataSpace`,
2. scene normalization maps them into visual-ready 2D point positions in `VisualSpace`,
3. normalized point data is uploaded once or when data changes,
4. panel panzoom applies afterward as the live viewing transform,
5. final clip/NDC coordinates are produced from panel-local view state.

The important property is:

1. panzoom changes should not require renormalizing the point resource.


## FramePlan Shape

Typical frame with no data changes:

1. no `UploadNode` for point data,
2. one `RenderNode` for the panel color pass.

Typical frame after point data changes:

1. one `UploadNode` for the normalized point resource,
2. one `RenderNode` for the panel color pass.


## DRP2 Categories Implied

1. resource write for dirty point data,
2. render-pass lifecycle,
3. draw commands,
4. queue submission.


## Pressure On The Spec

This example checks that:

1. `point` remains distinct from `pixel`,
2. data normalization is separate from panzoom,
3. one family can reuse normalized data across many panzoom updates,
4. the minimal `FramePlan` stays simple,
5. uploads appear inside the plan rather than through a separate execution-time path.
