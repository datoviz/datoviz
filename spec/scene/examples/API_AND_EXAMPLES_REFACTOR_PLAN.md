# API and Examples Refactor Plan

Status: draft handoff plan. Updated: 2026-06-09.

This plan records the decisions for the v0.4 examples and API consistency pass. v0.4-dev does not
need to preserve v0.3 API or ABI compatibility when a break gives a clearer, more consistent public
surface.


## Goals

1. Make public examples read like the intended v0.4 API, not like compatibility shims.
2. Separate scene examples from low-level runtime and host-integration examples.
3. Make sizing and scale semantics explicit across live windows, offscreen capture, scenario
   runners, screenshots, and browser routes.
4. Keep geometry and asset-import examples backed by public `geom`/`fileio` APIs rather than
   example-local parsers or shape builders.


## API Consistency Rules

Prefer descriptor-based constructors for runtime, integration, and importer APIs where options
interact or hidden defaults would be misleading.

Use this convention:

1. `dvz_xxx_desc()` returns a fully initialized default descriptor.
2. Descriptor-based constructors are canonical for app/view/window/capture/video/external-surface
   and geometry-import APIs.
3. Short convenience constructors are allowed only when the default behavior is obvious, the object
   is common, and the helper is a thin wrapper over the canonical path.
4. Avoid `*_with_config()` variants when a single descriptor can represent the same state.
5. Keep units explicit in names and comments: logical pixels, framebuffer pixels, device scale, user
   scale, and render scale are distinct.

Keep these short constructors or concise scene-object factories:

- `dvz_scene()`;
- `dvz_figure(scene, width, height, flags)`;
- `dvz_panel_full(figure)`;
- visual-family constructors such as `dvz_point(scene, flags)`, `dvz_marker(scene, flags)`,
  `dvz_mesh(scene, flags)`, and similar;
- common retained scene/domain helpers such as `dvz_scale(scene, NULL)`, `dvz_scale_bar(panel, NULL)`,
  and controller constructors with an optional descriptor pointer.

Normalize these toward descriptors:

- app creation and borrowed resources;
- view creation, including GLFW, offscreen, external surface, hosted/FFI, and future browser-shaped
  views;
- capture/video options;
- offscreen sizing and render-scale options;
- geometry builders/importers where options matter, including triangulation, PSLG, OBJ, PLY, and
  future glTF.


## View and Scale Semantics

The canonical view API should move toward one descriptor path:

```c
DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_OFFSCREEN);
desc.size_policy = DVZ_VIEW_SIZE_HOST_LOGICAL_PX;
desc.size_width = 800;
desc.size_height = 600;
desc.device_scale = 2.0f;
desc.user_scale = 1.0f;
desc.render_scale = 1.0f;

DvzView* view = dvz_view(app, figure, &desc);
```

Definitions:

- `DVZ_VIEW_SIZE_HOST_LOGICAL_PX`: requests layout size in logical pixels.
- `DVZ_VIEW_SIZE_FRAMEBUFFER_PX`: requests physical render-target size in pixels. Logical size may
  be derived from framebuffer size and device scale.
- `device_scale`: physical pixels per logical pixel. Native window backends usually own this value.
- `user_scale`: user-controlled multiplier for UI-like scene quantities such as text size, marker
  edge width, line width, tick sizes, margins, reserves, and overlay padding.
- `render_scale`: future quality/supersampling multiplier. It should not be conflated with Retina
  or user scale.

Default offscreen capture should remain exact-pixel and deterministic: asking for `1600x1200`
should produce a `1600x1200` image. Logical offscreen sizing should be explicit, for example
`logical_size=800x600, device_scale=2`, producing a `1600x1200` framebuffer. Supersampling should
be separate and should downsample to the requested output size.

Existing `dvz_view_device_scale()`, `dvz_view_user_scale()`, and `dvz_view_set_user_scale()` are the
right public direction. A live window should generally get device scale from the backend; user code
should change `user_scale` when it wants UI-like scene quantities to grow or shrink.


## Scenario Runner Integration

Scenario dimensions should be logical dimensions. The runner should carry scale explicitly:

```c
uint32_t logical_width;
uint32_t logical_height;
uint32_t framebuffer_width;
uint32_t framebuffer_height;
float device_scale;
float user_scale;
float render_scale;
```

Suggested runner options:

- `--size 1600x1200`: exact framebuffer/output pixels, logical size defaults to the same size and
  `device_scale=1`.
- `--logical-size 800x600 --device-scale 2`: logical layout at `800x600`, framebuffer at
  `1600x1200`.
- `--user-scale 1.5`: UI-like scene quantities scale without changing the framebuffer.
- future `--render-scale 2`: supersampled rendering with explicit downsample semantics.

The browser/WASM path should map the same model to CSS canvas size, WebGPU texture size, and
`devicePixelRatio`.


## Scale Proof Example

Add `examples/c/features/user_scale.c`.

The example should show a 2D axis plot with markers, marker edges, a stroked path, tick labels,
axis labels, and a small GUI slider that calls `dvz_view_set_user_scale(view, scale)`.

Expected behavior:

1. Marker edge widths, path widths, tick widths, tick lengths, font sizes, margins, reserves, and
   overlay-like spacing scale smoothly.
2. Axes and ticks are recomputed instead of being stretched as pixels.
3. Figure and panel layout remain stable and readable across approximately `0.75x` to `2.5x`.
4. The example becomes a regression target for screen-space scaling.

The current internals already track `device_scale`, `render_scale`, and `user_scale` during frame
emission and mark screen-space resources dirty when these values change. The implementation pass
should audit every screen-space annotation/visual path for missed `_scene_screen_scale()` usage.


## Examples Taxonomy

Keep the public categories:

- `examples/c/visuals/`: one visual family per example;
- `examples/c/features/`: one isolated scene feature, rendering technique, interaction feature, or
  common output workflow;
- `examples/c/composites/`: one semantic object that lowers to one or more visuals;
- `examples/c/showcases/`: composed scientific workflows and polished release material.

Add:

- `examples/c/advanced/`: low-level, host-integration, and non-default runtime examples that are
  useful to users but are not scene-gallery examples.

Keep:

- `examples/c/lab/`: diagnostics, experiments, temporary repros, and non-public development
  material.

Initial `advanced/` candidates:

- `raw_triangle_vklite.c`;
- `raw_triangle_drp2.c`;
- `external_surface_glfw.c`;
- possibly direct app/window examples when the main point is runtime setup rather than scene
  semantics.

Keep common output workflows in `features/` when they are likely user-facing:

- offscreen capture;
- video export;
- scene JSON/export diagnostics if positioned as scene serialization/debug output.

GUI examples stay in `features/` when they demonstrate GUI controls or GUI viewport features.
Hosted toolkit or borrowed-surface examples belong in `advanced/`.


## Example Naming and Triage

Rename technique examples with a `technique_` prefix:

- `technique_edl`;
- `technique_ssao`;
- `technique_msaa`;
- `technique_depth_cue`;
- possibly `technique_depth_test` and `technique_transparency`.

Remove weak `scene_` prefixes when the feature is clearer without them:

- `basic_scene` -> `basic_scene` or retained-scene naming;
- `scene_json` -> `json_export`;
- `compute_buffer_animation` -> `compute_buffer_animation`.

Other agreed renames:

- `panzoom` -> `panzoom`;
- `brain_volume` -> `brain_volume`.

Merge picking/selection examples into one strong `features/picking.c` with hover and selection
feedback. Keep tight tests for y-flip and coordinate readback regressions.


## External Surface Example

Replace the current null-surface diagnostic with an advanced example:

`examples/c/advanced/external_surface_glfw.c`

The example should:

1. Create a GLFW window directly.
2. Query GLFW Vulkan instance extensions.
3. Create `DvzApp` with those extensions.
4. Get Datoviz's borrowed `VkInstance`.
5. Create a `VkSurfaceKHR` with `glfwCreateWindowSurface()`.
6. Create a Datoviz view from the borrowed surface.
7. Drive a host-owned event/render loop.
8. Update framebuffer extent and content scale on resize.
9. Call `dvz_view_release_external_surface()` before destroying the host-owned surface.

This example should be native-only and advanced; it is not a gallery-facing scene feature.


## Geometry, Triangulation, and PSLG

Keep low-level geometry helpers:

```c
DvzGeometryEdges* dvz_geometry_edges(const DvzGeometry*);
DvzGeometryContours* dvz_geometry_contours(...);
```

Use `dvz_geometry_edges()` in triangulation examples to display generated triangle edges. Use
`dvz_geometry_contours()` in an isolines example, not in a triangulation example.

Add:

- `examples/c/features/triangulation_pslg.c`: once a public PSLG API exists, show input vertices
  and constrained segments versus output triangles.
- `examples/c/features/isolines.c`: structured surface grid or mesh scalar field with extracted
  contour segments/isolines.

Long-term PSLG API direction:

```c
DvzGeometry* dvz_triangulate_polygon(
    const DvzPolygonDesc* polygon, const DvzTriangulationDesc* desc);

DvzGeometry* dvz_triangulate_pslg(
    const DvzPslgDesc* pslg, const DvzTriangulationDesc* desc);
```

`DvzPslgDesc` should support vertices, constrained edges/segments, optional holes, optional region
seeds, and future region attributes. Do not fake PSLG examples with ad hoc pre-triangulated data.


## Builtin Shapes

Add shape builders in `geom`, not as example-local helpers.

Recommended examples:

- `examples/c/features/builtin_shapes_2d.c`: square/rectangle, disc, sector, regular polygon,
  star, and polygon with a hole.
- `examples/c/features/builtin_shapes_3d.c`: cube, UV sphere, cylinder, cone, torus, and arrow.
  Keep full gizmo axes and Platonic solids planned unless they can be added cleanly.

v0.3 exposed a broad shape family: square, disc, sector, histogram, polygon, surface, cube, sphere,
cylinder, cone, arrow, gizmo, torus, tetrahedron, hexahedron/cube, octahedron, dodecahedron,
icosahedron, OBJ, and custom shapes. v0.4 currently has cube, plane/rectangle, disc, sector,
regular polygon, star, UV sphere, cylinder, cone, torus, arrow, surface grid, polygon triangulation,
contour extraction, and Bezier tessellation. Full gizmo axes, classic polyhedra, histogram helpers,
and custom-shape compatibility remain planned or out of scope; avoid reintroducing v0.3 API names
solely for compatibility.


## Asset Import

OBJ should be a geometry/file-import feature, not a builtin shape.

Add a clean v0.4 loader API instead of copying the v0.3 `dvz_shape_obj()` surface:

```c
DvzGeometryObjDesc dvz_geometry_obj_desc(void);
DvzGeometry* dvz_geom_obj(const char* path, const DvzGeometryObjDesc* desc);
```

Implemented first slice: `v`, `vn`, and polygonal `f` records; faces are triangulated as fans;
missing normals are computed; texture coordinates/materials/groups are ignored. Normalize/center,
texcoords, vertex colors, object/group/material filtering, and a backend swap to `tiny_obj_loader.h`
remain planned if example pressure requires them.

Public proof lives in `examples/c/features/obj_loading.c`.

PLY remains planned for scientific and engineering workflows: point clouds, scans, meshes with
vertex colors/normals/scalar properties. Prefer `miniply` as the initial backend because it is small,
MIT-licensed, C++11-friendly, supports ASCII and binary PLY, and can triangulate polygon faces. Wrap
it behind a descriptor-based C API:

```c
DvzGeometryPlyDesc dvz_geometry_ply_desc(void);
DvzGeometry* dvz_geom_ply(const char* path, const DvzGeometryPlyDesc* desc);
```

Initial PLY support should cover positions, normals, RGB/RGBA colors, triangle faces, optional face
triangulation, and a clear decision on where scalar fields are stored.

Mark glTF 2.0 as v0.5 asset-import work. It is valuable but broader than mesh loading because it
brings node transforms, materials, images, buffers, coordinate conventions, and possibly animation.
`cgltf` is the likely C-friendly backend when this lane starts.


## Pickup Order

1. Write a short API consistency note or fold these rules into the relevant spec files.
2. Add `examples/c/advanced/` and move/create raw/runtime/host-integration examples there.
3. Design and implement `DvzViewDesc`/`dvz_view()` and scenario scale fields.
4. Add `features/user_scale.c` and use it to audit screen-space scaling.
5. Add polygon triangulation and isolines examples using public `geom` helpers.
6. Add builtin 2D and 3D shape builders and examples.
7. Add OBJ loader and `features/obj_loading.c`.
8. Keep PLY planned unless release pressure requires implementation.
9. Keep glTF 2.0 as v0.5 asset-import work.
10. Rename and triage existing examples, then run build/test/screenshot validation and
    `git diff --check`.
