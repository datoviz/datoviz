# Field Visualization Roadmap

> **Status:** exploratory roadmap for future field consumers.
> **Primary gap:** `SampledField`, `image`, and `volume` cover regular scalar/RGBA data, but
> vector fields, tensor fields, categorical label volumes, and sparse/bricked fields need clearer
> semantics before becoming public APIs.


## Summary

Datoviz already has the right foundation for regular sampled data:

- `SampledField` as scene-owned regular 2D/3D data;
- `image` and `volume` as consumers;
- categorical scales;
- image probing and volume slice probing;
- deferred vector and label field mentions.

This note records future field directions without duplicating the active `SampledField` contract.


## Vector Fields

### Simple Examples

- wind vectors over a map;
- CFD velocity field with vorticity image;
- ocean currents;
- magnetic field lines;
- gradient field over an image;
- diffusion principal directions;
- plasma or tokamak field lines.


### Data Model

Vector fields can be regular sampled fields or unstructured-grid fields.

Regular case:

```text
DvzSampledField
  dimension = 2D or 3D
  role = vector
  components = 2 or 3
  format = float or normalized
  origin/spacing/axis metadata
```

Unstructured case belongs to [`UNSTRUCTURED_GRID_DESIGN.md`](../future/UNSTRUCTURED_GRID_DESIGN.md), with
field location set to point, cell, or face.


### Render Views

Recommended views:

- arrow/quiver glyphs;
- line-integral convolution or texture flow for 2D fields;
- streamlines and pathlines;
- particles/tracers advected through the field;
- slice-plane arrows through a 3D field;
- magnitude image/volume using a scalar derived field;
- vector probe readout at cursor or selected position.


### Compute Opportunities

- streamline integration;
- seed generation and thinning;
- particle advection;
- vorticity/divergence/curl derived fields;
- magnitude and normalization;
- vector-field resampling between grids.


## Tensor Fields

### Simple Examples

- diffusion MRI tensors;
- FEM stress/strain tensors;
- covariance ellipses or ellipsoids;
- material anisotropy;
- image structure tensors;
- uncertainty covariance at sample points.


### Data Model

Tensor fields need rank, symmetry, dimensionality, and location metadata.

Suggested roles:

```text
tensor_2x2_symmetric
tensor_3x3_symmetric
tensor_2x2_full
tensor_3x3_full
```

Common derived quantities:

- eigenvalues;
- eigenvectors;
- trace;
- determinant;
- anisotropy;
- principal direction;
- von Mises stress for stress tensors.


### Render Views

- ellipses or ellipsoids from eigen decomposition;
- oriented glyphs or superquadrics;
- principal direction segments;
- scalar images/volumes from derived invariants;
- tensor-probe readouts;
- slices through 3D tensor fields.

The first implementation should probably be a tensor-glyph helper over existing instanced mesh,
sphere, segment, or marker paths, not a large standalone visual family.


### Compute Opportunities

- eigen decomposition;
- derived scalar invariant computation;
- glyph instance generation;
- tensor downsampling or representative selection;
- field-line integration along principal directions.


## Categorical Label Volumes

### Simple Examples

- brain atlas segmentation;
- cell/nucleus segmentation volume;
- organ label volume;
- material phase labels;
- geological facies model.


### Data Model

A label field is an integer `SampledField` plus a label table.

```text
field: integer 2D or 3D sampled field
label table:
  label_id
  name
  color
  opacity
  visible
  group
```

Label fields should use nearest sampling by default. Linear interpolation of label ids is not
semantically valid unless a visual explicitly requests a probabilistic or boundary-smoothed mode.


### Render Views

- 2D label image overlay;
- 3D label volume slice;
- categorical direct-volume rendering;
- selected-label boundary overlay;
- label outline/contour on slices;
- per-label opacity and visibility;
- label legend and colorbar-like categorical display.


### Picking And Probing

Expected label-volume probe payload:

```text
field id
voxel index
uvw coordinate
physical coordinate, if metadata exists
label id
label name
label color
sampled raw value
```

DVR label picking is harder than slice probing and can remain deferred. Slice and explicit voxel
probe paths are the first target.


## Sparse Voxel And Bricked Fields

### Simple Examples

- occupancy grids in robotics;
- sparse microscopy volumes;
- adaptive mesh refinement scalar fields;
- sparse material phases;
- chunked label volumes;
- large empty-space medical or simulation volumes.


### Data Model

Sparse fields should not be forced into always-resident dense textures.

Possible resource classes:

```text
SparseField
  voxel coordinates or page table
  values
  optional brick id per voxel

BrickedField
  brick dimensions
  brick coordinates
  resident brick table
  per-brick bounding boxes
  per-brick min/max metadata
  dense payload per resident brick
```

For many applications, Python can own the high-level chunk store and Datoviz C can own the resident
GPU resources and page metadata.


### Render Views

- instanced visible voxel cubes;
- sparse point/splat view;
- bricked volume raymarch;
- chunk debug overlay;
- occupancy/label slice view;
- empty-space-skipping DVR.


### Runtime Requirements

- page/chunk residency tracking;
- partial brick uploads;
- brick visibility/culling metadata;
- stable field identity while resident chunks change;
- fallback rendering while some chunks are missing;
- upload telemetry and memory budget reporting.

These requirements overlap with [`OUT_OF_CORE_PROGRESSIVE_DESIGN.md`](../future/OUT_OF_CORE_PROGRESSIVE_DESIGN.md).


## Relationship To Existing Specs

- [`SAMPLED_FIELD_API_DESIGN.md`](../promoted/SAMPLED_FIELD_API_DESIGN.md) owns regular dense field rationale.
- [`../../visuals/IMAGE.md`](../../visuals/IMAGE.md) owns 2D sampled rendering.
- [`../../visuals/VOLUME.md`](../../visuals/VOLUME.md) owns dense 3D scalar/RGBA volume rendering.
- [`../../semantics/SCALES.md`](../../semantics/SCALES.md) owns continuous and categorical scales.
- [`UNSTRUCTURED_GRID_DESIGN.md`](../future/UNSTRUCTURED_GRID_DESIGN.md) owns fields attached to cell meshes.


## Example Plans

Useful future examples:

- vector field: wind or CFD arrows + streamlines + probes;
- tensor field: diffusion tensor or FEM stress ellipsoid glyphs;
- label volume: atlas/segmentation volume with label table and slice probing;
- sparse voxel: occupancy or AMR field with chunk debug overlay.


## Open Questions

- Should vector/tensor roles be part of `SampledField`, separate `Field` resources, or only
  metadata conventions?
- Should categorical labels use `Scale` directly or a dedicated label table object?
- Should sparse/bricked fields be a new resource type or a capability tier of `SampledField`?
- Which tensor formats and derived invariants are common enough to standardize?
- Which field operations should Datoviz compute directly versus accept as precomputed arrays?
