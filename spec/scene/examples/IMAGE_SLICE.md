# Example: Image Slice Backed By Volumetric Sampling

This example instantiates the `slice` decision as an `image`-family mode.


## Owning Specs

This example should be read against:

1. `VISUAL_FAMILIES.md` for `slice` under `image`,
2. `../VISUAL_FAMILY_RULES.md` for the `image` family contract,
3. `../pipeline/RESOURCE_MODEL.md` for sampled and derived field handling,
4. `../pipeline/TRANSFORM_PIPELINE.md` for slice placement versus panel transform staging.


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

If a compute-assisted slice path exists:

1. the compute output should be treated as frame-local by default,
2. persistence across frames should require an explicitly declared derived cache,
3. the image-family semantics should remain the same whether the slice path is compute-assisted or
   not.


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
4. camera transforms stay panel-local,
5. optional compute assistance does not implicitly create authoritative persistent scene data.
