# Scene Attribute Sources

This document defines how individual visual attributes declare their data multiplicity and update
frequency.

It refines `RESOURCE_MODEL.md` and `../semantics/VISUAL_CONTRACT.md` at a finer granularity: individual fields
within a visual's data rather than whole resources.


## Purpose

A visual attribute such as `color`, `size`, or `position` may be supplied at different levels of
detail:

1. one value shared by all items,
2. one value per item,
3. one value per logical group, shared across all items in that group.

The choice affects memory, upload cost, and GPU access pattern.
It is also a natural way for users to express what they actually know about their data — not a
low-level optimization decision.

This document names these choices, defines their semantics, and adds an orthogonal update-frequency
hint that gives the scene layer additional planning information.


## Core Rule

Attribute source and mutability are declared by the user.

The GPU implementation strategy — which buffer type, whether to use multiple draw calls, how to
encode group lookups — is chosen by the scene layer and is not user-visible.


## Attribute Source

Every visual attribute that accepts data from the user must declare an **attribute source**.

The four valid sources are:

1. `CONSTANT`
2. `PER_ITEM`
3. `PER_SPAN`
4. `PER_GROUP`


### `CONSTANT`

One value is supplied for all items.

The value may still change across frames, but every item sees the same value at any given time.

Examples:

1. all pixels in a scatter plot are the same red color,
2. all segments share a fixed linewidth,
3. one opacity value applies to all markers in a visual.

Scene implementation: typically a push constant, uniform binding, or small parameter block.
The scene layer chooses the mapping; the user does not.


### `PER_ITEM`

One value is supplied for each item, in item order.

This is the most expressive source and the most memory-intensive for large item counts.

Examples:

1. each point has its own position,
2. each marker has its own size and angle,
3. each glyph instance has its own color.

Scene implementation: a dense per-item buffer, typically realized as a vertex attribute or storage
buffer read.
The choice depends on item count, access pattern, and capability, and is transparent to the user.


### `PER_SPAN`

One value is supplied per structural span.

A span is a contiguous range of items that forms one logical unit in a span-structured visual:
one path in `path`, one string in `glyph`.
Span membership is encoded in the visual's span-boundary metadata (`span_sizes` attribute), not
in a per-item group index.
`PER_SPAN` is only valid for `GroupedItemTable` visuals; it is a validation error on flat
`ItemTable` visuals.

Examples:

1. 20 paths each with its own color — one `rgba_u8` per path,
2. 200 paths each with its own linewidth — one `float32` per path,
3. 50 glyph strings each with its own baseline color — one `rgba_u8` per string.

Scene implementation: the scene resolves span membership from the stored span boundaries and
expands or indexes to the correct per-item value at upload or shader time.

`PER_SPAN` is the correct source for attributes that "vary by path" or "vary by string".
`PER_GROUP` (below) should not be used for this structural purpose.


### `PER_GROUP`

One value is supplied per logical group.
Each item carries or implies a group identity, and all items in the same group share the group's
attribute value.

Examples:

1. 3 neuron populations, 1M spikes each, one color per population,
2. 200 electrode channels, each channel a different linewidth,
3. K brain-atlas regions rendered together, each region a distinct opacity.

User declaration requirements depend on the resource class of the visual:

**`ItemTable` visuals** (`point`, `pixel`, `marker`, `segment`, etc.): there are no intrinsic group
boundaries, so each item must carry an explicit per-item group index integer.
The group value table is a separate small resource.

**`GroupedItemTable` visuals** (`path`, `glyph`, etc.): structural span boundaries are encoded in
the table metadata, but semantic group identity is separate. A span-structured visual uses a
`"group_id"` array of length `span_count` when `PER_GROUP` values should be shared by several
spans. The group value table is still a separate small resource.

Scene implementation: the scene layer may choose between multiple draw calls, a storage buffer
lookup keyed by group index, or a per-item expanded copy.
The user does not select or see the strategy.

`PER_GROUP` is especially important for scientific visualization patterns such as:

1. spike rasters with per-neuron color or size,
2. atlas meshes with per-region style state,
3. multi-channel waveform or wiggle displays with per-channel color,
4. path bundles with per-path linewidth or opacity.


## Relationship To `GroupedItemTable`

`PER_GROUP` attribute source and `GroupedItemTable` are related but distinct concepts.

**Terminology**: "span" and "group" are distinct concepts that must not be confused.

- **Span** — a contiguous structural boundary unit in a `GroupedItemTable`: one polyline in
  `path`, one string in `glyph`.
  Spans define the topological layout of the data.
  The public API uses the `"span_sizes"` attribute to declare them.
  Attributes that vary by span use **`PER_SPAN`** source.

- **Group** — a semantic population identity: one neuron population, one brain region, one
  electrode channel.
  Flat visuals declare groups via a per-item integer `"group_id"` attribute. Span-structured
  visuals declare groups via a per-span integer `"group_id"` attribute.
  Attributes that vary by semantic group use **`PER_GROUP`** source.

`GroupedItemTable` is a scene resource class that owns both item storage and span boundary
metadata.
It is primarily used by visual types such as `path` and `glyph` where span membership is
intrinsic to the data contract.

The valid combinations are:

1. **`ItemTable` + `PER_GROUP`**: the visual has no intrinsic span structure; each item must
   carry an explicit per-item group index integer alongside the group value table.
   Example: a flat scatter of 3M spikes where each spike carries a neuron group index.

2. **`GroupedItemTable` + `PER_SPAN`**: span membership is already encoded in the table
   boundaries; the user supplies one value per span.
   Example: 200 paths each with its own color or linewidth.

3. **`GroupedItemTable` + `PER_GROUP`**: spans have a semantic group identity beyond the span
   itself; each span carries a group index, and the attribute varies per group.
   Example: 200 paths where paths belong to 5 neuron populations with per-population color.

4. **`GroupedItemTable` without `group_id` + `PER_GROUP`**: not supported — use `PER_SPAN`
   when one value per span is intended, or provide per-span `group_id` when several spans share
   semantic group values.


## Which Sources Each Attribute Accepts

Not every attribute accepts all three sources.

Each family spec defines, for each attribute, which sources are valid.

General rules:

1. `position` is always `PER_ITEM` — a single position for all items is not meaningful.
2. style attributes such as `color`, `size`, `opacity`, `linewidth` typically accept all three
   sources.
3. orientation or shape attributes such as `angle` or `marker_shape` typically accept `CONSTANT`
   or `PER_ITEM`.
4. group identity itself is always `PER_ITEM` — it is a per-item integer index.

When a family spec does not list a source for an attribute, the default is `PER_ITEM`.


## Source Selection In The Public API

### Active first-slice API status

The active v0.4 first-slice C API does not yet implement the full source model described in this
document.

Today, builtin scene visuals expose retained dense attribute arrays through:

```c
dvz_visual_set_data(visual, attr_name, data, item_count)
dvz_visual_set_data_range(visual, attr_name, data, first_item, item_count)
```

The API exposes item counts and, for range updates, an item index (`first_item`).
It does not expose a byte offset, a binding offset, or a source-layout descriptor.

For the active point visual, `position`, `color`, and `size` are all per-item attributes and must use
the same `item_count`.
The scene stores `item_count * item_size` bytes for each attribute, emits one buffer upload per dirty
attribute range, and the GLSL point shader reads `size` from a vertex input.

There is currently no active public API for declaring that a builtin visual attribute is:

1. one constant value shared by the whole visual,
2. one value per semantic group,
3. one value per structural span.

The source model below is the intended API direction, not the current implementation contract.

### Intended source-specific range API

Attribute source should be explicit in the public API.
It should not be inferred from a count value because `1` is ambiguous: it may mean one item, one
span, one group, or one constant value.

The intended API shape is:

```c
int dvz_visual_set_item_data(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_item, uint32_t item_count);

int dvz_visual_set_span_data(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_span, uint32_t span_count);

int dvz_visual_set_group_data(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_group, uint32_t group_count);

int dvz_visual_set_value(
    DvzVisual* visual, const char* attr_name, const void* value);
```

For ranged sources, the offset is always expressed in the source's semantic coordinate system:

| Function | Source | Offset unit |
|---|---|---|
| `dvz_visual_set_item_data` | `PER_ITEM` | item index |
| `dvz_visual_set_span_data` | `PER_SPAN` | span index |
| `dvz_visual_set_group_data` | `PER_GROUP` | group index |
| `dvz_visual_set_value` | `CONSTANT` | none; exactly one value |

Full replacement is represented by `first_* = 0` and `*_count` equal to the current source count.
Separate no-offset convenience wrappers may exist, but the range form should be the canonical API so
partial updates are available uniformly.

Group membership is separate from group-valued attributes because several attributes may share the
same group topology:

```c
int dvz_visual_set_group_ids(
    DvzVisual* visual, const uint32_t* group_ids,
    uint32_t first_item, uint32_t item_count);

int dvz_visual_set_span_group_ids(
    DvzVisual* visual, const uint32_t* group_ids,
    uint32_t first_span, uint32_t span_count);
```

Flat `ItemTable` visuals use per-item group ids.
Span-structured `GroupedItemTable` visuals use per-span group ids when semantic groups are shared by
several spans.

The scene validates that the selected source is accepted by the visual family and attribute.
For example, `position` generally accepts only `PER_ITEM`, while style attributes such as `color`,
`size`, and `linewidth` may accept `CONSTANT`, `PER_ITEM`, `PER_SPAN`, or `PER_GROUP` depending on
the visual family.


## Implementation Avenue: Optimizing `CONSTANT` Sources

`CONSTANT` sources are important because they let the scene avoid expanding one value to every item.
For example, a point visual with one shared point size should not allocate and upload an
`N * sizeof(float)` size buffer solely because the shader can currently read only a vertex attribute.

The preferred implementation direction is:

1. keep the public API semantic (`size` is a visual attribute whose source is `CONSTANT`),
2. lower that source to a visual parameter block, push constant, or uniform buffer when practical,
3. emit a shader/pipeline layout that reads the value from that constant source,
4. fall back to dense per-item expansion only when the runtime or visual family cannot support the
   optimized path.

This means the optimization belongs in the scene-to-DRP2 lowering layer, not in user code.
Users should declare the data multiplicity; the renderer should choose the storage and shader access
strategy.

### Shader interface options

Vulkan shader interfaces are mostly fixed at pipeline creation time.
Switching an attribute between vertex input, uniform, and group lookup therefore cannot be a purely
draw-time decision unless the selected pipeline already declares all required inputs.

Valid implementation strategies are:

1. **Generic fallback shader**: declare all supported sources, use a small mode field, and bind dummy
   resources for unused paths. This avoids shader permutation growth but keeps extra interface
   declarations and may leave runtime branches or indirect loads.
2. **Accessor-template variants**: keep visual logic in one shared shader body and generate only the
   source-specific accessor and layout declarations, for example `get_size()` reading either
   `inSize` or `params.size`. This avoids hand-written shader duplication while allowing unused
   inputs to disappear from the pipeline layout.
3. **Specialization constants**: specialize source modes at pipeline creation so branches can be
   constant-folded. This is useful for reducing branch cost, but still creates pipeline variants and
   should not be treated as a draw-time switch.
4. **Dense expansion fallback**: expand one constant into a per-item buffer when the optimized path is
   unavailable. This preserves correctness but loses the memory/upload advantage.

Dynamic SPIR-V editing is technically possible but should not be the first implementation path.
Changing shader interfaces directly in SPIR-V turns this into a compiler-backend problem and is harder
to validate and debug than generated GLSL accessors or bounded pipeline variants.

### Avoiding combinatorial shader variants

The scene layer should not create one shader for every combination of attribute sources.
For a visual with many attributes, `attribute|constant|group` choices quickly become a combinatorial
space.

The intended approach is a bounded hybrid:

1. provide one generic fallback path for correctness and uncommon combinations,
2. define a feature-layout key from source modes, for example
   `position=PER_ITEM,color=CONSTANT,size=CONSTANT`,
3. cache only the pipeline variants that are actually requested,
4. add hand-picked optimized variants for common hot layouts,
5. share the shader body and generate only small source accessor blocks.

For point visuals, a useful first optimized key would be:

```text
position=PER_ITEM
color=PER_ITEM
size=CONSTANT
```

The optimized vertex shader would still read position and color as vertex inputs, but would read size
from a small parameter block instead of binding a size vertex buffer.

### `PER_GROUP` lowering

`PER_GROUP` sources have two main GPU realizations:

1. **draw-per-group or draw-per-span**: sort or batch items by group and bind one constant parameter
   block per draw. This avoids per-item group lookup but increases draw count.
2. **group indirection**: store a per-item or per-span `group_id` plus a compact group-value table,
   then look up `group_values[group_id]` in the shader. This preserves one draw for interleaved groups
   but adds an index buffer/load.

The scene should choose between these strategies from item count, group count, update frequency,
whether data is already grouped contiguously, and runtime capability.


## Mutability Hint

Orthogonal to source, each attribute may carry an optional **mutability hint**.

The hint tells the scene layer how often the user expects to update the value.

The three hint levels are:

1. `static` — the value is set once and not expected to change.
2. `dynamic` — the value changes occasionally, for example in response to user interaction.
3. `streaming` — the value changes every frame or nearly every frame.

The mutability hint does not determine pointer ownership. `dvz_visual_set_data()` copies by default
for all mutability levels. Zero-copy borrowed data requires a separate explicit API or flag with a
clear caller lifetime contract. See `RESOURCE_MODEL.md` — "Data Ownership And Memory Model" — for
the normative ownership rules.

The hint is set via a separate call:

```c
dvz_visual_set_mutability(visual, attr_name, hint)
```

where `hint` is one of `DVZ_MUTABILITY_STATIC`, `DVZ_MUTABILITY_DYNAMIC` (default), or
`DVZ_MUTABILITY_STREAMING`. The call is optional and should be made before the first
`dvz_visual_set_data` call for best effect.

The hint is optional.
The default when absent is `dynamic`, which is safe but not maximally optimized.

Choosing the right hint allows the scene layer to select better GPU allocation strategies:

1. `static` attributes may be placed in immutable GPU buffers or coalesced for better cache use.
2. `dynamic` attributes may use staging-copy paths with change detection.
3. `streaming` attributes may use ring buffers or persistent mapped memory when the runtime
   supports it.

The mutability hint is advisory.
The scene layer is not required to implement a different code path for each level.
It is allowed to treat `dynamic` and `static` identically if implementation cost outweighs the
benefit.


## Relationship To Resource-Level Mutability

`RESOURCE_MODEL.md` already defines mutability classes at the whole-resource level:

1. immutable asset,
2. infrequently updated parameter block,
3. per-frame dynamic stream,
4. transient per-plan derived resource,
5. readback-only sink.

Attribute-level mutability hints and resource-level mutability classes are compatible but operate at
different scopes.

When an `ItemTable` contains both static positions and streaming colors, the preferred approach is to
place them in separate resources so that each resource can carry the appropriate resource-level
mutability class.

The attribute source model should therefore inform resource layout decisions.
Family specs and the scene API may use attribute mutability hints to suggest or enforce that static
and streaming attributes live in separate resources.


## Example: Pixel Visual

The `pixel` visual has two per-item data attributes:

| Attribute | Accepted sources | Default source | Notes |
|---|---|---|---|
| `position` | `PER_ITEM` only | `PER_ITEM` | position must vary by item |
| `color` | `CONSTANT`, `PER_ITEM`, `PER_GROUP` | `PER_ITEM` | all three are valid |

Visual-wide parameter (not an item attribute):

| Parameter | Description |
|---|---|
| `size` | pixel size in screen pixels, applies to all items |

A user wanting 3 populations of 1M pixels each with per-population colors would declare:

1. `position` as `PER_ITEM`,
2. `color` as `PER_GROUP` with a 3-entry color table and a per-item group index.

The scene layer chooses how to realize this in DRP2 — for example as three draw calls, or as a
storage buffer with a group index vertex attribute — without the user specifying either.


## Example: Color As `CONSTANT` Versus `PER_ITEM`

When `color` is `CONSTANT`:

1. the user supplies one rgba value,
2. memory cost is O(1),
3. updating the color is a small uniform write.

When `color` is `PER_ITEM`:

1. the user supplies N rgba values,
2. memory cost is O(N),
3. updating one item's color is a subrange write to the item buffer.

For 1M pixels, `CONSTANT` color uses 4 bytes total.
`PER_ITEM` color uses 4 MB.
The user should choose based on what their data actually requires.
