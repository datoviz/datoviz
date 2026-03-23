# Example: Image Slice Backed By Volumetric Sampling

This example instantiates the `slice` decision as an `image`-family mode.


## Scene Setup

1. one scene,
2. one 3D panel,
3. one `image` visual in slice-like mode,
4. one volumetric sampled field as source data,
5. one 3D camera controller.


## Family And Variant

Family:

1. `image`

Variant axes:

1. slice-like mode backed by volumetric sampling,
2. color interpretation mode such as direct or colormap,
3. optional border or placement mode.


## Resource Schema Instance

Scene-facing resources:

1. source volumetric `SampledField`,
2. image-family placement and style `StyleBlock`,
3. optional `DerivedField` if the slice mode materializes intermediate extracted data,
4. optional panel-local derived placement state.


## Transform Pipeline

1. the volumetric sampled content exists in domain or voxel-like `DataSpace`,
2. slice placement is defined as an image-family semantic mode,
3. the resulting placed image belongs to `VisualSpace`,
4. camera/view transforms act afterward as panel-local viewing state.

The important rule is:

1. the scene spec talks about image placement over volumetric sampling,
2. it does not elevate backend texture dimensionality into the family taxonomy.


## FramePlan Shape

Typical frame:

1. optional `UploadNode` when the volumetric field or slice parameters change,
2. optional `ComputeNode` only if a future slice mode needs it,
3. one `RenderNode` for the visible image-family contribution.


## DRP2 Categories Implied

1. resource writes for sampled field or derived slice resources,
2. render-pass lifecycle,
3. draw commands,
4. queue submission.


## Pressure On The Spec

This example checks that:

1. `slice` is not promoted to a separate family,
2. `image` can host slice-like semantics cleanly,
3. volumetric sampling source and image-family placement remain separate concepts,
4. camera transforms stay panel-local.
