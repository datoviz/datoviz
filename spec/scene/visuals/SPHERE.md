# Visual Family: `sphere`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`sphere` visual family.

It refines `VISUAL_FAMILIES.md`, `VISUAL_MINI_CONTRACTS.md`, `ATTRIBUTE_SOURCES.md`, and
`VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`sphere` renders per-item 3D spheres with Phong shading.

Each item is one sphere defined by a center position and radius. Spheres are rendered as
ray-cast impostors — a single quad per sphere with the sphere geometry computed analytically
in the shader. This produces pixel-accurate silhouettes and correct depth at far lower cost
than a tessellated mesh sphere per item.

Typical uses: molecular visualization (atoms), particle systems, cell body positions,
electrode contacts, 3D scatter plots with physical extent.

For a single large sphere or a sphere with a custom texture, use `mesh` with `dvz_shape_sphere`.


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` center in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |


### `color`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.
When `color_mode = scalar` and `lighting = phong`, the colormap-derived color feeds the
diffuse term.


### `size`

Standard — see `SHARED_ATTRIBUTES.md`. Sphere radius (not diameter).
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.
Supports `direct` and `scalar` modes — see `size_mode` variant axis.


## Visual-Wide Parameters

### `size_space`

Standard — see `SHARED_ATTRIBUTES.md`. Default: `data`.

`data` is the natural default for `sphere` — spheres typically represent physical objects
whose size should scale with zoom (atoms, cells). Use `screen` when spheres are decorative
markers that should stay constant size regardless of zoom.


### Lighting Parameters

Standard — see `SHARED_ATTRIBUTES.md`.
Applies when `lighting = phong`. Ignored when `lighting = flat`.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `size_mode` | `direct`, `scalar` | `direct` |
| `lighting` | `phong`, `flat` | `phong` |

All set at visual creation time.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
Picking returns the sphere index. The impostor shader computes the exact ray–sphere
intersection, so picking is geometrically accurate (not bounding-box based).


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Large sphere or custom texture | `mesh` with `dvz_shape_sphere` |
| 2D circular marks without depth | `point` or `marker` |
| Many atoms or particles | `sphere` (impostor, much cheaper than mesh per item) |


## Minimum Cases This Spec Must Support

1. atom positions with per-element color — `color` `PER_ITEM` rgba, `size` `CONSTANT`,
2. per-sphere radius — `size` `PER_ITEM`, `size_space = data`,
3. activity-colored cell bodies — `color_mode = scalar` with colormap Scale,
4. flat-shaded spheres — `lighting = flat`,
5. electrode contacts by group — `color` `PER_GROUP`, `size` `PER_GROUP`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_sphere_position` | `position` `PER_ITEM` |
| `dvz_sphere_color` | `color`, extended sources and scalar mode |
| `dvz_sphere_size` | `size`, extended sources |
| `dvz_sphere_texture` | deferred — see below |
| `dvz_sphere_light_pos/color` | standard lighting — see `SHARED_ATTRIBUTES.md` |
| `dvz_sphere_material_params` | `ambient`, `diffuse`, `specular` |
| `dvz_sphere_shine` | `shininess` |
| `dvz_sphere_emit` | `emissive` |

v0.4 adds: `PER_GROUP` sources, `scalar` color mode, `size_space`, `lighting` variant axis.
v0.4 renames: `emit` → `emissive`.
v0.4 defers: texture-mapped spheres (spherical UV mapping) — see deferred questions.


## Deferred Questions

1. whether texture-mapped spheres (spherical UV computed analytically in the shader, no
   user-supplied texcoords) should be supported as a `color_mode = texture` variant,
2. whether per-item `emissive` or `shininess` is useful for mixed-material sphere sets.
