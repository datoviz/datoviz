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
dvz_visual_set_data_range(visual, attr_name, data, first_item, item_count);
```

Range writes update a contiguous item interval for one attribute. Attribute names and element
formats must match the visual family contract.

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
