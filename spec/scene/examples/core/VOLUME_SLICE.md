# Example: Volume Slice

This example instantiates `volume.render_mode = slice`.


## Owning Specs

This example should be read against:

1. `../../semantics/VISUAL_FAMILIES.md` for the `volume` family,
2. `../../semantics/VISUAL_FAMILY_RULES.md` for the `volume` family contract,
3. `../../pipeline/RESOURCE_MODEL.md` for sampled and derived field handling,
4. `../../pipeline/TRANSFORM_PIPELINE.md` for slice placement versus panel transform staging.


## Scene Setup

1. one scene,
2. one 3D panel,
3. one `volume` visual in slice render mode,
4. one volumetric sampled field as source data,
5. one 3D camera controller.


## Family And Variant

Family:

1. `volume`

Variant axes:

1. `render_mode = slice`,
2. color interpretation mode such as direct or colormap,
3. optional border or placement mode.


## Resource Schema Instance

Scene-facing resources:

1. source volumetric `SampledField`,
2. volume slice placement and parameter data,
3. optional `DerivedField` if the slice mode materializes intermediate extracted data,
4. optional panel-local derived placement state.


## Transform Pipeline

1. the volumetric sampled content exists in domain or voxel-like `DataSpace`,
2. slice placement is defined as a volume render-mode parameter,
3. the resulting slice geometry belongs to `VisualSpace`,
4. camera/view transforms act afterward as panel-local viewing state.

The important rule is:

1. the scene spec talks about volume slicing over volumetric sampling,
2. it does not elevate backend texture dimensionality into the family taxonomy.


## FramePlan Shape

Typical frame:

1. optional `UploadNode` when the volumetric field or slice parameters change,
2. optional `ComputeNode` only if a future slice mode needs it,
3. one `RenderNode` for the visible volume-slice contribution.

If a compute-assisted slice path exists:

1. the compute output should be treated as frame-local by default,
2. persistence across frames should require an explicitly declared derived cache,
3. the volume-slice semantics should remain the same whether the slice path is compute-assisted or
   not.


## DRP2 Categories Implied

1. resource writes for sampled field or derived slice resources,
2. render-pass lifecycle,
3. draw commands,
4. queue submission.


## Pressure On The Spec

This example checks that:

1. `slice` is not promoted to a separate family,
2. `volume.render_mode = slice` hosts slicing semantics cleanly,
3. volumetric sampling source and slice placement remain separate concepts,
4. camera transforms stay panel-local,
5. optional compute assistance does not implicitly create authoritative persistent scene data.
