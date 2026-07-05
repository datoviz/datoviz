> **Execution Status**
> - **Status:** `PARTIALLY IMPLEMENTED SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-22`
> - **Purpose:** preserve remaining geometry-module decisions after the canonical geometry utility
>   spec absorbed the durable CPU-side rules.

# Geometry Design

## Decision Addressed

The v0.4 CPU geometry layer should be named `geom`, centered on `DvzGeometry`, and kept distinct
from the scene `mesh` visual family.

The remaining proposal-stage question is public API shape and staging, not whether procedural
geometry and triangulation belong in the v0.4 stack.


## Short Summary

`geom` provides CPU-side geometry containers, procedural generators, preprocessing operations,
planar triangulation helpers, and import paths. It emits ordinary geometry payloads that scene mesh
resources upload and render.

Because the v0.4 branch does not preserve v0.3 API or ABI compatibility, the `geom` migration
should be a direct replacement rather than a compatibility bridge. Do not keep `DvzShape` in the
public geometry API only to ease transition. If a short-lived adapter is useful while landing the
module, keep it private and remove it before treating `geom` as active.

This replaces the old central `DvzShape` framing with a cleaner split:

1. `DvzGeometry` is the generic render-oriented CPU container;
2. "shape" is a procedural-generation concept;
3. planar inputs such as polygons and PSLGs are separate input domains;
4. mesh visual concerns such as material, contour rendering, picking, and runtime resources stay
   outside the base geometry type.


## Chosen Direction

| Topic | Direction |
|---|---|
| Module/name | Use `geom` with public umbrella `datoviz/geom.h`. |
| Core type | Use `DvzGeometry`; remove `DvzShape` from the public canonical path instead of supporting both APIs. |
| Coordinates | Renderable geometry stores 3D positions; planar APIs remain genuinely 2D and embed into 3D output when needed. |
| Base payload | Positions, normals, colors, UVs, triangle indices, and later tangents. |
| Excluded base fields | Do not bake isoline, contour, left/right distance, or shader-specific staging fields into all geometry objects. |
| Generators | Cube, plane, surface grid, sphere, cylinder, cone, torus, arrow, gizmo axes, and classic polyhedra remain in scope. |
| Operations | Transform, merge, normal generation, bounds, indexing/unindexing, and later weld/tangent/edge helpers. |
| Triangulation | Keep polygon, structured surface grid, and PSLG/constrained triangulation as distinct input domains. |
| Dependencies | Earcut is suitable for simple polygons; constrained triangulation remains backend-agnostic at the public API. |


## Canonical Migration Links

The authoritative rules now live in:

1. [Geometry Utilities](../../semantics/GEOMETRY_UTILITIES.md) for generator, triangulation,
   curve, SDF/MSDF, dependency, and resource-mapping contracts;
2. [Polygon And PSLG API Design](POLYGON_PSLG_API_DESIGN.md) for the active polygon, polygon-set,
   mesh-upload-helper, and PSLG public API shape decisions;
3. [Visual Family: mesh](../../visuals/MESH.md) for how generated geometry is consumed by scene
   mesh resources and visuals;
4. [Transform Pipeline](../../pipeline/TRANSFORM_PIPELINE.md) for F64 computation, normalization,
   and upload-space behavior;
5. [Resource Model](../../pipeline/RESOURCE_MODEL.md) for the upload/resource boundary.

Do not restate generator tables or triangulation dependency policy here.


## Remaining Unresolved Points

1. Final public C function names and descriptor structs for generators and operations.
2. Whether the next generator slice should prioritize sphere or arrow/gizmo axes.
3. Exact partial-update policy for structured surface grids.
4. OBJ import scope and whether richer asset import belongs here or in an asset layer.
5. PSLG/constrained triangulation backend choice and build option policy.
6. Whether contour/isoline preparation helpers live in `geom` or a mesh-shading helper namespace.


## Landed First Slice

Implemented on 2026-05-21:

1. public `DvzGeometry` replaces the old public `DvzShape` scaffold in `datoviz/geom`;
2. `src/geom` is an active core object module linked into `libdatoviz` and core test runners;
3. `dvz_geometry()`, `dvz_geometry_reset()`, and `dvz_geometry_destroy()` own positions,
   normals, colors, UVs, and triangle indices through the shared allocator wrappers;
4. `dvz_geometry_bounds()` computes F64 bounds;
5. examples now bridge retained F64 CPU geometry to scene mesh upload paths with explicit local
   conversion helpers and generic visual data calls;
6. `dvz_geometry_cube()` and `dvz_geometry_plane()` generate indexed geometry with normals, UVs, and
   zero-initialized color descriptors defaulting to opaque white; cube descriptors also support
   six per-face colors for the duplicated cube-face layout;
7. focused `geom` tests cover allocation/reset, cube bounds/index validity, plane normals/bounds,
   surface grids, transforms, merge behavior, and core-runner wiring.


## Second Implementation Slice

Implemented on 2026-05-22:

1. `dvz_geometry_surface_grid()` generates indexed structured grids with row/column descriptors, optional
   height/color arrays, UVs, smooth normals, and row/column provenance on `DvzGeometry`;
2. `dvz_geometry_compute_normals()`, `dvz_geometry_transform()`, and `dvz_geometry_merge()` cover the
   first normal/transform/composition operation slice;
3. `DvzGeometry` data can be uploaded into a scene mesh visual by explicitly downcasting dense
   position/color/normal attributes and binding copied indices through `dvz_visual_set_index_data()`;
4. tests cover generated surface-grid counts, winding/index validity, invalid dimensions, transform
   behavior, merge index rebasing, and the direct scene mesh upload path.


## Deferred Follow-Ups

Additional v0.4 follow-up work on 2026-05-22 added:

1. `dvz_geometry_sphere()` as the first post-cube solid generator;
2. `dvz_geometry_surface_grid_update_heights()` for in-place structured-grid height updates;
3. retained mesh `texcoords` support, with example-local geometry upload code lowering
   `DvzGeometry` UVs into the mesh visual attribute set;
4. `examples/c/visuals/surface_grid.c` as a visible app-path pressure test for generated geometry.

Remaining deferred work:

1. Add gizmo-axis and classic polyhedron generators when a concrete example needs them.
2. Add richer structured-grid update/provenance helpers once surface examples require more
   update-efficient behavior.
3. Add CPU curve tessellation helpers, starting with quadratic/cubic Bezier and then Catmull-Rom
   and B-spline helpers, with output directly consumable by `dvz_path()`.
4. Extend OBJ import beyond the first `v`/`vn`/`f` slice only when examples need texcoords,
   materials, groups, or normalization policy.
5. Defer PLY, glTF, constrained triangulation, contour/isoline sidecars, and richer asset import
   until the current container and direct upload path have more example pressure.
