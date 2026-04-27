# Geometry Utilities

This document specifies the CPU-side geometry utility layer bundled with Datoviz.

Geometry utilities operate above the scene layer and below user code.
They produce standard scene resource data (vertex arrays, index arrays, atlas textures) that
feed the normal resource upload path.


## Purpose

Certain geometry operations come up repeatedly across scientific visualization workflows and
cannot be cleanly delegated to user code or Python:

1. triangulation is needed before any polygon can be rendered as a mesh,
2. curve tessellation is needed before smooth paths can be rendered as line geometry,
3. line simplification is needed to reduce geographic or anatomical contour complexity at
   varying zoom levels,
4. hull computation is needed for cluster outlines, ROI boundaries, and point-set enclosures,
5. boolean polygon operations are needed for geographic overlays and region masking,
6. MSDF/SDF generation is needed for high-quality marker shapes and text rendering.

All utilities operate in **F64 (double-precision float)** throughout.
Their output vertex data enters the normal resource upload path and is downcast to F32 at
`UploadNode` time by the standard CPU precision policy (see `TRANSFORM_PIPELINE.md`).


## Bundled Dependencies

| Utility area | Library | Notes |
|---|---|---|
| Simple polygon triangulation | earcut (C++) | fast, handles holes, no external deps |
| Constrained Delaunay / PSLG | Triangle (Shewchuk) | quality mesh, PSLG input, public domain |
| Curve tessellation | built-in | Bézier, Catmull-Rom, B-spline |
| Line simplification | built-in | Douglas-Peucker |
| 2D hull | built-in | convex hull; concave hull deferred |
| Boolean polygon ops | Clipper2 | union, intersection, difference; small dep |
| MSDF/SDF generation | msdfgen | already bundled in v0.3 |
| Glyph atlas packing | msdf-atlas-gen | already bundled in v0.3 |


---

## Triangulation

### Simple Polygon — `DvzPolygon`

`DvzPolygon` is the input type for simple polygon triangulation.

It carries:
1. an outer ring of F64 vertices,
2. zero or more hole rings (each a contiguous vertex array).

The scene calls earcut internally and returns a flat F64 vertex array and a `uint32` index
array, both ready for a `mesh` visual.

```text
DvzPolygon poly = {
    .vertices    = (dvec2*) outer_pts,
    .vertex_count = n_outer,
    .holes        = hole_arrays,
    .hole_counts  = hole_sizes,
    .hole_count   = n_holes,
}
dvz_triangulate_polygon(&poly, &out_vertices, &out_indices)
```

Use this path for simple filled polygons, region outlines, and annotation shapes.


### PSLG — `DvzPSLG`

`DvzPSLG` is the input type for constrained Delaunay triangulation via the Triangle library.

A PSLG (Planar Straight-Line Graph) carries:
1. a set of F64 point coordinates,
2. a set of required (constrained) edges that must survive the triangulation,
3. optional hole seed points.

```text
DvzPSLG pslg = {
    .points       = pts_f64,
    .point_count  = n,
    .segments     = edges,          // (i, j) index pairs — must appear in output
    .segment_count = n_segs,
    .holes        = hole_seeds,     // one interior point per hole
    .hole_count   = n_holes,
}
dvz_triangulate_pslg(&pslg, &quality_opts, &out_vertices, &out_indices)
```

`DvzTriangulateQuality` carries optional Triangle-library quality constraints:
- minimum angle bound (degrees),
- maximum triangle area bound.

Use this path when:
1. specific edges must be preserved (coastlines, anatomical region boundaries),
2. quality mesh generation with angle or area constraints is needed,
3. scattered point triangulation (Delaunay from a point cloud) is needed.


### Triangulation And Precision

Both triangulators operate in F64 and produce F64 vertex output.
Earcut uses F64 arithmetic for all winding and intersection tests.
Triangle (Shewchuk) is a F64 library natively.
Downcast to F32 happens at `UploadNode` time, not here.


---

## Curve Tessellation

Curve tessellation converts smooth curve control points into polyline vertex sequences for
rendering with the `path` family.

Three curve types are supported:

### Bézier Curves

| Type | Control points |
|---|---|
| Quadratic | 3 per segment |
| Cubic | 4 per segment |

```text
dvz_tessellate_bezier_cubic(ctrl_pts, n_segments, tolerance, &out_pts, &out_count)
```

`tolerance` is the maximum F64 deviation between the curve and its polyline approximation,
in data-space units.
Smaller tolerance produces more vertices.


### Catmull-Rom Spline

Produces a smooth curve passing through all control points.
Adjacent control points define a $C^1$ continuous spline.

```text
dvz_tessellate_catmull_rom(ctrl_pts, n_pts, steps_per_segment, &out_pts, &out_count)
```

`steps_per_segment` controls the number of output polyline points per input interval.


### B-Spline

Uniform cubic B-spline through or near the control points.

```text
dvz_tessellate_bspline(ctrl_pts, n_pts, steps_per_segment, degree, &out_pts, &out_count)
```


### Tessellation Output

All tessellators return a flat F64 `dvec3` array suitable for upload as a `path` visual
vertex buffer.
The output count depends on the tolerance or steps-per-segment setting.


---

## Line Simplification

Douglas-Peucker simplification reduces the vertex count of a polyline while preserving its
visual shape within a given tolerance.

```text
dvz_simplify_path(pts_f64, n_pts, tolerance, &out_pts, &out_count)
```

`tolerance` is the maximum F64 perpendicular distance, in data-space units, that a removed
point may deviate from the simplified segment.

Use cases:
1. geographic data rendered at low zoom — simplify coastlines and roads before upload,
2. large anatomical contours — reduce vertex count without visible loss,
3. LOD (level-of-detail) path variants at different simplification levels.

The output is a subset of the input points (Douglas-Peucker is topology-preserving).
Output is F64 and feeds the normal upload path.


---

## 2D Hull

### Convex Hull

```text
dvz_hull_convex(pts_f64, n_pts, &hull_pts, &hull_count)
```

Returns the convex hull as an ordered F64 polygon ring (counter-clockwise).
Uses an $O(n \log n)$ algorithm.

Use cases: cluster outlines in scatter plots, electrode array footprints, bounding regions.


### Concave Hull

Concave (alpha-shape) hull computation is deferred.
Use the convex hull as the initial approximation.


---

## Boolean Polygon Operations

Boolean polygon operations on 2D polygons via the Clipper2 library.

Supported operations:

| Operation | Function |
|---|---|
| Union | `dvz_polygon_union` |
| Intersection | `dvz_polygon_intersection` |
| Difference | `dvz_polygon_difference` |
| XOR | `dvz_polygon_xor` |

All functions accept and return `DvzPolygon` objects (outer ring + holes).
Input and output are in F64.

Clipper2 uses exact integer arithmetic internally (coordinates are scaled to integers with
a configurable precision factor); the wrapper converts F64 ↔ integer transparently.

Use cases:
1. geographic region masking (clip data to a polygon boundary),
2. merging overlapping ROIs,
3. computing overlap regions between two anatomical areas.


---

## SDF And MSDF Pipeline

### Purpose And v0.3 Baseline

The SDF/MSDF pipeline enables high-quality anti-aliased rendering of arbitrary vector shapes —
custom marker symbols, text glyphs, and annotation decorations — at any resolution and scale.

Datoviz bundles **msdfgen** (multi-channel signed distance field generator) and
**msdf-atlas-gen** (atlas packer for fonts and glyph sets).
Both were already present in v0.3.

The pipeline is used by:
1. the `marker` family — `DVZ_MARKER_MODE_SDF` and `DVZ_MARKER_MODE_MSDF`,
2. the `glyph` family — all text rendering via font atlases.


### SVG Path → MSDF Texture

A single SVG path string can be converted directly to an MSDF texture via the v0.3 functions,
which carry forward unchanged into v0.4:

```text
float* msdf = dvz_msdf_from_svg(svg_path_string, width, height)
// single-channel SDF variant:
float* sdf  = dvz_sdf_from_svg(svg_path_string, width, height)
```

The result is a float texture that is uploaded and attached to a marker visual in `MSDF` or
`SDF` mode.
`dvz_marker_tex_scale` must be set to the texture width so the shader scales SDF distances
correctly.

This is the recommended path for custom marker shapes defined as SVG paths.


### Font Glyph Atlas — `DvzAtlas`

`DvzAtlas` is the scene resource for font-backed glyph atlases.

```text
DvzAtlas* atlas = dvz_atlas(ttf_size, ttf_bytes)
dvz_atlas_codepoints(atlas, codepoints, n)   // or dvz_atlas_string for a string set
dvz_atlas_generate(atlas)
DvzTexture* tex = dvz_atlas_texture(atlas, batch)
```

The atlas packs selected glyphs into a single MSDF texture.
Individual glyph coordinates are looked up via `dvz_atlas_glyph` and `dvz_atlas_glyphs` for
batch lookup.

Atlases can be pre-generated and saved with `dvz_atlas_export` / `dvz_atlas_import` to avoid
regenerating them at startup.


### Per-Item Shape Variation Via Atlas

In v0.4, a `marker` visual in `DVZ_MARKER_MODE_MSDF` can index into a shared `DvzAtlas` on
a per-item basis using a shape index attribute.
This allows a single marker visual to display different custom shapes per item, without
requiring one visual per shape.

The shape index attribute selects the atlas entry; the atlas carries the UV coordinates for
each packed shape.
This is a v0.4 addition over v0.3 (which required separate visuals for different MSDF shapes).


### Annotation And Callout Shapes

The SDF pipeline applies to annotation shapes (rounded boxes, speech bubbles, pointer arrows)
generated via msdfgen rather than rasterized.
This produces crisp shapes at any DPI without pre-rendering at a fixed pixel size.
Annotation shape generation is deferred; the infrastructure (msdfgen + upload path) is in
place.


### GPU-Side SDF Computation (Future)

For distance fields derived from dynamic binary masks (e.g., brain region outlines from
segmentation maps), GPU-side SDF computation via the jump flooding algorithm is a future
compute path.
It would produce a `Texture2DResource` suitable for use in the SDF marker or annotation
pipeline.
This is not part of the v0.4 baseline but the architecture supports it via `ComputeNode`.


---

## Relationship To Scene Resources

Geometry utility output is not a special resource class.
The output of triangulation, tessellation, simplification, hull, and boolean operations is
plain F64 vertex and index data that enters the standard resource upload path:

1. triangulation output → `IndexedGeometry` resource → `mesh` visual,
2. curve tessellation output → `ItemTable` or `GroupedItemTable` resource → `path` visual,
3. hull output → `DvzPolygon` → triangulation → `mesh` or `primitive` visual,
4. boolean output → `DvzPolygon` → triangulation → `mesh` visual,
5. MSDF output → `Texture2DResource` → `marker` or `glyph` visual.

No special resource handling is needed.
The utilities are CPU-side data preparation steps, not scene-layer extension points.


---

## Deferred Questions

1. concave hull (alpha shapes) — useful for non-convex point clusters; deferred until use cases
   are clearer,
2. whether boolean polygon operations should be exposed at the scene API level or remain a
   lower-level utility,
3. per-item shape variation via atlas — exact attribute API spelling is deferred,
4. GPU-side SDF computation (jump flooding) — deferred to a future compute path,
5. whether Douglas-Peucker simplification should be available as an automatic LOD strategy
   rather than a one-shot utility.
