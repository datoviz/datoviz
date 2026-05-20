# Scene Attribute Sources

Status: normative v0.4 scene pipeline spec, with current first-slice API status called out.

This document defines per-attribute data multiplicity and update-frequency hints. It refines
[RESOURCE_MODEL.md](RESOURCE_MODEL.md) and
[`../semantics/VISUAL_CONTRACT.md`](../semantics/VISUAL_CONTRACT.md) at field granularity.


## Purpose

Visual attributes such as `position`, `color`, `size`, or `stroke_width` may be constant, per item,
per structural span, or per semantic group. Users declare the source and optional mutability; the
scene chooses buffers, parameter blocks, shader access, draw splitting, or expansion.


## Core Rules

1. Attribute source is semantic and user-declared, never inferred from count `1`.
2. Mutability is orthogonal to source and advisory for planning/allocation.
3. GPU layout and shader strategy are scene/backend choices.
4. Source-specific offsets are expressed in the source's semantic coordinate system.


## Layout Hints

Attribute layout hints may express producer and update behavior without exposing Vulkan bindings.

| Hint use | Example | Planning implication |
|---|---|---|
| external/frequent producer | CUDA/CuPy writes `position` every frame | keep independent external/synchronized buffer |
| static style attributes | `color` + `size` rarely change together | may interleave or coalesce |
| streaming attribute | per-frame values | ring/persistent staging if available |


## Attribute Sources

| Source | Cardinality | Valid on | Typical lowering |
|---|---|---|---|
| `CONSTANT` | one value for all items | shareable attributes | parameter block, push constant, uniform, or dense fallback |
| `PER_ITEM` | one value per item | all user data attributes unless restricted | dense item buffer or storage lookup |
| `PER_SPAN` | one value per structural span | `GroupedItemTable` visuals only | expand/index by stored span boundaries |
| `PER_GROUP` | one value per semantic group | visuals with `group_id` | draw-per-group, lookup, or expansion |

`position` is generally `PER_ITEM` only. Style attributes such as `color`, `size`, `opacity`, and
`stroke_width` usually accept multiple sources. Family specs define exact validity; when absent, the
default is `PER_ITEM`.


## Spans Versus Groups

| Term | Declared by | Meaning | Matching source |
|---|---|---|---|
| span | `span_sizes` in a `GroupedItemTable` | structural range: one path or one string | `PER_SPAN` |
| group | per-item or per-span `group_id` | semantic identity: population, region, channel | `PER_GROUP` |

Valid combinations:

| Resource/source | Requirement | Example |
|---|---|---|
| `ItemTable` + `PER_GROUP` | per-item `group_id` plus group value table | scatter spikes colored by population |
| `GroupedItemTable` + `PER_SPAN` | one value per span; no `group_id` needed | one color per path |
| `GroupedItemTable` + `PER_GROUP` | per-span `group_id` plus group value table | many paths sharing population colors |
| `GroupedItemTable` without `group_id` + `PER_GROUP` | invalid | use `PER_SPAN` or add group ids |


## Public API Status

The active first-slice API currently exposes retained dense arrays:

```c
dvz_visual_set_data(visual, attr_name, data, item_count)
dvz_visual_set_data_range(visual, attr_name, data, first_item, item_count)
```

For the active point visual, `position`, `color`, and `size` are dense per-item attributes with the
same `item_count`. There is not yet installed API for `CONSTANT`, `PER_SPAN`, or `PER_GROUP`
builtin visual attributes. The full source model below is intended API direction.


## Intended Source-Specific API

Canonical range forms:

```c
int dvz_visual_set_item_data(DvzVisual* visual, const char* attr_name,
                             const void* data, uint32_t first_item, uint32_t item_count);
int dvz_visual_set_span_data(DvzVisual* visual, const char* attr_name,
                             const void* data, uint32_t first_span, uint32_t span_count);
int dvz_visual_set_group_data(DvzVisual* visual, const char* attr_name,
                              const void* data, uint32_t first_group, uint32_t group_count);
int dvz_visual_set_value(DvzVisual* visual, const char* attr_name, const void* value);
```

| Function | Source | Offset unit |
|---|---|---|
| `dvz_visual_set_item_data` | `PER_ITEM` | item index |
| `dvz_visual_set_span_data` | `PER_SPAN` | span index |
| `dvz_visual_set_group_data` | `PER_GROUP` | group index |
| `dvz_visual_set_value` | `CONSTANT` | none |

Group topology is separate from group-valued attributes:

```c
int dvz_visual_set_group_ids(DvzVisual* visual, const uint32_t* group_ids,
                             uint32_t first_item, uint32_t item_count);
int dvz_visual_set_span_group_ids(DvzVisual* visual, const uint32_t* group_ids,
                                  uint32_t first_span, uint32_t span_count);
```


## Lowering Guidance

| Source | Preferred optimization | Required fallback |
|---|---|---|
| `CONSTANT` | parameter block, push constant, uniform, or accessor-specific variant | dense per-item expansion |
| `PER_GROUP` | draw-per-group/span when contiguous or group-table indirection when interleaved | expanded per-item buffer |
| mixed sources | bounded feature-layout keys and cached requested variants | generic shader with dummy bindings/modes |

Do not create one shader for every combinatorial attribute-source combination. Use a generic
correctness path, accessor-template/source-mode variants for common hot layouts, and cached
feature-layout keys such as `position=PER_ITEM,color=PER_ITEM,size=CONSTANT`.


## Mutability Hint

| Hint | C enum | Meaning |
|---|---|---|
| `static` | `DVZ_MUTABILITY_STATIC` | set once, not expected to change |
| `dynamic` | `DVZ_MUTABILITY_DYNAMIC` | occasional changes; default |
| `streaming` | `DVZ_MUTABILITY_STREAMING` | every-frame or nearly every-frame changes |

The hint should be set before first data write for best effect:

```c
dvz_visual_set_mutability(visual, attr_name, hint)
```

It does not determine pointer ownership. `dvz_visual_set_data()` copies by default for all levels;
borrowed/zero-copy data requires an explicit lifetime contract. When attributes in one visual have
incompatible mutability, the resource layout should be allowed to split them into separate resources.
Whole-resource mutability rules are canonical in [RESOURCE_MODEL.md](RESOURCE_MODEL.md).


## Minimal Pixel Example

| Attribute | Accepted sources | Default | Notes |
|---|---|---|---|
| `position` | `PER_ITEM` | `PER_ITEM` | position must vary by item |
| `color` | `CONSTANT`, `PER_ITEM`, `PER_GROUP` | `PER_ITEM` | group color requires `group_id` |
| `size` | visual parameter | constant | screen-pixel size for all items |

For 1M pixels, a constant RGBA color is O(1) storage and a per-item RGBA color is O(N). The user
declares the data meaning; the scene chooses the realization.
