# Scene Scales

This document defines the scene-level scale and colormap model.

A scale is a scene-owned mapping from a scalar domain to a visual encoding such as color, size, or
opacity.

Scales are referenced by visual attributes and by explanatory objects such as colorbars and legends.


## Purpose

Scales serve two related roles:

1. they let users declare a reusable, named mapping from data values to visual encodings,
2. they give the scene layer a stable identity to share between visuals and their explanatory
   objects.

Without explicit scale identity, a colorbar cannot know which visual it explains, and two visuals
with visually similar mappings cannot be distinguished from two visuals that share the same
underlying semantic mapping.


## Position

Scales sit between:

1. visual attribute declarations — a visual may declare that one of its attributes uses a scale,
2. scene resources — a scale may own or reference a small parameter block or lookup texture,
3. explanatory objects — a colorbar or legend entry attaches to a scale, not to a visual directly.

Scales are scene-owned objects.
They are not visual-private or DRP2-level constructs.


## Core Rule

A scale maps scalar input values to a typed output.

The mapping is always:

1. declared at the scene level,
2. applied by the scene layer before or during frame planning,
3. not expressed as backend shader constants or texture units directly.

The GPU implementation — uniform color lookup, palette texture sampling, or other — is chosen by
the scene layer and is not user-visible.


## Scale Kinds

The first scene slice should support these scale kinds:

1. `color` — maps `float32` scalars to `rgba_u8` colors via a continuous colormap palette,
2. `categorical` — maps integer category IDs to `rgba_u8` colors via a discrete color set,
3. `size` — maps `float32` scalars to a float size value within a declared output range,
4. `opacity` — maps `float32` scalars to an alpha value in `[0, 1]`.

Additional scale kinds such as `linewidth` or `shape` may be added later.

The most important and most common kind is `color`.
`categorical` is semantically distinct from `color` and is described in its own section.
The rest of this document covers color scales first, then categorical, then size and opacity.


## Color Scale

A color scale maps a scalar domain to colors through a named or custom palette.


### Domain

The domain is the range of input scalar values the scale expects.

| Property | Description |
|---|---|
| `domain_min` | lower bound of the input range, `float64` |
| `domain_max` | upper bound of the input range, `float64` |
| `clamp` | behavior outside the domain: `clamp` (default) or `repeat` |

Values outside the domain are clamped to the nearest endpoint by default.
`repeat` wraps the value modulo the domain width, useful for cyclic quantities such as angles.

The domain lives in data space.
It is the user's responsibility to set it to a meaningful range for their data.
The scene does not auto-fit the domain unless a future API explicitly requests it.


### Palette

The palette defines the color sequence the scale maps onto.

Two palette sources are supported:

**Named palette**: the user selects a palette by name from the built-in set.

The minimum required named palettes are:

| Name | Description |
|---|---|
| `viridis` | perceptually uniform, colorblind-friendly sequential |
| `plasma` | perceptually uniform sequential, warm tones |
| `magma` | perceptually uniform sequential, dark tones |
| `inferno` | perceptually uniform sequential |
| `cividis` | colorblind-safe sequential |
| `greys` | greyscale sequential |
| `reds`, `blues`, `greens` | single-hue sequential |
| `coolwarm` | diverging, blue–red |
| `bwr` | diverging, blue–white–red |
| `hsv` | cyclic, hue-based |

Additional palettes may be added without breaking the contract.

**Custom palette**: the user supplies an array of RGBA color stops.
The scene interpolates linearly between stops.
Stop positions are uniform unless the user also supplies explicit stop positions in `[0, 1]`.


### Normalization

The scale normalizes input values to `[0, 1]` before palette lookup.

The normalization function is controlled by the `interpolation` field:

| Value | Formula | Typical use |
|---|---|---|
| `linear` (default) | `t = (v - min) / (max - min)` | general purpose |
| `log` | `t = (log(v) - log(min)) / (log(max) - log(min))` | intensity maps, power-law data |
| `sqrt` | `t = (√v - √min) / (√max - √min)` | count data, area perception |
| `power(γ)` | `t = ((v - min) / (max - min))^γ` | gamma correction, contrast |

`log` normalization requires `domain_min > 0`.
`sqrt` normalization requires `domain_min ≥ 0`.
All normalizations clamp `t` to `[0, 1]` (or wrap for `repeat` clamp mode).

For `repeat` clamp mode:

```
t = fmod((value - domain_min) / (domain_max - domain_min), 1.0)
```

### Diverging Center

For diverging palettes (e.g., `coolwarm`, `bwr`) where the palette midpoint should align
with a specific data value rather than the domain midpoint, an optional `center` field
overrides the default:

| Field | Default | Description |
|---|---|---|
| `center` | `(domain_min + domain_max) / 2` | data value that maps to palette midpoint (`t = 0.5`) |

When `center` is set, the normalization is piecewise-linear:
- below center: `t = 0.5 * (v - domain_min) / (center - domain_min)`
- above center: `t = 0.5 + 0.5 * (v - center) / (domain_max - center)`

This allows asymmetric domains (e.g., −10 to +5 with white at 0) without artificially
adjusting `domain_min` or `domain_max`.
`center` is ignored when `interpolation ≠ linear`.


### Output Type

The output of a color scale is always `rgba_u8` (4-byte RGBA).

When a visual attribute uses a color scale:

1. `PER_ITEM` source: one `float32` scalar per item is uploaded; the scene or GPU maps it to color,
2. `CONSTANT` source: one `float32` is supplied; one color is computed and applied to all items,
3. `PER_GROUP` source: one `float32` per group is supplied; each group gets one mapped color.

The scene layer may apply the mapping on the CPU at upload time or on the GPU via a palette texture,
depending on item count, update frequency, and runtime capability.
The user does not choose the mapping location.


## Categorical Scale

A categorical scale maps integer category IDs to colors from a discrete color set.

It is semantically distinct from a continuous color scale:

1. the input is an integer category ID, not a continuous scalar,
2. there is no domain normalization — each ID is an index directly into the color set,
3. the mapping is not interpolated between entries,
4. out-of-range IDs wrap modulo the color set size.

### Color Set

Two color set sources are supported:

**Named color set**: the user selects a built-in discrete palette by name.

| Name | Description |
|---|---|
| `tab10` | 10 visually distinct colors |
| `tab20` | 20 visually distinct colors |

Additional named categorical palettes may be added without breaking the contract.

**Custom color set**: the user supplies an explicit array of `rgba_u8` colors.
Category ID `i` maps to entry `i % len(colors)`.

### Input And Output

| Property | Description |
|---|---|
| input | `int32` category ID per item |
| output | `rgba_u8` color |

When a visual attribute uses a categorical scale:

1. `PER_ITEM` source: one `int32` category ID per item is uploaded,
2. `CONSTANT` source: one `int32` is supplied; one color is applied to all items,
3. `PER_GROUP` source: one `int32` per group is supplied; each group gets one mapped color.

### Updates

A categorical scale supports the same update separation as a continuous color scale:

1. **color set update** — change the named palette or custom color array; marks the scale dirty,
   does not require re-uploading item ID data.
2. **item ID update** — upload new category IDs; marks the visual's item data dirty, does not
   change the scale itself.


## Size Scale

A size scale maps scalars to float size values.

| Property | Description |
|---|---|
| `domain_min`, `domain_max` | input scalar range |
| `output_min`, `output_max` | output size range in the declared unit |
| `output_unit` | `screen_pixels` or `data_units` |
| `interpolation` | `linear` (default) or `sqrt` or `log` |

`sqrt` interpolation is useful when mapping scalar quantities to mark areas (so that perceived area
scales linearly with value).
`log` interpolation is useful for data spanning several orders of magnitude.


## Opacity Scale

An opacity scale maps scalars to alpha values in `[0, 1]`.

| Property | Description |
|---|---|
| `domain_min`, `domain_max` | input scalar range |
| `interpolation` | `linear` (default) |

The output is a `float32` alpha value clamped to `[0, 1]`.
It is typically combined with a base color by multiplying the alpha channel.


## Custom Colormap Registration

A user-defined colormap can be registered with the scene under a name, making it referenceable
by string in the same way as built-in named palettes.

```text
dvz_colormap_register(scene, "my_map", colors_rgba_u8, n)
```

`colors_rgba_u8` is a flat array of `n` RGBA `u8` values defining the palette from `t = 0`
to `t = 1`.
`n` should be at least 2; 256 is the typical resolution for a smooth continuous colormap.

Once registered, the name `"my_map"` can be used anywhere a named palette is accepted:

```text
scale = dvz_scale_color(scene, &(DvzColorScaleDesc){
    .domain_min = 0.0,
    .domain_max = 1.0,
    .palette     = "my_map",
})
```

Registration is scene-scoped — the name is valid for the lifetime of the scene.
Registering with a name that already exists replaces the previous palette and marks all
scales using that name dirty.

This is the primary path for Python users who define colormaps as NumPy arrays
(`uint8` or `float32` arrays of shape `(N, 3)` or `(N, 4)`) and want to pass them to
Datoviz by name rather than constructing a scale descriptor inline.


## Scale Identity And Sharing

All scale kinds — `color`, `categorical`, `size`, and `opacity` — have a stable logical identity
within the owning scene.

That identity allows:

1. multiple visuals to reference the same scale,
2. a colorbar or legend to attach to a scale rather than to a specific visual,
3. scale-dirty propagation: when scale parameters change, all referencing visuals and explanatory
   objects are invalidated correctly.

Two visuals that reference the same scale object share a mapping identity.
A colorbar that attaches to that scale explains both visuals simultaneously.

Two visuals that happen to use the same palette but reference different scale objects are
semantically distinct mappings.
A colorbar should not aggregate them without explicit instruction.

This rule preserves the aggregation guarantee stated in `LEGENDS_AND_COLORBARS.md`:
shared explanatory objects should only combine semantically identical mappings.


## Declaring A Scale On A Visual Attribute

When a visual attribute uses `scalar` color mode (see `visuals/PIXEL.md` and family specs), it
must reference a color or categorical scale.

The declaration binds:

1. the attribute (e.g., `color`),
2. the scale object,
3. the attribute source (`CONSTANT`, `PER_ITEM`, or `PER_GROUP`).

### Explicit Handle

The preferred form creates a named scale handle that can be shared across visuals and attached to
explanatory objects:

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

### Inline Shortcut

When a scale is used on exactly one visual and no colorbar or legend is needed, the user may
declare the mapping parameters directly on the visual without materializing a scale handle.
The scene creates an anonymous scale internally.

```text
visual_set_colormap(visual, {
    domain_min = 0.0,
    domain_max = 1.0,
    palette = VIRIDIS,
    source = PER_ITEM,
})
```

An anonymous scale has no stable identity outside the visual.
It cannot be shared with other visuals or attached to a colorbar.
If the user later needs to attach a colorbar, they should switch to an explicit handle.

**API spelling** — four typed shorthand constructors cover the common cases:

```c
/* Color scale: float input → rgba via named colormap */
DvzScale* scale = dvz_scale_color(scene, "viridis", domain_min, domain_max);

/* Categorical scale: integer input → rgba via discrete palette */
DvzScale* scale = dvz_scale_categorical(scene, "tab10", n_categories);

/* Size scale: float input → float output in [px_min, px_max] */
DvzScale* scale = dvz_scale_size(scene, px_min, px_max, domain_min, domain_max);

/* Opacity scale: float input → float output in [0, 1] */
DvzScale* scale = dvz_scale_opacity(scene, domain_min, domain_max);
```

Update functions operate on an existing handle without recreating the visual:

```c
dvz_scale_set_domain(scale, min, max);           /* update domain bounds */
dvz_scale_set_colormap(scale, "plasma");          /* change named palette */
dvz_scale_set_stops(scale, rgba_stops, n_stops); /* custom RGBA color stops */
dvz_scale_destroy(scale);
```

All four constructors return a `DvzScale*` that can be passed to `dvz_visual_set_scale`.
`dvz_scale_categorical` is a convenience wrapper: it selects the first `n_categories` colors
from the named discrete palette and maps integer item indices to those colors.
It is equivalent to calling `dvz_scale_color` with the same palette name and a domain of
`[0, n_categories - 1]`, but signals categorical intent at construction time.


## Scale Updates

Scales support the following updates without recreating the visual:

1. **domain update** — change `domain_min` and/or `domain_max`.
   Marks the scale dirty; invalidates color for all referencing visuals.
   Does not require re-uploading item scalar data.

2. **palette update** — change the palette name or custom stop array.
   Marks the scale dirty; invalidates color for all referencing visuals.
   Does not require re-uploading item scalar data.

3. **scalar data update** — the user uploads new scalar values to the visual attribute.
   Marks the visual's item data dirty; does not change the scale itself.

This separation is important: updating the colormap (scale) is a cheap scene-state change, while
updating item scalars is a data upload.
They are independent and may occur separately.


## Capability Adaptation

If a runtime cannot support GPU-side palette lookup (for example, no sampled texture support for
the palette), the scene may fall back to CPU-side colormap application at upload time.

In that case:

1. the scalar attribute is mapped to rgba on the CPU before upload,
2. the visual receives rgba data instead of scalar data,
3. a capability adaptation diagnostic is emitted,
4. dynamic domain or palette updates will require re-uploading item color data rather than just
   updating the scale.

This fallback is transparent to the user except for the diagnostic and the higher update cost.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `visuals/PIXEL.md` | first family to reference this scale model via `color_mode = scalar` |
| `LEGENDS_AND_COLORBARS.md` | colorbars attach to scale identity defined here |
| `ATTRIBUTE_SOURCES.md` | scalar attribute source feeds the scale mapping |
| `INVALIDATION_AND_CACHING.md` | scale-dirty propagation rules |
| `CAPABILITY_ADAPTATION.md` | GPU vs CPU palette lookup fallback |
