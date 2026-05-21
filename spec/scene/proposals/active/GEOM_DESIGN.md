> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-20`
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
2. [Visual Family: mesh](../../visuals/MESH.md) for how generated geometry is consumed by scene
   mesh resources and visuals;
3. [Transform Pipeline](../../pipeline/TRANSFORM_PIPELINE.md) for F64 computation, normalization,
   and upload-space behavior;
4. [Resource Model](../../pipeline/RESOURCE_MODEL.md) for the upload/resource boundary.

Do not restate generator tables or triangulation dependency policy here.


## Remaining Unresolved Points

1. Final public C function names and descriptor structs for generators and operations.
2. Exact memory ownership and allocation helpers for `DvzGeometry` buffers.
3. Which first-slice generators land with tests beyond cube and surface grid.
4. OBJ import scope and whether richer asset import belongs here or in an asset layer.
5. How much structured-grid provenance is stored in `DvzGeometry` versus a sidecar descriptor.
6. PSLG/constrained triangulation backend choice and build option policy.
7. Whether contour/isoline preparation helpers live in `geom` or a mesh-shading helper namespace.


## First Implementation Slice

1. Replace the current public `datoviz/geom` shape scaffold with a lean `DvzGeometry` declaration
   and descriptors for the first generators.
2. Add a `src/geom` object module using the normal allocator wrappers and explicit destroy/reset
   helpers.
3. Land a small generator set first: cube, plane, and structured surface grid, with positions,
   normals, colors, UVs, and `uint32` triangle indices.
4. Add geometry operations needed by those generators: bounds, normal generation, transform, and
   merge.
5. Add tests that validate ownership, counts, index ranges, generated bounds, normal direction, and
   scene mesh upload compatibility.
6. Defer OBJ import, constrained triangulation, contour/isoline sidecars, and richer asset import
   until the core container and upload path are stable.
