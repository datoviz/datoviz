# Unstructured Grid And Cell Mesh Design

> **Status:** exploratory future scene-resource proposal.
> **Primary gap:** `mesh` covers triangle surfaces and `volume` covers regular voxel fields, but
> finite-element and finite-volume datasets use volumetric cells with separate topology and field
> semantics.


## Summary

Unstructured grids should be represented as semantic scene resources that own points, cells, cell
types, and fields. Rendering should use derived views: boundary surface meshes, cut-plane meshes,
cell-edge overlays, vector/tensor glyphs, isosurfaces, and probes.

This proposal complements [`../visuals/MESH.md`](../visuals/MESH.md), not replaces it. The existing
`mesh` visual remains the right representation for rendered triangle surfaces.


## Simple Examples

- tetrahedral FEM stress field in a mechanical part;
- hexahedral CFD mesh around an airfoil;
- finite-volume reservoir simulation grid;
- wedge/pyramid hybrid mesh in a geophysical model;
- unstructured ocean or atmospheric grid;
- medical simulation mesh with displacement and scalar fields.


## Core Data Model

Candidate resource:

```text
DvzUnstructuredGrid
  point_count
  points[point_count]             vec3
  cell_count
  cell_type[cell_count]           tet, hex, wedge, pyramid, triangle, quad, polyhedron
  cell_offset[cell_count + 1]      offsets into connectivity
  connectivity[...]               point indices
  optional face topology cache
  optional boundary face cache
  point fields                    scalar, vector, tensor, category
  cell fields                     scalar, vector, tensor, category
  time steps                      optional
```

Supported first cell types should be deliberately small:

1. tetrahedron;
2. hexahedron;
3. wedge/prism;
4. pyramid;
5. triangle/quad boundary faces.

Mixed cells should be allowed by the semantic model, even if the first runtime supports only a
subset.


## Rendering Views

Recommended views:

| View | Description | Likely lowering |
|---|---|---|
| boundary surface | exterior faces colored by point or cell field | `mesh` |
| cell wireframe | all or selected cell edges | `segment`/`path` overlay |
| cut plane | intersection polygons through cells | derived `mesh` |
| isosurface | scalar-field surface inside cells | derived `mesh` |
| point glyphs | nodal vector/tensor markers | `marker`, `segment`, `mesh`, `sphere` |
| cell glyphs | cell-centered arrows/ellipsoids | glyph or instanced mesh |
| deformation | display `points + scale * displacement` | derived point buffer |


## Field Semantics

Fields need explicit association:

```text
location = point | cell | face | integration_point
kind     = scalar | vector | tensor | category
time     = static | time-varying
units    = optional
```

Examples:

- displacement: point vector field;
- von Mises stress: point or cell scalar field;
- material id: cell category field;
- velocity: cell or point vector field;
- stress tensor: cell tensor field.


## Interpolation And Sampling

Unstructured-grid probes require more than texture sampling.

Useful query payload:

```text
grid id
cell id
cell type
local coordinates
barycentric or reference-cell coordinates
interpolated field values
nearest point id, optional
physical coordinate
```

The first implementation can support CPU-side probe and cut-plane generation. GPU-accelerated
search/interpolation can be added later.


## Picking And Selection

Selection should support:

- surface face selection;
- owning cell/element selection;
- point/node selection;
- material region selection;
- selected cell edge/boundary highlight;
- linked diagnostics panels.

Picking a rendered boundary face should resolve back to the source cell id when that mapping exists.
This is the key difference from ordinary surface mesh face picking.


## Compute Opportunities

Potential future compute passes:

- boundary extraction for dynamic topology;
- cut-plane extraction;
- isosurface extraction;
- deformation of point buffers;
- scalar min/max reductions;
- vector/tensor glyph generation;
- cell visibility or clipping classification;
- element-quality diagnostics.

These should produce derived resources consumed by existing visuals.


## Out-Of-Core And Progressive Notes

Large simulation meshes may not fit as one always-resident resource.

Future support may need:

- partition or block ids;
- per-block bounding boxes;
- visibility-driven block loading;
- field-specific residency;
- progressive refinement;
- low-resolution boundary mesh fallback;
- topology and field arrays loaded independently.

This overlaps with [`OUT_OF_CORE_PROGRESSIVE_DESIGN.md`](OUT_OF_CORE_PROGRESSIVE_DESIGN.md).


## Example Plan

Add a future example such as:

```text
spec/scene/examples/engineering/UNSTRUCTURED_CELL_MESH_VIEWER.md
```

First useful slice:

- deterministic tetra/hex cantilever or flow-domain mesh;
- boundary surface colored by scalar field;
- cell-edge overlay;
- cut plane with scalar interpolation;
- cell picking and readout;
- deformation slider for FEM-like data.


## Open Questions

- Should this be named `UnstructuredGrid`, `CellMesh`, or `CellComplex`?
- Which cell types are mandatory in the first implementation?
- Should field arrays be stored in one resource object or separate named resources attached to the
  grid?
- Should cut/isurface generation be CPU-first or compute-first?
- How should VTK/meshio conversion metadata be represented without putting file readers in C?
