> **Execution Status**
> - **Status:** `ACTIVE API DESIGN ADDENDUM`
> - **Updated on:** `2026-05-24`
> - **Purpose:** record the v0.4 polygon, polygon-set, and PSLG API decisions before implementation.

# Polygon And PSLG API Design

## Decision Addressed

Datoviz needs filled polygons, polygon outlines, polygon collections, and later PSLG/constrained
triangulation without adding a parallel renderer path or repeating the v0.3 `DvzShape` API.

The main API question is how to expose this without confusing CPU geometry input, leaf visuals, and
higher-level scene objects.


## Chosen Direction

Polygons should land in two layers:

1. CPU-side geometry utilities for triangulation and preprocessing;
2. later scene-level semantic objects for rendered fill/stroke composition.

The first implementation slice should be CPU-side only: triangulate polygon input to `DvzGeometry`,
then render it through the existing `mesh`/`primitive` visual path. A scene composite can follow once
the data model and triangulation behavior are tested.


## API Conventions

This design follows the cross-module public API rules in
[`../../../api/PUBLIC_API_CONVENTIONS.md`](../../../api/PUBLIC_API_CONVENTIONS.md):

1. use typed public functions rather than generic property bags;
2. prefer flat role/property setters for style and frequently changed state;
3. reserve descriptor structs for coherent records and construction or bulk operations;
4. keep C++ and backend-library types out of public headers;
5. keep WASM and generated bindings practical.


## CPU Geometry Names

If `DvzPolygon` becomes the renderable scene object name, CPU-only triangulation input must use a
different name. Preferred candidates:

1. `DvzGeomPolygon`,
2. `DvzPolygonData`.

The first is more consistent with the `geom` module; the second reads more clearly at user call
sites. The implementation should choose one before adding public headers and use it consistently.

The CPU input type should represent:

1. one outer F64 2D ring;
2. zero or more F64 2D hole rings;
3. optional source/user id metadata only when needed for polygon sets;
4. no renderer, panel, color, material, or stroke state.


## First Slice: Polygon Triangulation

The first implementation should add:

```c
DvzGeometry* dvz_triangulate_polygon(
    const DvzGeomPolygon* polygon, const DvzTriangulateDesc* desc);
```

The output is an ordinary `DvzGeometry` with F64 positions, normals, colors where provided, and
triangle-list indices. It should set `DVZ_GEOMETRY_INDEXING_TRIANGULATION`.

The first built-in backend should be earcut because it is already vendored and compatible with the
MIT project. The function should:

1. accept simple polygons with holes;
2. normalize away a repeated closing point;
3. validate finite coordinates, minimum unique vertex counts, and nonzero area;
4. produce deterministic index buffers for stable tests;
5. return `NULL` or an error status on invalid or unsupported input rather than asserting on user
   data.


## Mesh Geometry Upload Helper

`DvzGeometry` is already the CPU geometry container, but uploading it to a scene mesh currently
requires hand-written conversion and multiple generic visual calls. Add a mesh helper after or with
the triangulation slice:

```c
int dvz_mesh_set_geometry(DvzVisual* mesh, const DvzGeometry* geometry);
```

This helper should:

1. require a mesh visual;
2. copy geometry data into retained visual attributes and the visual index buffer;
3. downcast positions, normals, and texcoords to the current F32 visual attribute format;
4. not transfer ownership of `geometry`;
5. remain a convenience wrapper over ordinary visual data/index APIs.

The exact name may still be reviewed during the API consistency pass, but the concept should remain
a copied `DvzGeometry` -> mesh upload helper, not a new geometry ownership path.


## Scene Polygon Composite

After CPU triangulation is tested, add a renderable scene object:

```c
DvzPolygon* polygon = dvz_polygon(scene, flags);
```

The object is a semantic composite, not a `DvzVisual`. Internally it may own:

1. a fill visual, backed by `mesh` or `primitive`;
2. a stroke visual, backed by `path` or `segment`;
3. CPU ring data and derived `DvzGeometry`;
4. dirty flags for ring, fill style, stroke style, and derived visual data.

Common user-facing APIs should be flat and role/property based:

```c
dvz_polygon_outer(polygon, count, xy);
dvz_polygon_hole(polygon, count, xy);
dvz_polygon_fill_color(polygon, color);
dvz_polygon_stroke_color(polygon, color);
dvz_polygon_stroke_width(polygon, width);
```

Do not make a nested `DvzPolygonStyle` the primary API. Optional style convenience structs may be
added later only if the flat setters remain available.

Generated visuals may be exposed by role for advanced use and tests, but ordinary users should not
need to know that fill and stroke are separate visuals.


## Polygon Collections

Many-region use cases such as choropleths should use one semantic polygon object in multi mode or a
dedicated collection object. The API should distinguish array index from stable user id.

Primary per-region setters:

```c
dvz_polygon_outer_at(polygons, polygon_index, count, xy);
dvz_polygon_hole_at(polygons, polygon_index, hole_index, count, xy);
dvz_polygon_fill_color_at(polygons, polygon_index, color);
dvz_polygon_stroke_color_at(polygons, polygon_index, color);
dvz_polygon_stroke_width_at(polygons, polygon_index, width);
```

Optional bulk/range helpers are allowed for large datasets:

```c
dvz_polygon_fill_colors(polygons, first_polygon, polygon_count, colors);
```

Bulk helpers are convenience and performance APIs. They should not replace the per-region API as the
conceptual model.

If stable semantic ids are needed for picking or external data joins, use a separate id setter:

```c
dvz_polygon_region_id_at(polygons, polygon_index, user_id);
```

Do not use a single `region_id` argument when the value is only a positional array index.


## Stroke And Fill Lowering

Filled polygons lower to triangulated `DvzGeometry`, then to `mesh` or `primitive`.

Outlines lower from the original rings, not from triangle edges. This avoids the v0.3 contour
fragility where outline behavior depended on earcut preserving original contour vertices and on
mesh shader sidecar attributes.

Until native closed path spans exist, outlines may repeat the first point at the end of each ring.
Holes should produce their own closed outline rings.


## PSLG Direction

PSLG support should stay CPU-side first:

```c
DvzGeometry* dvz_triangulate_pslg(const DvzPSLG* pslg, const DvzTriangulateDesc* desc);
```

`DvzPSLG` represents F64 points, constrained edges, holes, and optional quality controls. It should
not expose CDT, Triangle, or any backend-specific type.

Backend policy:

1. earcut is the first built-in backend for simple polygon rings with holes;
2. CDT is the preferred first candidate to evaluate for built-in constrained triangulation;
3. Shewchuk Triangle should not be vendored or enabled by default because its license does not
   preserve the expected MIT redistribution and commercial-use surface;
4. Triangle may be supported only as an explicitly optional external backend if useful.


## Implementation Order

1. Add CPU polygon input and earcut-backed `dvz_triangulate_polygon()`.
2. Add focused `geom` tests for simple, concave, holed, closed, degenerate, and invalid polygons.
3. Add copied `DvzGeometry` -> mesh upload helper.
4. Add a small C example that renders a triangulated polygon through `mesh`.
5. Add the scene `DvzPolygon` composite with fill/stroke flat setters.
6. Add polygon collections with per-region and optional range setters.
7. Evaluate CDT for `DvzPSLG` after polygon fill and collection rendering are stable.


## Explicit Non-Goals For The First Slice

1. No new render pipeline for polygons.
2. No public `DvzShape` compatibility layer.
3. No built-in Triangle dependency.
4. No mesh-shader contour sidecar attributes as the primary outline path.
5. No nested style-struct-only API.
