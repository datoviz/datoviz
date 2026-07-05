Execution Status:

- Status: active v0.4 RC candidate slice proposal
- Purpose: define a narrow retained visual item-range feature without adding domain-specific time, event, particle, or track semantics to Datoviz
- Scope: scene visual state, FramePlan draw contribution, point visual first, optional splat if low risk
- Non-scope: custom visual shaders, visual modifiers, semantic event/track resources, GPU compaction, sorting, indirect draw, and domain data loaders

# Retained Visual Item Ranges

## Purpose

Scientific visualization applications often prepare large flat item tables where contiguous ranges have useful runtime meaning. Examples include time-sorted event clouds, particle trails, LiDAR sweeps, progressive point-cloud reveal, spike rasters, and track samples prepared by a higher-level Python or application layer.

Datoviz should not know those domain meanings. It should provide one generic mechanism: a retained visual may render only a contiguous range of its logical items without changing or re-uploading its attribute data.

This feature is intentionally smaller than visual modifiers, custom visual shaders, or GPU-side filtering. It is a low-risk pre-RC optimization and API clarification that lets higher layers do semantic preparation while Datoviz preserves efficient retained rendering.

## Layering Rule

Domain fields such as `frame`, `time`, `speed`, `intensity`, `track_id`, `cell_id`, or `event_timestamp` do not belong in the Datoviz primitive visual C API.

The intended layering is:

```text
application / Python / GSP / VisPy2
    owns loaders, tables, domain fields, sorting, binning, units, and high-level semantics

Datoviz scene
    owns visual attributes, scene buffers, scales, item identity, item ranges, validation, and planning

DRP2 / runtime
    owns backend-agnostic resources, draw commands, barriers, and execution
```

For example, a microbubble or event-cloud producer may sort samples by frame and keep a `frame_offsets` table outside Datoviz. Each app tick it updates only the visual item range:

```text
hi = frame_offsets[current_frame]
lo = frame_offsets[max(0, current_frame - tail)]
```

Datoviz receives `lo` and `hi - lo`, not the meaning of the frame axis.

## Definitions

### Logical item count

A visual has a logical item count established by its family-valid item attributes. For a point visual, this is the number of point items. For an instanced billboard visual, this is the number of logical instances. Family specs own exact item-count rules.

### Active item range

An active item range is an optional retained visual state:

```text
[first_item, first_item + item_count)
```

Only logical items in that half-open range participate in rendering and item-level operations that depend on rendered visual contribution.

### Full range

When no active item range is set, the visual behaves as before: all valid logical items participate.

### Empty range

A range with `item_count == 0` is valid and renders nothing.

## Semantics

1. The item range is visual state, not attribute data.
2. Setting the item range must not upload or rewrite attribute buffers.
3. Setting the item range dirties the visual draw contribution and any affected pick/query/export contribution.
4. Attribute resources, scale state, and visual-family data validation remain unchanged.
5. The range is expressed in logical item indices, not bytes, vertices, or backend instances.
6. The range is contiguous in logical item space.
7. Invalid ranges are validation errors.
8. Clearing the range restores full visual participation.
9. Picking and selection identity use global logical item indices, not local range-relative indices.
10. A visual attached to multiple panels uses the same visual-local item range unless a later spec defines panel-local draw filtering.

## Proposed C API Shape

The first public API should stay small and FFI friendly:

```c
typedef struct DvzItemRange
{
    uint32_t first_item;
    uint32_t item_count;
} DvzItemRange;

int dvz_visual_set_item_range(DvzVisual* visual, uint32_t first_item, uint32_t item_count);
DvzResult dvz_visual_clear_item_range(DvzVisual* visual);
bool dvz_visual_get_item_range(const DvzVisual* visual, DvzItemRange* out);
```

Return policy:

- `dvz_visual_set_item_range()` returns `0` on success and `-1` on validation error.
- `dvz_visual_clear_item_range()` is idempotent.
- `dvz_visual_get_item_range()` returns `false` if `visual` or `out` is invalid. When no active range is set, it should either return the effective full range or expose an explicit `active` bit in a later growable result struct. The first API should choose one behavior and document it in the installed header.

Open API detail before implementation: whether `DvzItemRange` needs a `flags` or `active` field. If the first getter returns the effective full range, no active bit is required.

## FramePlan and DRP2 Lowering

The first slice should not require a new DRP2 command.

Active DRP2 draw commands already carry the required offset/count fields:

```text
Draw:        first_vertex, vertex_count, first_instance, instance_count
DrawIndexed: first_index, index_count, base_vertex, first_instance, instance_count
```

Scene lowering maps logical item ranges to the correct backend draw parameters for each family.

For a point-list path:

```text
first_vertex = first_item
vertex_count = item_count
```

For an instanced billboard path:

```text
first_instance = first_item
instance_count = item_count
```

For indexed or expanded families, the family spec must define whether contiguous logical item ranges lower directly, require span/range conversion, or are unsupported until a later slice.

## Picking, Queries, and Selection

Item range affects rendered participation. Items outside the active range are not pickable through rendered visual picking because they are not drawn.

The reported item identity must remain the logical item index in the visual, not a local range-relative index.

Implementation options:

1. Use backend first-instance or first-vertex semantics and ensure the shader or query payload still produces the global item id.
2. Add a small base-item value to the relevant pick path if a backend lowering produces local indices.
3. Temporarily disable or explicitly diagnostic unsupported pick paths for a family until identity is correct.

Silent local-index picking is not acceptable.

## Initial Visual Family Scope

### Point

Point is the RC target. It is the simplest and highest-payoff use case for time-sorted flat item tables.

The first implementation should cover:

1. render range;
2. empty range;
3. clear range;
4. range updates across repeated frames;
5. picking identity if point picking is enabled in the selected backend path, or an explicit diagnostic if not yet supported.

### Splat

Splat may be included only if it remains low risk. If splat uses an instanced billboard path, item range should map naturally to `first_instance` and `instance_count`, but picking and transparent-pass behavior still need validation.

If splat support would delay RC stabilization, document it as deferred.

### Other families

Pixel, marker, sphere, segment, path, mesh, image, volume, glyph, and semantic composites are outside the pre-RC item-range slice unless a family already shares the exact point-like path and tests are trivial.

## Interaction with Attribute Sources

Item range is orthogonal to attribute source.

A visual may have:

```text
position: PER_ITEM
color:    CONSTANT, PER_ITEM, or PER_GROUP
size:     CONSTANT, PER_ITEM, or PER_GROUP
range:    [first_item, item_count]
```

The active range does not change the source mode or item count of any attribute. It only changes which logical items contribute to current draw/query/export work.

## Interaction with Scene Buffers

Item ranges complement future shared attribute buffers.

A higher layer may upload one static or dynamic scene buffer and bind the same attribute storage to multiple visuals. Each visual may then expose a different item range and style.

Example use pattern:

```text
one shared item table
    -> accumulated visual draws [0, hi)
    -> front visual draws [lo, hi)
```

This is a visual state and planning feature, not a domain-specific temporal API.

## Non-Goals

This proposal does not add:

1. semantic time, speed, intensity, or track APIs;
2. event, particle, or trackset data objects;
3. shader-side scalar predicates;
4. temporal reveal modifiers;
5. per-item visibility masks;
6. non-contiguous index-list filtering;
7. GPU stream compaction;
8. GPU sorting or binning;
9. indirect draw;
10. custom visual shader replacement.

Those remain separate v0.5+ or later topics.

## Relationship to Future Work

Visual item ranges are the simple contiguous-range case. They should coexist with later mechanisms:

- scalar color/size/opacity mappings through scene scales;
- shared scene buffers and external producers;
- visual modifiers for shader-side predicates and style transforms;
- GPU compaction and indirect draw for large arbitrary filters;
- custom visual families for genuinely new primitives.

Item ranges should not block those future features, but they should also not wait for them.

## Implementation Order

1. Add retained visual state for optional item range.
2. Add validation against the visual logical item count.
3. Mark draw/query/export contribution dirty when the range changes.
4. Lower point item range to DRP2 draw offset/count fields.
5. Preserve global item identity in picking or emit explicit diagnostics for unsupported paths.
6. Add focused scene tests or fixtures for repeated range changes and empty/full ranges.
7. Add one small feature example or adapt an existing retained update/visibility example only if needed for release proof.
8. Keep broader visual families deferred unless low-risk tests are available.

## Acceptance Criteria for v0.4 RC

The slice is RC-ready only if:

1. point visual range rendering is validated in the native scene path;
2. repeated frame updates do not re-upload unchanged attribute data unnecessarily;
3. empty and cleared ranges are covered;
4. invalid ranges produce deterministic validation errors;
5. picking identity is either correct or explicitly diagnosed as unsupported for the affected path;
6. docs and feature/status wording classify the feature as narrow retained visual item ranges, not a general temporal/filtering system.

## Documentation Guidance

Examples should describe item ranges generically:

```text
Render items [first_item, first_item + item_count) of a retained visual.
```

Avoid examples that imply Datoviz owns semantic time or event models. Higher-level docs may explain how GSP/VisPy2 or Python data-preparation layers can sort and bin data before driving item ranges.
