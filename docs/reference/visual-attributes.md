# Visual Attributes

Visual attributes are named data inputs consumed by retained visual families: positions, colors,
sizes, radii, texture coordinates, normals, indices, transforms, and family-specific style data.

This page describes the public data model. See each [visual family](visual-families/index.md) for
family-specific required and optional attributes.

## Attribute Concepts

| Concept | Meaning |
| --- | --- |
| Attribute name | Stable string such as `position`, `color`, `diameter_px`, `radius`, `normal`, or `texcoords`. |
| Format | Element type and shape expected by the family, such as `vec3`, RGBA8, scalar float, or `mat4`. |
| Source | Semantic multiplicity: constant, per item, per span, or per group. |
| Mutability | Planning hint: static, dynamic, or streaming. |
| Resource role | How the attribute lowers: vertex input, index input, sampled field, storage buffer, parameter block, or readback target. |

## Current Public Write Path

The active release surface centers on dense retained writes:

```c
dvz_visual_set_data(visual, attr_name, data, item_count);
dvz_visual_set_data_range(visual, attr_name, first_item, data, item_count);
```

Range writes update a contiguous item interval for one attribute. Attribute names and element
formats must match the visual family contract.

## Array Shape And Storage

The first array dimension is the logical item count. Component dimensions describe one item: for
example, positions are `float32` with shape `(N, 3)`, RGBA colors are `uint8` with shape `(N, 4)`,
and scalar sizes are `float32` with shape `(N,)`. The exact contract is family-specific; never infer
it from the attribute name alone.

| Rule | C | Python NumPy facade |
| --- | --- | --- |
| Storage | Tightly packed array of the documented C element type. | Array with the documented dtype and shape. |
| Contiguity | Caller provides contiguous packed elements. | Non-contiguous arrays are copied to C-contiguous temporary storage. |
| Cardinality | `item_count` is explicit. | Inferred from `array.shape[0]`; a raw pointer still requires an explicit count. |
| Ownership | Dense setter copies before returning. | The facade keeps temporary arrays alive for the call; retained C storage owns the copy afterward. |
| Batch update | Every `DvzVisualDataUpdate` uses the same count. | A mapping or iterable of `(attribute, array)` pairs must have matching first dimensions. |

The facade validates dtype only for functions whose generated binding policy declares one. Treat
the family tables—not automatic conversion—as authoritative. Prefer explicit `np.asarray(...,
dtype=...)` and validate shapes before crossing the API boundary.

All configured dense per-item attributes on one visual must have the same logical count. When the
count changes, use `dvz_visual_set_data_many()` so validation is atomic. Include every existing
dense attribute whose old count would otherwise disagree. Use `dvz_visual_set_data_range()` only
after a full allocation and keep the update inside that allocation.

## Finite Values And Missing Data

There is no single missing-data convention across all visual families. Follow the narrowest
documented contract:

- Geometry, transforms, sizes, widths, radii, and extents should be finite. Invalid values may be
  rejected, produce incomplete bounds, or lead to backend-dependent geometry.
- Scalar point and pixel color attributes currently map non-finite values to transparent RGBA when
  the frame upload is derived. This is a specific scalar-color route, not a global mask policy.
- Plot helpers that explicitly document `NaN` separators may split paths or filled regions at those
  values.
- Sampled image and volume fields do not define a general mask or bad-color policy. Preprocess
  missing voxels/pixels or encode a deliberate alpha channel before upload.

Do not use a sentinel numeric value unless the consuming family documents it. For reproducible
scientific figures, decide missing-value handling before computing bounds, normalization, scales,
and colorbars.

## Numeric And Color Conventions

- Floating-point visual attributes use single-precision storage unless the family page explicitly
  says otherwise. Domains, tick values, and scale metadata may use `double` independently of GPU
  attribute storage.
- Integer identifiers and item-state bitfields retain their documented signedness and width. Do not
  pass platform-default Python integers as untyped array storage.
- `DvzColor` and `uint8 (N, 4)` arrays are RGBA8 display colors: sRGB-encoded RGB with straight
  alpha. They are not linear floating-point radiometric values.
- Angles are radians. A family page states the positive direction and coordinate convention when
  rotation is exposed.
- Names ending in `_px` use logical pixels. Framebuffer capture dimensions and pixel arrays use
  physical framebuffer pixels.


## Screen-Space Attributes

Attributes with a `_px` suffix are authored in logical pixels unless a visual-family page states a
narrower rule. They are display/style quantities: controllers move or transform item positions, but
do not scale the on-screen diameter or stroke width. The runtime converts logical pixels to
framebuffer pixels using the view/device scale when emitting draw commands.

| Attribute or setting | Families | Meaning | Coordinate effect |
| --- | --- | --- | --- |
| `diameter_px` | point, marker | Screen-space item diameter in logical pixels. | Center follows data/controller transform; diameter stays screen-stable. |
| `pixel_size_px` | pixel | Square mark side length in logical pixels. | Center follows data/controller transform; size stays screen-stable. |
| `stroke_width_px` | segment, path, vector, point edge style | Screen-space stroke width in logical pixels. | Endpoints/path points follow data/controller transform; stroke width stays screen-stable. |
| Text style size | semantic text/glyph lowering | Glyph size in logical pixels for the selected text renderer. | Placement follows the text placement contract; glyph size is screen-stable. |
| Panel padding/reserve | panels, axes, colorbars, legends | Layout bands in logical figure pixels. | Converted to viewport/scissor rectangles during frame planning. |
| Image `extent` | image/labels retained item placement | Data or panel-space rectangle by attachment/visual contract, not a `_px` size. | Follows data/controller transform unless attached in a fixed coordinate space. |

Do not infer pixel semantics from an attribute's numeric dtype alone. Use the attribute name and the
visual-family contract. For example, image `extent` is placement geometry, while `diameter_px` is a
screen-space style size.

## Sources

The scene model distinguishes these source kinds:

| Source | Cardinality | Typical use | Public status |
| --- | --- | --- | --- |
| Constant | One value for all items. | Shared color, opacity, size, material parameter. | Directional model; use family setters or dense data where no source API exists. |
| Per item | One value per logical item. | Point positions, marker sizes, segment endpoints. | Active dense data path. |
| Per span | One value per structural span. | One style per path or text span. | Family-specific/limited. |
| Per group | One value per semantic group. | Population, region, channel, category styling. | Directional model unless a family documents support. |

Do not infer source from count `1`. A one-item data upload is still per-item unless an API declares
constant source semantics.

## Mutability

| Hint | Meaning |
| --- | --- |
| Static | Set once, not expected to change. |
| Dynamic | Occasional changes; default. |
| Streaming | Every-frame or nearly every-frame changes. |

Mutability is a planning/allocation hint, not a pointer ownership rule. Ordinary data writes copy
caller memory unless an API explicitly documents borrowed or external-buffer behavior.

## External And GPU-Produced Data

External buffers and scene compute output are explicit resource paths. When a compute pass writes a
buffer that a visual later consumes as an attribute, the frame plan must order compute before render
and emit the required barrier.

The visual family still owns semantic validation: the bound buffer must provide attributes that the
family supports, with compatible format, stride, offset, and count.

## Update Rules

| Update | Expected behavior |
| --- | --- |
| Same count, changed data | Mark attribute/content dirty; avoid full topology rebuild when possible. |
| Changed item count | Resize logical visual payload and invalidate affected draw planning. |
| Changed format/source/mutability | Revalidate family contract and likely rebuild planning resources. |
| Sampled field content update | Mark field content dirty; visuals referencing the field observe the update. |
| Style setter update | Mark visual properties dirty; only rebuild frame plan if draw participation or variant changes. |

## Limitations

- Attribute names are family-specific; unsupported names should fail validation.
- Missing required attributes make the visual incomplete for rendering.
- Optional attributes may use family defaults.
- Backend buffer layout is not a public contract.
- Custom render shader replacement for built-in visuals is deferred in v0.4.

## See Also

- [Visual families](visual-families/index.md)
- [Feature status](feature-status.md)
- [Compute and graphics](compute-graphics.md)
- [C visual API](c-api/visuals.md)
