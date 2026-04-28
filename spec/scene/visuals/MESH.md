# Visual Family: `mesh`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`mesh` visual family.

It refines `VISUAL_FAMILIES.md`, `../VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
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

Defines the triangle list. No indexed lines or points — triangle list only.
The index count must be a multiple of 3.
Can be replaced at any time; the visual resizes as needed.


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


## Shape Builders

`mesh` integrates with a `Shape` builder API that generates vertex and index data for common
geometries: `square`, `disc`, `sphere`, `cube`, `cylinder`, `cone`, `torus`, `surface` (height
field), and the Platonic solids. Shape builders produce a `Shape` object that is uploaded to the
mesh visual in one call. This is a CPU-side convenience layer, not a separate visual family.

Shape builders also support per-shape transforms (`scale`, `translate`, `rotate`) and merging
multiple shapes into a single mesh visual.


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

v0.4 adds: `color_mode` variant axis, `lighting` variant axis, `backface_culling`,
`isoline_range`, auto-computed normals, `colormap` as Scale reference, `CONSTANT` color source,
empty visual support, dynamic index buffer.
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

A `normal_map` texture slot is supported in v0.4 alongside the diffuse `texture`.
When `normal_map` is set, the fragment shader perturbs the surface normal using the tangent-space
normal map before Phong shading. `texcoords` are shared between `texture` and `normal_map`.

```text
dvz_visual_set_texture(mesh, 0, diffuse_tex)    // slot 0: diffuse color
dvz_visual_set_texture(mesh, 1, normal_map_tex) // slot 1: normal map (optional)
```

When PBR rendering is activated in a future version, the `metallic` and `roughness` fields
drive the Cook-Torrance BRDF without any change to the public API surface.
See `LIGHTING.md` for the full PBR and ray tracing upgrade path.
