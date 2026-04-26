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

The scale normalizes input values to `[0, 1]` before palette lookup:

```
t = clamp((value - domain_min) / (domain_max - domain_min), 0, 1)
color = palette_lookup(t)
```

For `repeat` clamp mode:

```
t = fmod((value - domain_min) / (domain_max - domain_min), 1.0)
```


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

The exact API spelling is deferred to `PREFERRED_API_PROFILE.md` and the final C header work.


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


## Deferred Questions

1. the exact public API spelling for scale construction and update.

The following questions are resolved and no longer open:

- **Inline construction**: both explicit handles and an inline shortcut are supported; see
  the Declaring section above.
- **Categorical scales**: `categorical` is a first-class scale kind with integer input, not a
  palette variant of `color`.
- **`rgba_f32` output**: not needed. The output of all color and categorical scales is `rgba_u8`.
  The float32 *input* scalar is already covered by the color scale model.
- **Size and opacity construction surface**: all scale kinds share the same identity and sharing
  machinery; size and opacity scales use the same explicit-handle and inline-shortcut model as
  color scales.
