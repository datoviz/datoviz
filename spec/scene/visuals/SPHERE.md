# Visual Family: `sphere`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`sphere` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.


## Semantic Purpose

`sphere` renders per-item 3D spheres with Phong shading.

Each item is one sphere defined by a center position and radius. Spheres are rendered as
ray-cast impostors — a single quad per sphere with the sphere geometry computed analytically
in the shader. This produces pixel-accurate silhouettes and correct depth at far lower cost
than a tessellated mesh sphere per item.

Typical uses: molecular visualization (atoms), particle systems, cell body positions,
electrode contacts, 3D scatter plots with physical extent, textured globes.


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
Ignored when `color_mode = texture`.


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


### `texture`

| Property | Value |
|---|---|
| Type | `SampledField` scene resource |
| Mutability | `dynamic` |
| Applies to | `color_mode = texture` only |

2D texture mapped onto each sphere surface. UV coordinates are computed analytically in the
shader from the surface normal — no per-item texcoords are needed.


### `texture_projection`

| Property | Value |
|---|---|
| Type | enum: `equirectangular`, `spherical` |
| Default | `spherical` |
| Mutability | `dynamic` |
| Applies to | `color_mode = texture` only |

Controls how the texture is mapped onto the sphere surface.

| Value | Description |
|---|---|
| `equirectangular` | standard lat/lon projection — right for globe and planet textures |
| `spherical` | circular area projection, magnified and mirrored front/back — right for general decorative use |


### Lighting Parameters

Standard — see `SHARED_ATTRIBUTES.md`.
Applies when `lighting = phong`. Ignored when `lighting = flat`.

`emissive` and `shininess` additionally accept `PER_GROUP` source for sphere, allowing
different material properties per group (e.g., metallic vs. matte sphere populations in one
visual). `PER_GROUP` is the maximum granularity supported; `PER_ITEM` material attributes
are not supported. Per-item material is PBR territory, deferred to the PBR lighting path.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf sphere skipped and not pickable | no |
| `color` | opaque white RGBA | scalar NaN uses scale missing color | yes |
| `size` | required unless default size is set | scalar NaN uses size fallback | yes |
| texture fields | disabled unless texture mode is selected | missing required texture is validation error | no |
| lighting parameters | shared lighting defaults | NaN falls back to family default | yes |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar`, `texture` | `rgba` |
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
| 2D circular marks without depth | `point` or `marker` |
| Many atoms or particles | `sphere` (impostor, much cheaper than mesh per item) |


## Minimum Cases This Spec Must Support

1. atom positions with per-element color — `color` `PER_ITEM` rgba, `size` `CONSTANT`,
2. per-sphere radius — `size` `PER_ITEM`, `size_space = data`,
3. activity-colored cell bodies — `color_mode = scalar` with colormap Scale,
4. flat-shaded spheres — `lighting = flat`,
5. electrode contacts by group — `color` `PER_GROUP`, `size` `PER_GROUP`,
6. globe with equirectangular texture — `color_mode = texture`, `texture_projection = equirectangular`,
7. mixed-material populations — `emissive` `PER_GROUP`, `shininess` `PER_GROUP`.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_sphere_position` | `position` `PER_ITEM` |
| `dvz_sphere_color` | `color`, extended sources and scalar mode |
| `dvz_sphere_size` | `size`, extended sources and scalar mode |
| `dvz_sphere_texture` | `texture` + `texture_projection` |
| `SPHERE_RECTANGULAR` specialization constant | `texture_projection = equirectangular` |
| `dvz_sphere_light_pos/color` | standard lighting — see `SHARED_ATTRIBUTES.md` |
| `dvz_sphere_material_params` | `ambient`, `diffuse`, `specular` |
| `dvz_sphere_shine` | `shininess`, now also `PER_GROUP` |
| `dvz_sphere_emit` | `emissive`, now also `PER_GROUP` |

v0.4 adds: `PER_GROUP` sources, `scalar` and `texture` color modes, `size_space`,
`size_mode`, `lighting` variant axis, `texture_projection` parameter.
v0.4 renames: `emit` → `emissive`.


## PBR Forward Compatibility

The v0.4 shading model for sphere impostors uses Blinn-Phong.

The visual parameter block reserves `metallic` and `roughness` fields (zero-initialized, ignored in v0.4)
for future PBR support, following the same pattern as `mesh`.
A future `normal_map` texture slot is also reserved.
See `semantics/LIGHTING.md` for the full upgrade path.
