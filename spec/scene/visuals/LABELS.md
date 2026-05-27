# Visual Family: `labels`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`labels` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`,
`../pipeline/ATTRIBUTE_SOURCES.md`, `../semantics/VISUAL_CONTRACT.md`, and
`../semantics/LEGENDS_AND_COLORBARS.md`.

Labels reuse image-rectangle placement machinery, but labels are not an image color mode. They are
integer sampled fields with categorical identity, categorical scale binding, presentation state, and
raw label-id probe behavior.


## Current Implementation Status

Status on 2026-05-27: the active v0.4 runtime implements the first retained 2D labels slice.

The implemented labels visual supports:

1. retained `labels` visual construction via `dvz_labels()`;
2. 2D integer `DvzSampledField` binding through `dvz_visual_set_field(labels, "field", field)`;
3. categorical scale binding through `dvz_visual_set_scale(labels, "labels", scale)`;
4. the same installed placement attributes as `image`: `position`, `extent`, `position_px`,
   `extent_px`, `anchor`, `tex_rect`, and legacy `texcoords`;
5. signed and unsigned integer field formats `R8`, `R16`, and `R32`;
6. retained presentation state for opacity, background label, selected label, hidden labels,
   boundary rendering, fallback seed, and 3D slice placeholders;
7. GLSL and WGSL integer texture-fetch shaders;
8. GPU-backed raw label-id probe/readback for 2D fields;
9. categorical legend integration through the bound scale.

The following sections describe the target labels contract. 3D label slices, optimized sparse/high-id
style lookup, and richer label editing/painting are planned capabilities unless explicitly marked as
implemented above.


## Semantic Purpose

`labels` renders categorical segmentation or region-id fields.

Each texel stores one stable label id. The visual resolves that id to presentation color and
selection state on the GPU. The persistent label-id metadata, display order, names, and colors
belong to the bound categorical scale, not to the visual.

Typical uses: segmentation masks, atlas regions, object IDs, categorical rasters, and napari-style
label overlays.


## Data Model

The primary data source is a 2D integer `DvzSampledField` with semantic
`DVZ_FIELD_SEMANTIC_LABEL`.

Accepted first-slice formats:

```text
R8_UINT,  R16_UINT,  R32_UINT
R8_SINT,  R16_SINT,  R32_SINT
```

Signed integer formats are part of the active contract because real datasets may use negative IDs.
The default background id is `0`, but callers may select another signed or unsigned background id
when it fits the field format.


## Per-Item Attributes

Labels use the same active rectangle-placement attributes as `image`.

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, image rectangle anchor in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |


### `extent`

| Property | Value |
|---|---|
| Type | `vec2`, image rectangle width and height |
| Accepted sources | `PER_ITEM` only in the active slice |
| Typical mutability | `dynamic` |


### `position_px` and `extent_px`

| Property | Value |
|---|---|
| Type | `position_px: vec3`, `extent_px: vec2` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

Pixel-space rectangle placement used for fixed overlays and UI-like label planes. Do not mix
`position`/`extent` and `position_px`/`extent_px` for the same visual item.


### `anchor`

| Property | Value |
|---|---|
| Type | `vec2` |
| Accepted sources | `PER_ITEM` only in the active descriptor |
| Typical mutability | `dynamic` |

Anchor inside the rectangle. The default convention follows `image`: `(-1, -1)` is top-left and
`(1, 1)` is bottom-right.


### `tex_rect`

| Property | Value |
|---|---|
| Type | `vec4`, `(u0, v0, u1, v1)` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

Texture sub-rectangle in normalized field coordinates. Defaults to the whole field.


### `texcoords`

| Property | Value |
|---|---|
| Type | `vec2` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

Legacy four-corner UV path used by some existing image/labels emission code. New examples should
prefer `extent` plus optional `tex_rect`.


## Visual-Wide Parameters

### `field`

| Property | Value |
|---|---|
| Type | 2D integer `DvzSampledField` |
| Binding slot | `"field"` |
| Mutability | `dynamic` |


### `labels` scale

| Property | Value |
|---|---|
| Type | categorical `DvzScale` |
| Binding slot | `"labels"` |
| Mutability | `dynamic` |

The scale owns label id, display color, label text, and display order.


### Presentation State

The active public setters configure transient visual presentation:

| Setter | Meaning |
|---|---|
| `dvz_labels_set_opacity()` | global label overlay opacity |
| `dvz_labels_set_background()` | label id treated as background |
| `dvz_labels_set_selected()` / `dvz_labels_clear_selected()` | highlighted label id |
| `dvz_labels_set_hidden()` | label ids hidden from rendering |
| `dvz_labels_set_boundary()` | boundary rendering style |
| `dvz_labels_set_fallback_seed()` | deterministic fallback color seed |
| `dvz_labels_set_slice_axis()` | placeholder axis for future 3D label slices |
| `dvz_labels_set_slice_position()` | placeholder slice position for future 3D label slices |


## Defaults And Missing Values

| Field | Default | Missing-value policy |
|---|---|---|
| `field` | required | missing field is a validation/planning error |
| categorical scale | optional but recommended | unknown ids use deterministic fallback colors |
| background id | `0` | ids matching background are transparent by default |
| selected id | none | no selected-label emphasis |
| hidden ids | none | all non-background ids visible |
| opacity | `1.0` | invalid opacity is rejected |
| boundary | disabled | invalid width is rejected |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| integer signedness | signed, unsigned | from field format |
| integer width | 8, 16, 32 bits | from field format |
| dimensionality | 2D, future 3D slice | 2D |


## Transform Model, Stage Participation, Picking

Labels use the standard image rectangle transform model.

Rendering samples the integer field with exact texel addressing. Linear interpolation must not be
used for label IDs.

The active request path supports raw 2D label-id probing. A successful probe returns the raw integer
label id as a categorical payload. Item picking over the rectangle follows the image-like item
identity path when enabled.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| continuous scalar or RGBA texture | `image` |
| integer segmentation or region IDs | `labels` |
| 3D scalar field | `volume` |
| future categorical 3D label field | `labels` 3D slice mode or future label-volume path |
| categorical explanation | `legend`, not `colorbar` |


## Minimum Cases This Spec Must Support

1. 2D unsigned segmentation mask with transparent background id `0`,
2. signed integer atlas labels including negative ids,
3. sparse high unsigned ids in `R32_UINT`,
4. selected-label highlight and hidden-label filtering,
5. categorical legend derived from the bound scale,
6. raw probe returning the integer label id under the cursor.


## Deferred Capabilities

1. 3D label fields and slice rendering;
2. arbitrary MPR through label volumes;
3. categorical direct-volume rendering;
4. optimized sparse/high-id GPU style lookup beyond first-slice fallback behavior;
5. label painting and partial edit tools;
6. exact boundary extraction as independent vector geometry;
7. labels-specific lasso/region selection.
