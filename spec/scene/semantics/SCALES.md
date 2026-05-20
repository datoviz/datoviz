# Scene Scales

## Status

Normative for scene-level scale and colormap semantics.

## Purpose

A scale is a scene-owned mapping from scalar or categorical data to a visual encoding such as
color, size, or opacity. Visual attributes reference scales; colorbars and legends attach to scale
identity, not directly to visuals.

## Core Rules

1. Scales are scene-owned objects, not visual-private or DRP2-level constructs.
2. A scale maps input values to a typed output.
3. Mapping is declared at the scene level and applied before or during frame planning.
4. Backend implementation details such as palette textures or CPU mapping are not user-visible.
5. Scale identity is stable within the owning scene.
6. Shared explanatory objects may aggregate only visuals that reference the same scale identity.

## Scale Kinds

| Kind | Input | Output | Purpose |
|---|---|---|---|
| `color` | `float32` scalar | `rgba_u8` | continuous colormap |
| `categorical` | `int32` category id | `rgba_u8` | discrete color set |
| `size` | `float32` scalar | `float32` size | size mapping |
| `opacity` | `float32` scalar | `float32` alpha in `[0, 1]` | opacity mapping |

Future kinds such as `linewidth` or `shape` may be added without changing the identity model.

## Continuous Color Scale

Required fields:

| Field | Description |
|---|---|
| `domain_min`, `domain_max` | input range, `float64` |
| `clamp` | `clamp` default or `repeat` for cyclic quantities |
| `unit` | optional display label only |
| `palette` | built-in name or custom RGBA stops |
| `interpolation` | `linear`, `log`, `sqrt`, or `power(gamma)` |
| `center` | optional diverging midpoint for linear interpolation |

Rules:

- The domain is in data space; v0.4 does not auto-fit unless a future API requests it.
- Values outside the domain clamp to endpoints unless `clamp = repeat`.
- `log` requires `domain_min > 0`; `sqrt` requires `domain_min >= 0`.
- `unit` affects labels only; no unit algebra is applied.
- Output is always `rgba_u8`.
- Diverging `center` maps to palette `t = 0.5`, uses piecewise-linear normalization, and is ignored
  when interpolation is not linear.

Minimum named palettes:

| Name | Type |
|---|---|
| `viridis`, `plasma`, `magma`, `inferno`, `cividis` | perceptual sequential |
| `greys`, `reds`, `blues`, `greens` | sequential |
| `coolwarm`, `bwr` | diverging |
| `hsv` | cyclic |

Custom palettes are arrays of RGBA stops. Stop positions are uniform unless explicit positions in
`[0, 1]` are supplied.

## Categorical Scale

A categorical scale maps integer category ids directly to discrete colors.

Rules:

- Input is `int32`; output is `rgba_u8`.
- No normalization or interpolation is applied.
- Category id `i` maps to `colors[i % len(colors)]`.
- Built-in color sets must include `tab10` and `tab20`.
- Custom color sets are explicit `rgba_u8` arrays.
- Palette/color-set updates mark the scale dirty but do not require reuploading item ids.
- Item-id updates mark the visual data dirty but do not change the scale.

## Size Scale

| Field | Description |
|---|---|
| `domain_min`, `domain_max` | input scalar range |
| `output_min`, `output_max` | output size range |
| `output_unit` | `screen_pixels` or `data_units` |
| `interpolation` | `linear`, `sqrt`, or `log` |
| `unit` | optional input display label |

`sqrt` is useful for area perception; `log` is useful for data spanning orders of magnitude.

## Opacity Scale

| Field | Description |
|---|---|
| `domain_min`, `domain_max` | input scalar range |
| `interpolation` | `linear` default |

The output alpha is `float32` clamped to `[0, 1]` and is typically multiplied into a base color.

## Attribute Sources

When a visual attribute uses a scale, the declaration binds:

1. the attribute, such as `color`;
2. the scale object;
3. the source: `CONSTANT`, `PER_ITEM`, or `PER_GROUP`.

For color scales:

- `PER_ITEM`: one scalar per item;
- `CONSTANT`: one scalar for all items;
- `PER_GROUP`: one scalar per group.

For categorical scales, the same source modes supply integer ids.

## Declaration Forms

Preferred form: create an explicit scale handle and attach it to visuals and explanatory objects.

```text
scale = scene_scale(scene, {
    kind = COLOR,
    domain_min = 0.0,
    domain_max = 1.0,
    palette = VIRIDIS,
})
visual_set_color_scale(visual, scale)
visual_set_color_source(visual, PER_ITEM)
```

Inline shortcut: a visual may declare mapping parameters directly when the scale is used by exactly
one visual and no colorbar/legend sharing is required. The scene creates an anonymous internal
scale. Anonymous scales cannot be shared or attached to explanatory objects.

Common C shorthands:

```c
DvzScale* dvz_scale_color(DvzScene* scene, const char* palette, double min, double max);
DvzScale* dvz_scale_categorical(DvzScene* scene, const char* palette, uint32_t n_categories);
DvzScale* dvz_scale_size(DvzScene* scene, float px_min, float px_max, double min, double max);
DvzScale* dvz_scale_opacity(DvzScene* scene, double min, double max);
```

All return a `DvzScale*` accepted by `dvz_visual_set_scale`.

## Custom Colormap Registration

Scene-scoped registration makes a user colormap referenceable by name:

```text
dvz_colormap_register(scene, "my_map", colors_rgba_u8, n)
```

`n >= 2`; 256 stops is typical for smooth continuous colormaps. Registering an existing name
replaces the palette and marks all referencing scales dirty.

## Updates And Invalidation

Supported updates without recreating visuals:

| Update | Dirtied state | Item data reupload |
|---|---|---|
| domain bounds | scale | no |
| palette/stops/color set | scale | no |
| scalar/category data | visual item data | yes |

This separation keeps colormap changes independent from item-data uploads.

## Capability Adaptation

If GPU palette lookup is unavailable, the scene may map scalar/category data to RGBA on the CPU at
upload time. The fallback is transparent except that:

1. the uploaded visual data is RGBA rather than scalar/category ids;
2. a capability diagnostic is emitted;
3. dynamic domain or palette updates require reuploading item color data.

## Related Specs

| Document | Relationship |
|---|---|
| `visuals/PIXEL.md` | first family using scalar color mode |
| `semantics/LEGENDS_AND_COLORBARS.md` | explanatory objects attach to scale identity |
| `pipeline/ATTRIBUTE_SOURCES.md` | source modes feeding scales |
| `pipeline/INVALIDATION_AND_CACHING.md` | dirty propagation |
| `validation/ADAPTATION.md` | capability fallback diagnostics |
