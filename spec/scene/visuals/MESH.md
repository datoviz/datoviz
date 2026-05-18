# Visual Family: `mesh`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`mesh` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`mesh` renders indexed triangle geometry with optional lighting, texture mapping, edge overlay,
and isoline rendering.

Each item in the mesh geometry resource is one vertex. Triangles are defined by an index buffer
(three vertex indices per triangle). This is the primary family for 3D surface geometry.

Typical uses: brain surfaces, terrain, 3D anatomical models, procedural geometry, height fields,
isosurfaces, polyhedral shapes.

Volumetric cell meshes used by FEM, CFD, and finite-volume solvers are a related but distinct data
model. They should not be forced into the surface `mesh` family directly. See the exploratory
[`../proposals/UNSTRUCTURED_GRID_DESIGN.md`](../proposals/UNSTRUCTURED_GRID_DESIGN.md) note for
tetra/hexa/wedge cell topology, cell-centered fields, cut planes, and element picking.


## Current Implementation Status

Status on 2026-05-17: the active v0.4 runtime implements the first retained mesh slice plus
GPU instanced draws through `instance_transform`.

The implemented path supports:

1. retained `mesh` visual construction via `dvz_mesh()`;
2. triangle-list rendering through the scene -> FramePlan -> DRP2 -> vklite path;
3. `position` per-vertex data;
4. optional `color` per-vertex data, with an opaque-white default when omitted;
5. optional `normal` per-vertex data for the current lit material shader path;
6. optional scene-owned `"index"` buffer bindings for indexed draws;
7. optional `instance_transform` mat4 data, one transform per mesh instance;
8. depth-tested rendering, arcball/camera transforms, WBOIT/depth-peeling participation, SSAO
   G-buffer participation when normals are present, and offscreen/app execution coverage.

The following sections describe the target mesh contract. Texture material slots, scalar colormap
mode, automatic normal generation, edge overlay, isolines, shape-builder integration, and mesh
face/region picking are planned capabilities unless explicitly marked as implemented above.


## Surface Plot Contract

A surface plot is a structured-grid mesh convenience, not a separate baseline visual family.

The preferred construction path is:

1. `geom` generates a structured surface-grid `DvzGeometry` payload from row/column counts,
   heights, optional scalar/color data, origin, and two grid basis vectors;
2. the scene uploads or updates that payload as a mesh geometry resource;
3. the `mesh` visual renders it with the usual mesh styling: lighting, material, colormap,
   texture, edge overlay, transparency, and isolines;
4. app/UI code may expose surface-specific controls such as height scale, colormap range,
   contour mode, and camera presets.

The convenience API may expose a user-facing `surface` helper, but internally it should still
create or update a mesh geometry resource. It should not create a parallel surface renderer or a
second mesh-like visual contract.

When the input is a regular grid, the scene should keep optional structured-grid provenance
alongside the mesh resource. This provenance is useful for efficient height-only updates, normal
recomputation, row/column edge overlays, surface-specific contours, scalar isolines, and future
level-of-detail. Rendering can still proceed through ordinary indexed triangles.


## Geometry Resource

The `mesh` visual references a scene-owned mesh geometry resource.

The resource owns:

1. vertex attributes,
2. the index buffer,
3. dirty ranges for partial vertex/index updates,
4. resource identity used for sharing across visuals and panels.

The visual owns material, transform, visibility, picking policy, and other instance state. A mesh
with `instance_transform` renders the same vertex/index payload once per transform with a DRP2
instanced draw instead of duplicating vertices.
Convenience APIs may upload geometry directly when constructing a mesh visual, but semantically that
creates or replaces a scene mesh resource rather than making the visual privately own geometry.


## Per-Vertex Attributes

Each item in the geometry resource is one vertex.

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |


### `color`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`.
Applies when `color_mode = rgba` or `scalar`.
When `color_mode = scalar` and `lighting = phong`, the colormap-derived color feeds the diffuse
term of the Phong equation.
Ignored when `color_mode = texture`.


### `normal`

| Property | Value |
|---|---|
| Type | `vec3`, unit surface normal |
| Accepted sources | `CONSTANT`, `PER_ITEM` |
| Typical mutability | `static` |
| Optional | yes — see below |

Per-vertex normal used for Phong shading. When not provided and `lighting = phong`, the scene
computes flat normals from the index buffer and position data automatically.
Providing smooth normals explicitly (e.g., from a mesh file) produces smoother shading.
Ignored when `lighting = flat`.


### `texcoords`

| Property | Value |
|---|---|
| Type | `vec2`, `(u, v)` in `[0, 1]` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `static` |
| Optional | yes — required only when `color_mode = texture` |

UV coordinates for texture sampling. The user-facing type is `vec2`; any additional per-vertex
data needed by the implementation is managed internally.
Not used when `color_mode` is `rgba` or `scalar`.


### `isoline_value`

| Property | Value |
|---|---|
| Type | `float32` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |
| Optional | yes — required only when `isoline_count > 0` |

Per-vertex scalar used to draw isoline contours on the mesh surface.
Isolines are placed at `isoline_count` evenly-spaced levels within `isoline_range`.


## Index Buffer

| Property | Value |
|---|---|
| Type | flat `uint32` array, three indices per triangle |
| Mutability | `dynamic` |

Defines the triangle list for the mesh resource. No indexed lines or points — triangle list only.
The index count must be a multiple of 3.
Can be replaced at any time; visuals referencing the resource observe the new geometry on the next
validated frame plan.


## Visual-Wide Parameters

### `texture`

| Property | Value |
|---|---|
| Type | `SampledField` scene resource |
| Mutability | `dynamic` |
| Applies to | `color_mode = texture` only |

2D texture applied via per-vertex `texcoords`. Must be RGBA `u8`.

Status on 2026-05-17: this is not implemented for mesh visuals yet. The active texture binding
path is image-only; mesh texture support should add a mesh material texture slot rather than reuse
the image-only convenience setter directly.


### `colormap`

| Property | Value |
|---|---|
| Type | `Scale` reference (kind = color) — see `semantics/SCALES.md` |
| Mutability | `dynamic` |
| Applies to | `color_mode = scalar` only |

Maps per-vertex scalar values to display colors. The Scale domain also defines the range used
by `isoline_range` when `isoline_range` is not set explicitly.


### `backface_culling`

| Property | Value |
|---|---|
| Type | `bool` |
| Default | `true` |
| Mutability | `dynamic` |

When `true`, back-facing triangles are not rendered (standard for solid closed meshes).
Set to `false` for open surfaces that must be visible from both sides (e.g., cortical surfaces,
thin shells).


### Lighting Parameters

Standard — see `SHARED_ATTRIBUTES.md`.
Applies when `lighting = phong`. Ignored when `lighting = flat`.


### Edge Overlay Parameters

#### `edgecolor`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | transparent — no edges drawn |
| Mutability | `dynamic` |

Color of triangle edges (wireframe lines) drawn on top of the mesh surface.
Set to transparent to disable. Edge detection is handled internally by the scene.

The preferred implementation is a derived edge-overlay geometry path, not Vulkan polygon line mode
and not a permanent fragment-only wireframe mode in the baseline mesh shader. The scene should
derive a unique edge list from the triangle index buffer, classify boundary and interior edges when
that information is useful, and render those edges as a separate overlay pass using the same panel
transform and depth state as the source mesh.

#### `linewidth`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `1.0` |
| Mutability | `dynamic` |

Width of drawn edges. Only relevant when `edgecolor` is not transparent.


### Wireframe Recommendation

Status on 2026-05-16: mesh wireframe is a planned visual capability, not part of the active
first-slice mesh runtime.

The recommended high-quality path is:

1. derive an edge table from the mesh indices, preserving stable edge ids when the mesh resource is
   stable;
2. expand each edge to a camera-facing ribbon or segment impostor in a dedicated overlay pipeline;
3. evaluate analytic coverage in the fragment shader for antialiased edges;
4. depth-test against the mesh depth buffer, with optional depth bias or polygon-offset equivalent
   to reduce z-fighting;
5. expose edge color, linewidth, opacity, boundary-only mode, and feature-edge mode as mesh visual
   parameters;
6. allow the edge overlay to participate in transparency and capture/export as an ordinary derived
   scene contribution.

Barycentric-coordinate wireframe remains useful as a diagnostic shader variant, but it requires
vertex duplication or extra barycentric payloads and couples edge styling to the surface shader.
It should not be the default public mesh wireframe path.


### Isoline Parameters

#### `isoline_count`

| Property | Value |
|---|---|
| Type | `uint32` |
| Default | `0` — isolines disabled |
| Mutability | `dynamic` |

Number of evenly-spaced isoline levels within `isoline_range`.
Requires `isoline_value` per-vertex data.

#### `isoline_range`

| Property | Value |
|---|---|
| Type | `vec2` — `(min, max)` |
| Default | derived from the `colormap` Scale domain when `color_mode = scalar`; otherwise auto-computed from `isoline_value` data |
| Mutability | `dynamic` |

Value range over which isoline levels are distributed.
Setting this explicitly overrides the auto-derived range.

#### `isoline_color`

| Property | Value |
|---|---|
| Type | `rgba_u8` or `Scale` reference (kind = color) |
| Default | black `(0, 0, 0, 255)` |
| Mutability | `dynamic` |

Color of isoline contours. Two forms:

1. **Uniform** (`rgba_u8`): all isolines share one color.
2. **Mapped** (`Scale` reference): each isoline level is colored by mapping its data value
   through the Scale. This allows elevation-colored contours or multi-valued isofield display.

When a Scale is used, the isoline value at each level is looked up through the Scale domain
and palette, independent of the `colormap` Scale used for mesh face coloring.

For mesh isolines, prefer a dedicated mesh-isoline or derived-overlay path once scalar surface
fields are retained in the mesh resource. Keep isoline payloads out of the baseline position/color/
normal mesh vertex layout unless the selected shader variant actually needs them.


## Shape Builders

`mesh` integrates with a `Shape` builder API that generates vertex and index data for common
geometries: `square`, `disc`, `sphere`, `cube`, `cylinder`, `cone`, `torus`, `surface` (height
field), and the Platonic solids. Shape builders produce a `Shape` object that is uploaded to a
mesh geometry resource in one call. This is a CPU-side convenience layer, not a separate visual
family.

Shape builders also support per-shape transforms (`scale`, `translate`, `rotate`) and merging
multiple shapes into a single mesh resource.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf vertex invalidates affected primitive or fails validation in strict mode | no |
| `index` | optional for non-indexed draws | invalid index is validation error | no |
| `color`, `normal`, `texcoord` | mode-dependent defaults | scalar NaN uses scale missing color | yes |
| edge overlay fields | disabled unless enabled | invalid width/color falls back only when explicitly configured | yes |
| lighting parameters | shared lighting defaults | NaN falls back to family default | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar`, `texture` | `rgba` |
| `lighting` | `phong`, `flat` | `phong` |

Both set at visual creation time.

`color_mode = texture` requires `texcoords` per-vertex and a `texture` resource.
`color_mode = scalar` with `lighting = phong`: the colormap-derived color feeds the Phong
diffuse term.
`lighting = flat` skips normal computation and Phong shading; `normal` data is ignored.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
Picking returns the **face (triangle) index**, not the vertex index. Face picking is more
useful for mesh interaction (the user clicks a surface region, not a vertex). The face index
is the index of the first vertex of the triangle divided by 3 (i.e., `vertex_index / 3` for
non-indexed triangles, or the index into the index buffer triplet for indexed geometry).

Status on 2026-05-17: mesh face picking is specified here but not implemented. The current
GPU-backed request path covers point picking and image probing first.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| 3D spheres with per-sphere attributes | `sphere` |
| 2D filled polygons without depth | `primitive` with `triangle_list` |
| Volume rendering | `volume` |
| Point cloud | `point` or `pixel` |


## Minimum Cases This Spec Must Support

1. brain surface with vertex-colored regions — `color_mode = rgba`, Phong, `backface_culling = false`,
2. terrain with satellite texture — `color_mode = texture`,
3. activity-colored cortical surface — `color_mode = scalar` with colormap Scale,
4. flat-shaded polyhedral mesh — `lighting = flat`,
5. wireframe overlay — `edgecolor` set,
6. isoline contours on a height field — `isoline_value` `PER_ITEM`, `isoline_count > 0`,
7. procedural sphere from shape builder — `dvz_shape_sphere` → `mesh`,
8. interactive app with deferred data — visual created empty, geometry uploaded on data arrival.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_mesh_position` | `position` `PER_ITEM` |
| `dvz_mesh_color` | `color` `PER_ITEM` |
| `dvz_mesh_texcoords` | `texcoords` `PER_ITEM` (user-facing `vec2`, internal layout hidden) |
| `dvz_mesh_normal` | `normal` `PER_ITEM`, now optional with auto-compute fallback |
| `dvz_mesh_isoline` | `isoline_value` `PER_ITEM` |
| `dvz_mesh_left/right/contour` | hidden — implementation detail for edge rendering |
| `dvz_mesh_texture` | `texture` resource |
| `dvz_mesh_index` | `index` buffer, now `dynamic` |
| `dvz_mesh_light_pos/color` | `light_pos`, `light_color` |
| `dvz_mesh_material_params` | `ambient`, `diffuse`, `specular` |
| `dvz_mesh_shine` | `shininess` |
| `dvz_mesh_emit` | `emissive` |
| `dvz_mesh_edgecolor/linewidth` | `edgecolor`, `linewidth` |
| `dvz_mesh_density` | `isoline_count` |
| `dvz_mesh_alloc` | not needed — visual resizes automatically |
| `dvz_mesh_shape` / `dvz_mesh_reshape` | shape builder integration |

The target v0.4 contract adds: `color_mode` variant axis, `lighting` variant axis,
`backface_culling`, `isoline_range`, auto-computed normals, `colormap` as Scale reference,
`CONSTANT` color source, empty visual support, and dynamic index buffer updates. The active
first slice currently covers empty visual construction, dense per-vertex uploads, default color
generation, normals when supplied, and scene-owned index buffers.
v0.4 renames: `emit` → `emissive`.
v0.4 hides `left`/`right`/`contour` — edge distance data is computed internally.


## PBR Forward Compatibility

The v0.4 material model uses Blinn-Phong shading (`ambient`, `diffuse`, `specular`,
`shininess`, `emissive`).

The visual parameter block reserves two additional fields for future PBR support:

| Reserved field | PBR role | v0.4 value |
|---|---|---|
| `metallic` | metallic factor `[0, 1]` | zero-initialized, ignored |
| `roughness` | roughness factor `[0, 1]` | zero-initialized, ignored |

A `normal_map` texture slot is reserved in the target v0.4 contract alongside the diffuse
`texture`. When `normal_map` is set, the fragment shader perturbs the surface normal using the
tangent-space normal map before Phong shading. `texcoords` are shared between `texture` and
`normal_map`.

```text
dvz_visual_set_texture(mesh, 0, diffuse_tex)    // slot 0: diffuse color
dvz_visual_set_texture(mesh, 1, normal_map_tex) // slot 1: normal map (optional)
```

When PBR rendering is activated in a future version, the `metallic` and `roughness` fields
drive the Cook-Torrance BRDF without any change to the public API surface.
See `semantics/LIGHTING.md` for the full PBR and ray tracing upgrade path.
