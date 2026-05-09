> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 geometry module so procedural mesh generation,
>   triangulation, and CPU-side mesh preprocessing have a stable home before broader `mesh`
>   implementation work lands.

# Geometry Design

This note records the intended role, naming, scope, and API shape of the v0.4 geometry layer.


## Objective

Bring back the useful CPU-side geometry capabilities that existed around `DvzShape` in v0.3, but
with cleaner boundaries and a more durable API for v0.4.

The geometry layer should provide:

1. a generic CPU-side geometry container,
2. procedural generation for common shapes and solids,
3. mesh preprocessing operations,
4. planar triangulation helpers,
5. import paths for common geometry sources,
6. a clean handoff to the scene `mesh` visual family.


## Naming

The chosen module direction is:

1. module name: `geom`
2. public umbrella header: `datoviz/geom.h`
3. current supporting headers:
   [include/datoviz/geom.h](../../../include/datoviz/geom.h),
   [include/datoviz/geom/types.h](../../../include/datoviz/geom/types.h),
   [include/datoviz/geom/enums.h](../../../include/datoviz/geom/enums.h)

Preferred core type name:

1. `DvzGeometry`

Why:

1. it matches the `geom` module name cleanly,
2. it avoids overloading `mesh`, which is also a visual family,
3. it is broader and more durable than reviving `DvzShape` as the central type.

`shape` should remain a concept for procedural generators, not the core container name.


## v0.3 Assessment

v0.3 had the right overall idea:

1. a CPU-side geometry object,
2. shape generators for common solids and surfaces,
3. geometry operations such as merge, transform, and normal generation,
4. simple planar triangulation via `earcut`,
5. import of OBJ geometry.

What v0.3 mixed together too aggressively:

1. generic mesh data,
2. procedural-shape generation,
3. mesh-visual-specific contour/isoline payloads,
4. triangulation-backend details,
5. contour rendering assumptions tied to indexing style.

The v0.4 design should keep the useful capabilities while separating those concerns.


## Core Container

The geometry layer should center on a generic CPU-side renderable geometry container.

Recommended core fields:

1. vertex count
2. index count
3. `vec3` positions
4. `vec3` normals
5. packed vertex colors
6. UV coordinates
7. triangle indices
8. optional later fields such as tangents

The core geometry container should not include mesh-visual-specific contour/isoline staging data.

These v0.3-style fields should not live in the base geometry type:

1. `isoline`
2. `d_left`
3. `d_right`
4. `contour`

If later mesh shader families need those payloads, they should be produced by dedicated geometry
preparation helpers rather than baked into all geometry objects.


## 2D and 3D

The geometry layer should support both 2D and 3D, but not by pretending every operation is
dimension-agnostic.

There are three useful categories:

1. native 2D geometry,
2. native 3D geometry,
3. planar 2D input embedded into a 3D renderable mesh.

Recommended rule:

1. keep `DvzGeometry` render-oriented and 3D-oriented,
2. represent renderable positions as `vec3`,
3. keep planar input APIs genuinely 2D where that is the natural domain.

This means:

1. 2D polygon/PSLG inputs should use 2D coordinates,
2. triangulation should emit 3D geometry on a plane,
3. the downstream mesh visual should consume `DvzGeometry` without caring whether the source began
   as 2D or 3D.

Phase 1 planar embedding convention:

1. 2D inputs live on the XY plane,
2. emitted geometry uses `z = 0`,
3. default normals point along `+Z` unless intentionally flipped.

Later, arbitrary planar embedding could be added, but it is not required now.


## Procedural Shapes and Solids

The geometry module should include procedural generation for common reusable shapes.

Immediate useful families:

1. cube
2. plane
3. surface grid
4. sphere
5. cylinder
6. cone
7. torus
8. arrow

Classic solids worth keeping in scope:

1. tetrahedron
2. cube / hexahedron
3. octahedron
4. dodecahedron
5. icosahedron

Those are standard and useful. They should live naturally in the `geom` module.

Pragmatic note:

1. a colored cube with duplicated face vertices is immediately useful for the first live mesh
   example,
2. flat-shaded and smooth-shaded generation should both be supported explicitly,
3. indexed and expanded/non-indexed output modes should be deliberate choices, not accidents.


## Mesh Operations

The geometry module should own generic CPU-side mesh preprocessing operations.

Useful baseline operations:

1. transform
2. translate
3. scale
4. rotate
5. merge
6. compute normals
7. compute bounds
8. index / unindex
9. weld / unweld or equivalent later

Potential later additions:

1. tangent generation
2. edge extraction
3. adjacency helpers
4. contour/isoline preparation helpers
5. subdivision or decimation if a real use case appears


## Triangulation

The geometry module should support triangulation, but with a cleaner structure than v0.3.

There are three distinct planar-mesh paths:

1. structured surface/grid triangulation,
2. polygon triangulation,
3. PSLG / constrained triangulation.

Those should be treated as different input domains, even if they all eventually produce triangle
meshes.


## Earcut

The v0.3 code already had a lightweight `earcut` wrapper in
[v0.3/src/scene/geometry.cpp](../../../v0.3/src/scene/geometry.cpp).

That is still useful in v0.4.

Recommended role for `earcut`:

1. simple polygons,
2. polygons with holes,
3. lightweight default planar triangulation backend,
4. deterministic CPU preprocessing with minimal baggage.

Improvements over v0.3:

1. support outer ring plus hole rings, not only one contour,
2. do not expose `earcut` as the geometry API itself,
3. do not tie contour rendering semantics to whether `earcut` produced the indices.


## PSLG and Constrained Triangulation

If Datoviz needs general planar straight-line graph support, `earcut` is not enough.

PSLG scope includes:

1. multiple contours,
2. holes,
3. explicit constrained segments,
4. more general planar meshing than simple polygon fills.

Recommendation:

1. leave a clean API place for constrained triangulation now,
2. do not make a Triangle-style backend the default dependency for Phase 1,
3. do not couple the public geometry API to any specific constrained-triangulation library name.

This means the public design should be backend-agnostic, even if `earcut` is the first concrete
backend.


## API Shape for Planar Inputs

The geometry API should distinguish planar input descriptions from renderable geometry output.

Recommended conceptual input types:

1. `DvzPolygon`
2. `DvzPolygonRing`
3. `DvzPSLG`

Recommended conceptual output:

1. `DvzGeometry`

Recommended operation families:

1. polygon triangulation
2. surface-grid triangulation
3. PSLG/constrained triangulation

The geometry object should not carry triangulation-method flags as part of its long-term semantic
identity.

This is one place where v0.3 should change:

1. avoid carrying `DVZ_INDEXING_EARCUT` and `DVZ_INDEXING_SURFACE` as central API semantics,
2. treat those as implementation details or helper modes inside triangulation/preparation code.


## 2D vs 3D API Boundaries

Recommended split:

1. 2D planar input APIs remain explicitly 2D,
2. 3D solid generators remain explicitly 3D,
3. both paths emit or populate the same render-oriented `DvzGeometry` container.

This gives a clean model:

1. polygon rings are not forced into fake 3D input structures,
2. 3D solids do not inherit planar assumptions,
3. rendering only needs one downstream mesh container model.


## Suggested Public API Style

The exact names can evolve, but the standard API shape should look like this conceptually.

Core container:

1. `DvzGeometry* dvz_geometry(void);`
2. `void dvz_geometry_destroy(DvzGeometry* geom);`
3. resize/allocate helpers for vertices and indices

Procedural generators:

1. `dvz_geom_cube(...)`
2. `dvz_geom_plane(...)`
3. `dvz_geom_surface(...)`
4. `dvz_geom_uv_sphere(...)`
5. `dvz_geom_icosphere(...)`
6. `dvz_geom_cylinder(...)`
7. `dvz_geom_cone(...)`
8. `dvz_geom_torus(...)`
9. `dvz_geom_arrow(...)`
10. `dvz_geom_tetrahedron(...)`
11. `dvz_geom_octahedron(...)`
12. `dvz_geom_dodecahedron(...)`
13. `dvz_geom_icosahedron(...)`

Planar triangulation:

1. `dvz_geom_triangulate_polygon(...)`
2. `dvz_geom_triangulate_surface_grid(...)`
3. `dvz_geom_triangulate_pslg(...)`

Operations:

1. `dvz_geom_translate(...)`
2. `dvz_geom_scale(...)`
3. `dvz_geom_rotate(...)`
4. `dvz_geom_transform(...)`
5. `dvz_geom_compute_normals(...)`
6. `dvz_geom_merge(...)`
7. `dvz_geom_unindex(...)`
8. `dvz_geom_bounds(...)`

Import:

1. `dvz_geom_load_obj(...)`


## Configs Versus Long Argument Lists

For shapes with many knobs, prefer small config structs over long function signatures.

Likely useful examples:

1. sphere config
2. cylinder config
3. torus config
4. surface-grid config
5. triangulation config

This makes it easier to express:

1. flat versus smooth normals,
2. indexed versus expanded output,
3. segment counts,
4. generator variants such as UV sphere versus icosphere,
5. triangulation backend or quality flags later.


## Boundary With Mesh Visuals

The geometry layer should feed mesh visuals, not be defined by them.

Rules:

1. geometry generation and preprocessing stay CPU-side,
2. scene/mesh visuals consume resulting geometry data,
3. mesh-specific shading helpers such as contour/isoline metadata generation are optional derived
   steps, not base geometry identity.

This keeps the geometry module reusable across:

1. lit meshes,
2. textured meshes,
3. contour/isoline variants later,
4. future picking or export workflows.


## Immediate v0.4 Scope Recommendation

The first useful `geom` slice does not need the whole eventual feature set.

Recommended early scope:

1. `DvzGeometry` core container,
2. cube generator,
3. surface-grid generator/triangulation,
4. transform helpers,
5. normal generation,
6. merge helpers,
7. bounds helpers,
8. OBJ import,
9. polygon triangulation via `earcut`,
10. clear API space reserved for PSLG later.

That is enough to support:

1. the initial lit cube example,
2. simple scientific surface meshes,
3. imported triangle meshes,
4. future planar filled geometry.


## Explicit Changes From v0.3

The main changes relative to the old `DvzShape` approach should be:

1. rename the central abstraction toward `geom` / `DvzGeometry`,
2. keep the geometry container generic,
3. remove contour/isoline staging fields from the base type,
4. separate planar input descriptions from renderable geometry output,
5. keep triangulation backend choice out of the core geometry identity,
6. support both 2D and 3D cleanly while keeping the output container render-oriented and 3D,
7. keep procedural solids, including the Platonic solids, as first-class geometry generators.
