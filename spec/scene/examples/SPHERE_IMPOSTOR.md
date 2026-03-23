# Example: Sphere Visual With Impostor-First Rendering

This example instantiates the first-class `sphere` family with its default impostor-first direction.


## Scene Setup

1. one scene,
2. one 3D panel,
3. one `sphere` visual,
4. one dataset of sphere centers and radii,
5. one camera controller.


## Family And Variant

Family:

1. `sphere`

Variant axes:

1. impostor-first rendering path,
2. optional lighting or quality mode,
3. possible future mesh-backed fallback or alternate variant.


## Resource Schema Instance

Default impostor-first resources:

1. source `ItemTable` in `DataSpace` containing sphere centers and radii,
2. derived normalized `ItemTable` in `VisualSpace`,
3. `StyleBlock` for appearance and quality controls.

Optional alternate variant:

1. `IndexedGeometry` when a mesh-backed path is selected.


## Transform Pipeline

1. centers and radii originate in `DataSpace`,
2. they are normalized into visual-ready sphere data,
3. camera/view/projection transforms act afterward,
4. switching between impostor and mesh-backed variants should not change the high-level family
   semantics.


## FramePlan Shape

Typical frame:

1. `UploadNode` when sphere data or sphere-style parameters change,
2. one `RenderNode` for the sphere contribution,
3. optional different render variant choice depending on capabilities.


## DRP2 Categories Implied

1. resource writes for sphere instance data,
2. render-pass lifecycle,
3. draw commands,
4. queue submission.


## Pressure On The Spec

This example checks that:

1. `sphere` remains a first-class family,
2. impostor-first is a variant choice, not a family split,
3. mesh-backed rendering can remain an internal or variant-level choice,
4. the transform pipeline stays stable across variants.
