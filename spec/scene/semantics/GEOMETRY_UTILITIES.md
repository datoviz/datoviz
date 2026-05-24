# Geometry Utilities

Status: normative v0.4 scene semantics spec for CPU-side geometry helpers.

Implementation status on 2026-05-22: the active `geom` subset is in core builds with `DvzGeometry`,
owned buffers, cube/plane/sphere/surface-grid generators, bounds, normal recomputation,
transform/merge helpers, surface-grid height updates, F32 conversion helpers, and direct scene mesh
upload from `DvzGeometry` including retained mesh texcoords. Triangulation, curve tessellation,
simplification, hulls, polygon booleans, and richer import paths remain target capabilities.

Implementation update on 2026-05-24: `dvz_geometry_edges()` derives stable unique edge lists from
indexed triangle geometry, and `dvz_geometry_contours()` extracts unstitched contour segments from
per-vertex scalar values and explicit levels. The first live overlay example lowers both outputs to
ordinary `segment` visuals. Future contour stitching may lower connected contours to `path` spans.

Geometry utilities operate above the scene layer and below user code. They produce standard scene
resource data such as F64 vertices, indices, and atlas textures that enter the normal resource upload
path.

Triangulation, polygon-set rendering, mesh upload helpers, and PSLG backend choices are further
refined in
[../proposals/active/POLYGON_PSLG_API_DESIGN.md](../proposals/active/POLYGON_PSLG_API_DESIGN.md).


## Core Rules

1. Utilities are CPU-side data preparation steps, not scene graph nodes or visual families.
2. All geometric computation uses F64. Downcast to F32 happens at `UploadNode` time after normal
   scene normalization; see [`../pipeline/TRANSFORM_PIPELINE.md`](../pipeline/TRANSFORM_PIPELINE.md).
3. Outputs lower to existing resource classes in
   [`../pipeline/RESOURCE_MODEL.md`](../pipeline/RESOURCE_MODEL.md), not special resource classes.
4. v0.4 may break the old `DvzShape` surface. New utility APIs should expose `DvzGeometry`,
   explicit geometry input records, and explicit descriptors rather than preserving shape-era fields
   or compatibility names.
5. Geometry utility naming must stay distinct from renderable scene object naming. If `DvzPolygon`
   becomes a semantic scene object, CPU-side triangulation input should use a name such as
   `DvzGeomPolygon` or `DvzPolygonData`.


## Bundled Dependencies

| Utility area | Library | Contract |
|---|---|---|
| simple polygon triangulation | earcut (C++) | holes, fast, no external deps |
| constrained Delaunay / PSLG | permissive candidate TBD | constrained edges, holes, optional refinement |
| curve tessellation | built-in | Bezier, Catmull-Rom, B-spline |
| line simplification | built-in | Douglas-Peucker |
| 2D hull | built-in | convex hull baseline; concave deferred |
| boolean polygon ops | Clipper2 | union, intersection, difference, XOR |
| MSDF/SDF generation | msdfgen | marker shapes, annotation shapes |
| glyph atlas packing | msdf-atlas-gen | font/glyph atlases |


## Procedural Render Geometry

Generators produce ordinary indexed geometry payloads.

| Generator | Descriptor controls | Notes |
|---|---|---|
| primitive solids | cube, sphere, cylinder, cone, torus, arrow, classic polyhedra | cube and sphere implemented; remaining solids targeted |
| `dvz_geom_plane` | center, width, height, z offset, color | implemented as indexed XY plane |
| `dvz_geom_gizmo_axes` | axis length, shaft/head dimensions, tessellation, per-axis colors | scene/app owns pinning and camera sync |
| `dvz_geom_surface_grid` | rows/cols, height/color arrays, origin, basis, height policy, normals, metadata | implemented with row/column provenance and height updates; richer update helpers remain future work |


## Derived Mesh Overlays

Mesh wireframe is derived data, not a baseline mesh vertex layout concern. The first public helper
path derives unique edges from triangle indices and preserves endpoint vertex ids, adjacent face
counts, and boundary/nonmanifold flags. Scene or example code should lower those edges to `segment`
or a segment-like overlay pass with screen-space stroke width, analytic coverage, butt caps by
default, and depth bias or small geometric offset when drawn over the source surface.

Scalar isolines are also derived data. The first helper path emits independent contour segments from
indexed triangle geometry and a scalar value per vertex. This is intentionally exportable and
backend-neutral. Connected contour stitching, closed contour loops, label placement, and direct
`path` lowering remain follow-up work. Visual-only shader isolines are a separate future mesh
variant, not a replacement for semantic contour geometry.


## Triangulation

| Input | Function | Contract | Use |
|---|---|---|---|
| `DvzGeomPolygon` or `DvzPolygonData` | `dvz_triangulate_polygon` | outer F64 ring plus holes -> F64 vertices + `uint32` indices | filled polygons, regions, annotation shapes |
| `DvzPSLG` | `dvz_triangulate_pslg` | F64 points, constrained edges, holes, quality -> F64 vertices + `uint32` indices | boundaries, constrained/scattered Delaunay |

`DvzTriangulateQuality` carries optional minimum angle and maximum triangle area. Triangulation
backends operate in F64; no upload-time downcast occurs here.

Earcut is the preferred first built-in backend for simple polygons with holes because it is already
vendored and license-compatible with the MIT project. Shewchuk Triangle remains technically useful
but should not be bundled into the default Datoviz source or binary distribution because its license
does not preserve the expected MIT redistribution and commercial-use surface. Triangle may be
supported only as an explicitly optional external backend. A permissive constrained-Delaunay backend
such as CDT should be evaluated before adding built-in PSLG support.


## Curve Tessellation

Curve tessellation outputs flat F64 `dvec3` polyline data for `path` resources.

| Curve | Function | Controls |
|---|---|---|
| quadratic/cubic Bezier | `dvz_tessellate_bezier_cubic` and related forms | F64 deviation tolerance in data units |
| Catmull-Rom | `dvz_tessellate_catmull_rom` | steps per segment; passes through controls |
| uniform cubic B-spline | `dvz_tessellate_bspline` | steps per segment and degree |


## Line Simplification

`dvz_simplify_path(pts_f64, n_pts, tolerance, &out_pts, &out_count)` applies
Douglas-Peucker simplification. `tolerance` is maximum F64 perpendicular deviation in data units.
The output is a topology-preserving subset of input points. This is a one-shot utility; automatic
zoom-dependent LOD is future work.


## 2D Hull

`dvz_hull_convex(pts_f64, n_pts, &hull_pts, &hull_count)` returns an ordered CCW F64 polygon ring
using an O(n log n) algorithm. Concave/alpha-shape hull computation is deferred.


## Boolean Polygon Operations

| Operation | Function |
|---|---|
| union | `dvz_polygon_union` |
| intersection | `dvz_polygon_intersection` |
| difference | `dvz_polygon_difference` |
| XOR | `dvz_polygon_xor` |

All functions accept and return `DvzPolygon` objects with outer ring plus holes. Input/output are
F64. The Clipper2 wrapper scales F64 coordinates to exact integer arithmetic internally and converts
back transparently.


## SDF And MSDF

The SDF/MSDF pipeline uses msdfgen and msdf-atlas-gen for resolution-independent marker shapes,
annotation decorations, and text glyphs.

| Path | Contract |
|---|---|
| SVG path -> MSDF/SDF | `dvz_msdf_from_svg` / `dvz_sdf_from_svg` return float textures; `dvz_marker_tex_scale` matches texture width |
| font handle | `DvzFont` is typeface/metrics identity; atlas storage is separate and shareable |
| default font | `dvz_font_default(scene)` returns the built-in typeface |
| custom font | `dvz_font_load(scene, ttf_bytes, ttf_size)` |
| glyph usage | glyph/text objects reference a compatible `DvzFont*`; atlases are shared by default |
| preloading | `dvz_font_preload_string` and `dvz_font_preload_codepoints` avoid runtime growth when known ahead |
| import/export | `dvz_font_export` / `dvz_font_import` preserve pre-baked atlas startup paths |

Lazy auto-grow is the default glyph atlas policy. New codepoints mark compatible glyph/text users
dirty; atlas regeneration or patching is deferred to the next frame boundary, never mid-render.
Static atlas policy may reject undeclared codepoints instead.

`marker` in `DVZ_MARKER_MODE_MSDF` may index a shared atlas per item using a shape-index attribute,
allowing multiple custom marker shapes in one visual. Annotation shapes such as boxes, bubbles, and
arrows may use the same SDF infrastructure. GPU-side SDF generation via jump flooding is future
compute work.


## Resource Mapping

| Utility output | Resource path |
|---|---|
| triangulation | `IndexedGeometry` -> `mesh` |
| curve tessellation | `ItemTable`/`GroupedItemTable` -> `path` |
| hull | CPU polygon input -> triangulation -> `mesh`/`primitive` |
| boolean polygons | CPU polygon input -> triangulation -> `mesh` |
| MSDF/SDF | `Texture2DResource` -> `marker`, `glyph`, or annotation |

No special resource handling is needed.
