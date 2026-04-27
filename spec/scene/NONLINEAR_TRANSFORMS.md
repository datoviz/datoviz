# Non-Linear Coordinate Transforms

This document defines how the scene layer handles non-linear coordinate projections
such as geographic projections and polar coordinates.


## Scope

The `TRANSFORM_PIPELINE.md` covers the affine normalization and panel-transform stages.
This document covers transforms that are non-linear in data space — projections that
cannot be expressed as a matrix multiply.

Examples:
- geographic projections: Mercator, equirectangular, orthographic globe
- polar / radial coordinates: (r, θ) → (x, y)
- logarithmic spatial axes (distinct from log scale on a color attribute)
- custom user-defined projections


## v0.4 Path: CPU-Side Projection

In v0.4 non-linear projections are applied on the CPU before upload.

The user projects their data positions into the scene's Cartesian data space
before passing them to the visual:

```text
// user-side, pseudocode
xy = mercator_project(lon_lat)       // CPU, F64
dvz_visual_set_positions(visual, xy) // upload projected positions
```

The scene then applies its normal affine normalization and panel transform pipeline.
No scene-layer API is needed for v0.4 projection support.

This approach is simple, has no shader complexity, and works correctly with the
existing F64 precision policy (projection in F64, downcast at UploadNode).


## v0.4+ Path: GPU Projection Compute Pre-Pass

For large datasets where CPU re-projection on every data update is expensive,
or where the projection must respond to runtime parameters (e.g., globe rotation),
the scene will support a GPU projection compute pre-pass.


### Model

The panel declares a non-linear projection:

```text
dvz_panel_set_projection(panel, DVZ_PROJ_MERCATOR, &proj_params)
```

When a non-affine projection is set on a panel, the scene:

1. allocates a **persistent derived position buffer** for each visual in the panel,
2. inserts a `ComputeNode` into the `FramePlan` before the render pass,
3. the `ComputeNode` reads the raw position buffer and writes projected positions
   into the derived buffer,
4. the render pass reads the derived buffer instead of the raw position buffer.

The affine pan/zoom panel transform uniform continues to operate on the projected
positions in the render shader, as usual.


### When The Compute Pre-Pass Runs

The compute pre-pass is a **persistent derived resource**.
It runs only when:

1. the source position data is marked dirty (new data upload), or
2. the projection parameters are marked dirty (e.g., `dvz_panel_set_projection` called
   with new parameters).

Pan and zoom updates never trigger the compute pre-pass.
They update only the panel transform uniform, as for affine projections.

This means re-projection cost is paid only when data or projection parameters actually
change — not every frame.


### FramePlan Structure

```text
FramePlan (panel with non-linear projection, positions dirty):
  UploadNode    — raw position data
  ComputeNode   — project raw positions → derived position buffer
  RenderNode    — render using derived position buffer + affine panel transform

FramePlan (panel with non-linear projection, pan/zoom only):
  RenderNode    — render using cached derived position buffer + updated panel transform
```

The `ComputeNode` is omitted when neither positions nor projection parameters are dirty.


### Built-In Projections

The following projections will be supported as built-in compute shaders:

| Name | Description |
|---|---|
| `DVZ_PROJ_MERCATOR` | web Mercator (EPSG:3857) |
| `DVZ_PROJ_EQUIRECTANGULAR` | equirectangular (plate carrée) |
| `DVZ_PROJ_ORTHOGRAPHIC` | orthographic globe |
| `DVZ_PROJ_POLAR` | polar (r, θ) → (x, y) |

Additional built-in projections may be added without breaking the contract.


### Custom GPU Projections

A user-supplied compute shader can be registered as a named projection:

```text
dvz_projection_register(scene, "my_proj", compute_shader_src, &param_layout)
```

The compute shader receives the raw position buffer, the projection parameter block,
and writes to the derived position buffer.
The scene inserts it into the `FramePlan` using the same mechanism as built-in projections.

This uses the same compute shader registration infrastructure as custom visual families
(see `CUSTOM_VISUALS.md`).


### Interaction With Picking

The picking compute shader reads the same derived position buffer used by the render pass.
Pick coordinates are screen-space and are already consistent with the projected positions.
No special handling is needed for non-linear projections in the picking path.


### Interaction With Axes

Axes draw ticks and labels in data space.
For non-linear projections, axis tick positions must be projected before display.
The axes layer is responsible for generating tick positions in data space and passing
them through the same projection as the visual data.

The interaction between non-linear projections and axes is deferred to the axes spec.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `TRANSFORM_PIPELINE.md` | this document extends the pipeline for non-affine stages |
| `FRAME_PLAN_IR.md` | projection ComputeNode in FramePlan |
| `RESOURCE_MODEL.md` | derived position buffer as persistent compute-derived resource |
| `INVALIDATION_AND_CACHING.md` | projection dirty scope: source positions or parameters |
| `CUSTOM_VISUALS.md` | custom compute shader registration |
| `AXES.md` | tick projection for non-linear panels |


## Deferred Questions

1. exact parameter layout for each built-in projection,
2. interaction between non-linear projections and axes tick generation,
3. whether per-visual projection overrides (different projection per visual in the same panel)
   are needed.
