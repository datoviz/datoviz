> **Execution Status**
> - **Status:** `FUTURE SCENE-RESOURCE PROPOSAL`
> - **Updated on:** `2026-05-18`
> - **Purpose:** preserve exploratory direction for unstructured-grid resources, volumetric cell
>   topology, attached fields, derived surface views, and probes.
> - **Primary gap:** `mesh` covers triangle surfaces and `volume` covers regular voxel fields, but
>   finite-element and finite-volume datasets use volumetric cells with separate topology and field
>   semantics.

# Unstructured Grid And Cell Mesh Design


## Summary

Unstructured grids should be represented as semantic scene resources that own points, cells, cell
types, and fields. Rendering should use derived views: boundary surface meshes, cut-plane meshes,
cell-edge overlays, vector/tensor glyphs, isosurfaces, and probes.

This proposal complements [`../../visuals/MESH.md`](../../visuals/MESH.md), not replaces it. The existing
`mesh` visual remains the right representation for rendered triangle surfaces.

Use `UnstructuredGrid` as the preferred public/conceptual name. It is the standard
scientific-visualization term used by tools such as VTK, ParaView, VisIt, and meshio, and it clearly
distinguishes volumetric cell topology from the surface `mesh` visual family. Terms such as
`cell mesh` and `cell topology` remain useful internally, but `CellMesh` is too easily confused with
a visible surface mesh and `CellComplex` is less familiar to the target user base.


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

Recommended first runtime subset:

1. `tet4`;
2. `hex8`;
3. `tri3` boundary faces;
4. `quad4` boundary faces.

Reserve semantic names for `wedge6` and `pyramid5`, but do not require them in the first
implementation. High-order elements, polygonal cells, and general polyhedra should remain explicit
follow-up work. This keeps the first slice useful for common FEM and CFD datasets while avoiding
premature interpolation and face-extraction complexity.


## Topology And Field Ownership

The grid resource should own topology and point coordinates. Fields should be separate named
resources attached to the grid, not embedded as one large opaque payload.

Preferred split:

```text
DvzUnstructuredGrid
  points
  cells
  cell types
  connectivity
  optional derived topology caches

DvzGridField
  parent grid
  name
  location = point | cell | face | integration_point
  kind = scalar | vector | tensor | category
  component count and format
  static or time-varying payload
```

Reasons:

1. topology changes rarely, while fields may change often;
2. one grid commonly has many fields;
3. time-varying fields can be streamed or swapped independently;
4. field residency and progressive loading can be managed separately from topology;
5. users can switch active scalar/vector/tensor fields without replacing the grid;
6. partial updates are cleaner when topology, geometry, and field arrays have separate dirty state.

Conceptual API sketch:

```c
DvzUnstructuredGrid* grid = dvz_unstructured_grid(scene, &grid_desc);
DvzGridField* stress = dvz_grid_field(grid, "von_mises", &stress_desc);
DvzGridField* disp = dvz_grid_field(grid, "displacement", &disp_desc);
```

The API names are provisional. The ownership split is the important rule.


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

For deformation workflows, treat displacement as an attached point vector field, not as a topology
replacement. Display coordinates are derived from:

```text
display_position = rest_position + deformation_scale * displacement
```

Changing load step, time step, or deformation scale should update derived positions or parameters
without rebuilding cell connectivity.


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

Every derived render view that originates from grid topology should carry enough source-id metadata
to resolve interaction back to the unstructured grid:

| Derived view | Required source mapping |
|---|---|
| boundary triangle/quad | source cell id and source face id when available |
| cut-plane polygon/triangle | source cell id and local/reference coordinates |
| isosurface triangle | source cell id and field value/level |
| cell-edge segment | source cell id and optional source edge id |
| glyph | source point id or source cell id |

Without this mapping, the user can only pick derived triangles or segments, not scientific elements,
materials, or field values.


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

Start CPU-first or preprocessing-first for boundary extraction, cut planes, and isosurfaces. The
first goal should be semantic correctness and stable source-id mapping. GPU compute can replace the
producer later if it emits the same derived resources and source-id tables.

Recommended progression:

1. prepared or CPU-generated boundary surface;
2. CPU cut-plane extraction for correctness and interaction;
3. CPU or preprocessing isosurface extraction when needed;
4. GPU compute acceleration after DRP2 compute-to-render paths are mature.

This provides a reference path for WebGPU and for non-compute fallback modes.


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


## Prepared Cache And Metadata

Do not put VTK, VTU, Exodus, XDMF, CGNS, or solver-specific readers in C initially. Python, meshio,
or application code should prepare compact Datoviz-ready arrays.

Suggested prepared cache layout:

```text
metadata.json
points_f32.bin
cell_types_u8.bin
cell_offsets_u32.bin
connectivity_u32.bin
fields/
  von_mises_cell_f32.bin
  displacement_point_vec3_f32.bin
  material_cell_u32.bin
derived/
  boundary_indices_u32.bin
  boundary_source_cell_u32.bin
  boundary_source_face_u8.bin
```

Minimum metadata useful to C:

```text
coordinate_system
units
point_count
cell_count
cell_types
field names
field locations
field kinds
field units
time_step_count
value ranges
derived source-id maps
```

Optional provenance metadata may keep the original solver/file context:

```text
original_format
source_file
source_field_name
component_names
material_names
boundary_condition_names
block_names
partition_ids
```

Datoviz C should understand the metadata needed for rendering, validation, picking, units, labels,
and diagnostics. It should not need to understand the full original solver file model.


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

Minimum implementation target:

1. static `tet4`/`hex8` topology;
2. point positions;
3. one cell scalar field;
4. one point vector displacement field;
5. prepared or CPU-derived boundary surface;
6. boundary `source_cell_id` mapping;
7. mesh view colored by scalar;
8. wireframe/cell-edge overlay;
9. cell picking resolved from boundary face;
10. deformation-scale slider.


## Open Questions

- Should the eventual public API expose `DvzGridField` as a first-class handle, or keep fields
  attached through generic named resources?
- How much derived-topology caching should live in C versus prepared cache files?
- What is the smallest useful cut-plane API that preserves source-cell mapping?
- How should high-order elements be represented later without complicating the first linear-cell
  contract?
- Should block/partition metadata be part of the base grid resource or only part of out-of-core
  extensions?
