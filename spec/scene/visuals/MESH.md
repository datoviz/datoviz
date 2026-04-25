# Visual Family: `mesh`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`mesh` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`mesh` renders indexed triangle geometry with optional lighting, texture mapping, edge overlay,
and isoline rendering.

Each item is one vertex. Triangles are defined by an index buffer (three vertex indices per
triangle). This is the primary family for 3D surface geometry.

Typical uses: brain surfaces, terrain, 3D anatomical models, procedural geometry, height fields,
isosurfaces, polyhedral shapes.


## Per-Vertex Attributes

Each item is one vertex.

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |


### `color`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`.
Applies when `color_mode = rgba` or `scalar`. Ignored when `color_mode = texture`.


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

UV coordinates for texture sampling. Not used when `color_mode` is `rgba` or `scalar`.


### `isoline_value`

| Property | Value |
|---|---|
| Type | `float32` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |
| Optional | yes — required only when isoline rendering is enabled |

Per-vertex scalar used to draw isoline contours on the mesh surface.
Isolines appear at evenly-spaced levels between the domain min and max.


## Index Buffer

| Property | Value |
|---|---|
| Type | flat `uint32` array, three indices per triangle |
| Mutability | `static` |

Defines the triangle list. No indexed lines or points — triangle list only.
The index count must be a multiple of 3.


## Visual-Wide Parameters

### `texture`

| Property | Value |
|---|---|
| Type | `SampledField` scene resource |
| Mutability | `dynamic` |
| Applies to | `color_mode = texture` only |

2D texture applied via per-vertex `texcoords`. Must be RGBA `u8`.


### `colormap`

| Property | Value |
|---|---|
| Type | `Scale` reference (kind = color) — see `SCALES.md` |
| Mutability | `dynamic` |
| Applies to | `color_mode = scalar` only |

Maps per-vertex scalar values to display colors.


### Lighting Parameters

Applies when `lighting = phong`. Ignored when `lighting = flat`.

#### `light_pos`

| Property | Value |
|---|---|
| Type | array of up to 4 `vec4` — `(x, y, z, w)` where `w = 0` for directional, `w = 1` for point |
| Default | single directional light at `(1, 1, 1, 0)` |
| Mutability | `dynamic` |

#### `light_color`

| Property | Value |
|---|---|
| Type | array of up to 4 `rgba_u8` |
| Default | white `(255, 255, 255, 255)` |
| Mutability | `dynamic` |

#### `ambient`, `diffuse`, `specular`

| Property | Value |
|---|---|
| Type | `float32` in `[0, 1]` each |
| Default | `0.2`, `0.7`, `0.3` |
| Mutability | `dynamic` |

Material reflection coefficients.

#### `shininess`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `32.0` |
| Mutability | `dynamic` |

Phong specular exponent. Higher values produce tighter specular highlights.

#### `emit`

| Property | Value |
|---|---|
| Type | `float32` in `[0, 1]` |
| Default | `0.0` |
| Mutability | `dynamic` |

Self-emission factor. At `1.0` the mesh appears fully lit regardless of light positions.
Useful for unlit regions or emissive surfaces.


### Edge Overlay Parameters

#### `edgecolor`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | transparent — no edges drawn |
| Mutability | `dynamic` |

Color of triangle edges drawn on top of the mesh surface. Set to transparent to disable.

#### `linewidth`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | `1.0` |
| Mutability | `dynamic` |

Width of drawn edges. Only relevant when `edgecolor` is not transparent.


### Isoline Parameters

#### `isoline_count`

| Property | Value |
|---|---|
| Type | `uint32` |
| Default | `0` — isolines disabled |
| Mutability | `dynamic` |

Number of evenly-spaced isoline levels across the `isoline_value` domain.
Requires `isoline_value` per-vertex data to be set.

#### `isoline_color`

| Property | Value |
|---|---|
| Type | `rgba_u8` |
| Default | black `(0, 0, 0, 255)` |
| Mutability | `dynamic` |

Color of all isoline contours. Visual-wide.


## Shape Builders

`mesh` integrates with a `Shape` builder API that generates vertex and index data for common
geometries: `square`, `disc`, `sphere`, `cube`, `cylinder`, `cone`, `torus`, `surface` (height
field), and the Platonic solids. Shape builders produce a `Shape` object that is uploaded to the
mesh visual in one call. This is a CPU-side convenience layer, not a separate visual family.

Shape builders also support per-shape transforms (`scale`, `translate`, `rotate`) and merging
multiple shapes into a single mesh visual.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar`, `texture` | `rgba` |
| `lighting` | `phong`, `flat` | `phong` |

Both set at visual creation time.

`color_mode = texture` requires `texcoords` per-vertex and a `texture` resource.
`lighting = flat` skips normal computation and Phong shading; `normal` data is ignored.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
Picking returns the vertex index. Triangle-level picking (returning face index) is a deferred
question.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| 3D spheres with per-sphere attributes | `sphere` |
| 2D filled polygons without depth | `primitive` with `triangle_list` |
| Volume rendering | `volume` |
| Point cloud | `point` or `pixel` |


## Minimum Cases This Spec Must Support

1. brain surface with vertex-colored regions — `color_mode = rgba`, Phong lighting,
2. terrain with satellite texture — `color_mode = texture`,
3. activity-colored cortical surface — `color_mode = scalar` with colormap Scale,
4. flat-shaded polyhedral mesh — `lighting = flat`,
5. wireframe overlay — `edgecolor` set, `lighting = flat`,
6. isoline contours on a height field — `isoline_value` `PER_ITEM`, `isoline_count > 0`,
7. procedural sphere from shape builder — `dvz_shape_sphere` → `mesh`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_mesh_position` | `position` `PER_ITEM` |
| `dvz_mesh_color` | `color` `PER_ITEM` |
| `dvz_mesh_texcoords` | `texcoords` `PER_ITEM` |
| `dvz_mesh_normal` | `normal` `PER_ITEM`, now optional with auto-compute fallback |
| `dvz_mesh_isoline` | `isoline_value` `PER_ITEM` |
| `dvz_mesh_left/right/contour` | hidden — implementation detail for edge rendering |
| `dvz_mesh_texture` | `texture` resource |
| `dvz_mesh_index` | `index` buffer |
| `dvz_mesh_light_pos/color` | `light_pos`, `light_color` |
| `dvz_mesh_material_params` | `ambient`, `diffuse`, `specular` |
| `dvz_mesh_shine` | `shininess` |
| `dvz_mesh_emit` | `emit` |
| `dvz_mesh_edgecolor/linewidth` | `edgecolor`, `linewidth` |
| `dvz_mesh_density` | `isoline_count` |
| `dvz_mesh_shape` / `dvz_mesh_reshape` | shape builder integration |

v0.4 adds: `color_mode` variant axis, `lighting` variant axis, auto-computed normals,
`colormap` as Scale reference, `CONSTANT` color source.
v0.4 hides `left`/`right`/`contour` — edge distance data is computed internally.


## Deferred Questions

1. triangle-level (face) picking vs. vertex picking,
2. whether smooth normal auto-computation (area-weighted vertex normals) should be offered
   in addition to flat normals,
3. whether per-vertex `emit` or `shininess` is needed for heterogeneous material surfaces,
4. whether multiple textures (e.g., diffuse + normal map) should be supported,
5. whether `isoline_color` should support a `Scale` for multi-colored isolines.
