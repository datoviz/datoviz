# Visual Family: `primitive`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`primitive` visual family.

It refines:

1. `VISUAL_FAMILIES.md` — family taxonomy and rationale
2. `VISUAL_MINI_CONTRACTS.md` — family-level mini-contract (formerly called `basic`)
3. `ATTRIBUTE_SOURCES.md` — attribute granularity and mutability vocabulary
4. `VISUAL_CONTRACT.md` — shared visual responsibilities

> **v0.3 note**: this family was called `basic` in v0.3. The rename reflects its actual role as a
> raw GPU primitive surface rather than a beginner-friendly entry point.


## Semantic Purpose

`primitive` is a first-class low-level visual family for explicit topology-driven rendering.

It exposes GPU primitive topologies directly — point lists, line strips, triangle lists — with
minimal scene-level abstraction between the user's data and the draw call.

It exists to:

1. provide a durable low-level escape hatch when higher-level families do not fit,
2. support experimentation and renderer bring-up,
3. serve as a baseline pressure test for the visual contract.

`primitive` should stay intentionally constrained.
If a use case grows richer semantic behavior such as joins, caps, shapes, or per-item size, that is
pressure toward `path`, `segment`, `marker`, or another family — not toward expanding `primitive`.


## Topology

Topology is declared at visual creation time and cannot change without recreating the visual.

Supported topologies:

| Topology | Description |
|---|---|
| `point_list` | one point per vertex |
| `line_list` | independent line segments, two vertices each |
| `line_strip` | connected line sequence, one segment per adjacent pair |
| `triangle_list` | independent triangles, three vertices each |
| `triangle_strip` | connected triangle strip |
| `triangle_fan` | triangle fan from a fixed first vertex |

`primitive` does not support indexed rendering.
For indexed geometry, use the `mesh` family.


## Per-Item Attribute Schema

Each item in a `primitive` visual is one vertex.

### `position`

| Property | Value |
|---|---|
| Type | `vec3` — three `float32` values |
| Interpretation | `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |

Position is always per-vertex.
`z` participates in depth ordering.


### `color`

| Property | Value |
|---|---|
| Type | `rgba_u8` (direct) or `scalar_f32` (mapped) — see color mode below |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` or `streaming` |

Two color modes follow the pattern established in `visuals/PIXEL.md`:

**`rgba` mode** (default): 4-byte RGBA color per vertex (or constant, or per-group).

**`scalar` mode**: one `float32` per vertex (or constant, or per-group), mapped through an
associated `Scale` object.
See `SCALES.md` for the scale contract.

In v0.3, per-group color variation was typically implemented by computing colors on the CPU from a
group index and passing the result as per-vertex rgba.
In v0.4, the same intent is expressed by setting `color` source to `PER_GROUP` with a group color
table, which the scene layer maps transparently.


## Visual-Wide Parameters

### `size`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | screen pixels |
| Default | implementation-defined, suggested 1.0 |
| Mutability | `dynamic` |
| Applies to | `point_list` topology only |

Uniform point size for all vertices when topology is `point_list`.
Ignored for line and triangle topologies.

For line topologies, line width is not controlled through `primitive`.
If per-line width or cap control is needed, use `segment` or `path`.


## Color Mode Variant Axis

Identical to `pixel`:

1. `rgba` — direct 4-byte RGBA per vertex.
2. `scalar` — one `float32` per vertex mapped through a scale.

Default is `rgba`.

This is the only variant axis for `primitive`.
The family does not expose shape, join, or edge variants.


## Transform Model

`primitive` uses the standard two-stage scene transform:

1. **Normalization** — data-space positions normalized to visual space before upload.
2. **Panel transform** — panel-local camera or panzoom applied per-frame without re-upload.

`primitive` does not support a visual-local transform matrix.
If a per-visual affine transform is needed, apply it during normalization.


## Stage Participation

| Stage | Participation |
|---|---|
| Render | required |
| Compute | none |
| Picking | optional, simple |
| Offscreen / export | same as render |

Picking is optional.
When enabled, the pick result returns the vertex index as item identity.
No sub-item or group identity is defined.


## Picking Model

When picking is enabled:

1. each vertex is a pickable item,
2. a pick result returns `(panel_id, visual_id, vertex_index)`,
3. the user is responsible for interpreting vertex index in terms of their own data structure
   (e.g., which triangle or line segment a vertex belongs to).

`primitive` intentionally does not add semantic picking on top of raw vertex identity.
For family-level picking semantics, use `mesh`, `segment`, or `path`.


## Fallback Notes

`primitive` is already a minimal family.
Capability fallback pressure should be very low.

The `scalar` color mode fallback is the same as for `pixel`: if GPU-side palette lookup is
unavailable, the scene applies the colormap on the CPU at upload time.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Need indexed geometry | `mesh` |
| Need per-item size | `point` |
| Need line caps, joins, or width | `segment` or `path` |
| Need shape or rotation | `marker` |
| Need grouped sequences | `path` or `glyph` |

`primitive` should not grow indexed support, per-item size, or line-quality controls.
Each of those needs belongs to a dedicated family.


## Minimum Cases This Spec Must Support

1. point cloud with uniform color — `point_list`, `color` as `CONSTANT`,
2. point cloud with per-vertex color — `point_list`, `color` as `PER_ITEM` rgba,
3. many independent line segments — `line_list`, `color` as `PER_ITEM`,
4. a connected line strip across many vertices — `line_strip`,
5. flat-shaded triangle geometry — `triangle_list`,
6. 20 groups of sinusoid traces with per-group color — `line_strip`, `color` as `PER_GROUP`,
7. large point cloud with scalar colormap — `point_list`, `color` as `PER_ITEM` scalar mode.


## v0.3 Correspondence

In v0.3, this family was `basic`:

```c
dvz_basic(batch, topology, flags)
dvz_basic_position(visual, first, count, positions, flags)
dvz_basic_color(visual, first, count, colors, flags)
dvz_basic_group(visual, first, count, group_indices, flags)
dvz_basic_size(visual, size)
dvz_basic_alloc(visual, item_count)
```

The v0.4 mapping:

| v0.3 | v0.4 |
|---|---|
| `dvz_basic_position` | `position` attribute, `PER_ITEM` |
| `dvz_basic_color` | `color` attribute, `rgba` mode, `PER_ITEM` |
| `dvz_basic_group` | removed — replaced by `color` with `PER_GROUP` source |
| `dvz_basic_size` | `size` parameter, point topology only |

The `group` attribute is not carried forward as a named field.
The v0.3 pattern of computing per-group colors on the CPU and passing them as rgba is still valid
and maps to `color` as `PER_ITEM` rgba.
The new `PER_GROUP` color source replaces the case where the user only wanted a small color table
indexed by group.


## Deferred Questions

1. whether `triangle_fan` should be included given its limited support in some WebGPU
   implementations,
2. the exact public API spelling for topology declaration at creation time,
3. whether a per-vertex `alpha` attribute separate from `color` is useful for `primitive`,
4. whether `primitive` should expose a `linewidth` parameter for line topologies even without
   full cap and join control.
