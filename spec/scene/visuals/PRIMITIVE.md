# Visual Family: `primitive`

This document defines the per-item data contract, parameter schema, and behavioral rules for the
`primitive` visual family.

It refines `../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Shared attribute and behavioral definitions are in `SHARED_ATTRIBUTES.md`.

> **v0.3 note**: this family was called `basic` in v0.3.
>
> **v0.4 implementation**: landed. `dvz_primitive(scene, topology, flags)` supports
> `POINT_LIST`, `LINE_LIST`, `LINE_STRIP`, `TRIANGLE_LIST`, `TRIANGLE_STRIP` with `position` +
> `color` attributes, optional `normal`, optional index-buffer binding, and GPU item picking.
> `size` parameter for `point_list` topology and scalar color modes are not yet wired up.


## Semantic Purpose

`primitive` is a first-class low-level visual family for explicit topology-driven rendering.

It exposes GPU primitive topologies directly — point lists, line strips, triangle lists — with
minimal scene-level abstraction.

It exists to provide a durable low-level escape hatch, support experimentation and renderer
bring-up, and serve as a baseline pressure test for the visual contract.

`primitive` should stay intentionally constrained. Richer semantic needs belong to dedicated
families.


## Topology

Declared at visual creation time; cannot change without recreating the visual.

Topology is the **second argument** to the constructor:

```c
DvzVisual* vis = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
```

The full `DvzPrimitiveTopology` enum is imported by `include/datoviz/scene/enums.h` from the shared
Vulkan/vklite enum surface.

| Topology | Description |
|---|---|
| `point_list` | one point per vertex |
| `line_list` | independent line segments, two vertices each |
| `line_strip` | connected line sequence |
| `triangle_list` | independent triangles, three vertices each |
| `triangle_strip` | connected triangle strip |

An optional `"index"` buffer binding is installed for low-level indexed primitive draws. For
semantic indexed surface geometry, prefer `mesh`.


## Per-Item Attributes

Each item is one vertex.

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |


### `color`

Standard — see `SHARED_ATTRIBUTES.md`.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.

In v0.3, per-group color was implemented by computing colors CPU-side from a group index
(`dvz_basic_group`). In v0.4 this is replaced by `color` with `PER_GROUP` source.


### `normal`

| Property | Value |
|---|---|
| Type | `vec3`, normal in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` |

Optional. Used by primitive material/shading paths when enabled.


## Visual-Wide Parameters

### `size`

| Property | Value |
|---|---|
| Type | `float32`, screen pixels |
| Default | implementation-defined, suggested 1.0 |
| Mutability | `dynamic` |
| Applies to | `point_list` topology only |

Uniform point size. Ignored for line and triangle topologies.


## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf vertex skipped when possible; otherwise validation error | no |
| `color` | opaque white RGBA | scalar NaN uses scale missing color | yes |
| `topology` | constructor-selected topology | n/a | no |


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |

Set at visual creation time.


## Transform Model, Stage Participation, Picking

Standard — see `SHARED_ATTRIBUTES.md`.
Picking returns vertex index as item identity.


## Relationship To Other Families

| Situation | Preferred family |
|---|---|
| Need indexed geometry | `mesh` |
| Need per-item size | `point` |
| Need line caps, joins, or width | `segment` or `path` |
| Need shape or rotation | `marker` |
| Need grouped sequences | `path` or `glyph` |


## Minimum Cases This Spec Must Support

1. point cloud with uniform color — `point_list`, `color` `CONSTANT`,
2. point cloud with per-vertex color — `point_list`, `color` `PER_ITEM`,
3. independent line segments — `line_list`,
4. connected line strip — `line_strip`,
5. flat-shaded triangles — `triangle_list`,
6. 20 sinusoid groups with per-group color — `line_strip`, `color` `PER_GROUP`,
7. large point cloud with scalar colormap — `point_list`, `color` `PER_ITEM` scalar.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_basic_position` | `position`, `PER_ITEM` |
| `dvz_basic_color` | `color`, now also `CONSTANT`/`PER_GROUP` and `scalar` mode |
| `dvz_basic_group` | removed — replaced by `color` with `PER_GROUP` source |
| `dvz_basic_size` | `size` parameter, point topology only |


## Line Width

`primitive` does not expose a `linewidth` parameter. OpenGL line width is not reliably supported
across drivers and is not available in WebGPU. For thick lines, use `path` or `segment`.
`primitive` line topologies always render at 1 physical pixel.
