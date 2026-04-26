# Scene Attribute Sources

This document defines how individual visual attributes declare their data multiplicity and update
frequency.

It refines `RESOURCE_MODEL.md` and `VISUAL_CONTRACT.md` at a finer granularity: individual fields
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

The three valid sources are:

1. `CONSTANT`
2. `PER_ITEM`
3. `PER_GROUP`


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

**`GroupedItemTable` visuals** (`path`, `glyph`, etc.): group boundaries are already encoded in the
table metadata.
`PER_GROUP` source on these visuals infers group membership from those boundaries — no separate
per-item group index attribute is needed.
The group value table is still a separate small resource.

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

`GroupedItemTable` is a scene resource class that owns both item storage and group boundary metadata.
It is primarily used by families such as `path` and `glyph` where group membership is intrinsic to
the family contract (items belong to an ordered sequence).

`PER_GROUP` attribute source is an attribute-level declaration.
It says that a specific attribute value varies by group, not by item.

The two concepts interact as follows:

1. **`ItemTable` + `PER_GROUP`**: the visual has no intrinsic group structure, so the user must
   supply an explicit per-item group index integer alongside the group value table.
   Example: a flat scatter of 3M spikes where each spike carries a neuron group index.

2. **`GroupedItemTable` + `PER_GROUP`**: group membership is already encoded in the table
   boundaries, so no separate per-item group index is needed.
   The scene infers group identity from the boundary metadata.
   Example: 200 paths each with its own color — the path boundaries define the groups.

Group identity in `GroupedItemTable` visuals is always the table's own group structure.
There is no supported case where a `GroupedItemTable` visual uses `PER_GROUP` with a group
identity that differs from the table's boundaries.


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


## Mutability Hint

Orthogonal to source, each attribute may carry an optional **mutability hint**.

The hint tells the scene layer how often the user expects to update the value.

The three hint levels are:

1. `static` — the value is set once and not expected to change.
2. `dynamic` — the value changes occasionally, for example in response to user interaction.
3. `streaming` — the value changes every frame or nearly every frame.

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


## Deferred Questions

1. the exact public API shape for declaring attribute sources and mutability hints — deferred to
   the broader C API surface work in `PREFERRED_API_PROFILE.md`.

The following questions are resolved:

- **Group identity and range inference**: `ItemTable` visuals require an explicit per-item group
  index integer; `GroupedItemTable` visuals infer group membership from table boundaries with no
  separate index needed.
- **`PER_GROUP` reuse of `GroupedItemTable` boundaries**: confirmed — see the `Relationship To
  GroupedItemTable` section above.
- **Group count threshold for storage buffer vs draw calls**: an internal scene layer
  implementation detail, not user-visible, not specced.
