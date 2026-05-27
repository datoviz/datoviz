# Visual Family: `vector`

Status: proposed v0.4 visual family. No public `dvz_vector()` constructor or
`DVZ_VISUAL_TYPE_VECTOR` enum value is installed yet.

This document defines a vector and arrow visual contract. It refines
`../semantics/VISUAL_FAMILIES.md`, `../semantics/VISUAL_FAMILY_RULES.md`,
`../pipeline/ATTRIBUTE_SOURCES.md`, and `../semantics/VISUAL_CONTRACT.md`.

The first implementation should remain a normal retained scene visual and may lower internally to
generated shaft/head roles. It must not introduce a parallel renderer path.


## Objective

Datoviz should expose vector and arrow rendering as one semantic visual contract, not as ad hoc
user-managed `segment`, `path`, `marker`, or `primitive` combinations.

The immediate goal is a clean design target for quiver plots, wind fields, cell-motion vectors,
load arrows, displacement arrows, lattice vectors, and selected trajectory direction cues. The first
implementation may lower to existing stroke and marker machinery, but users should still interact
with one logical vector item.


## Current Position

Status on 2026-05-27: `vector` is concept/spec only.

The active runtime does not implement retained vector state, shader variants, DRP2 lowering,
request/readback behavior, or public C API.

Relevant existing contracts:

1. [`SEGMENT.md`](SEGMENT.md) owns independent endpoint-pair strokes.
2. [`PATH.md`](PATH.md) owns connected stroked sequences.
3. [`MARKER.md`](MARKER.md) owns screen-facing symbolic marks,
   including future arrow marker shapes.
4. [`../api/API_SURFACE.md`](../api/API_SURFACE.md) says vector/arrow APIs may expose one
   semantic mutation surface while lowering to leaf roles such as `"shaft"` and `"head_end"`.
5. [`IMPLEMENTATION_DECISIONS.md`](IMPLEMENTATION_DECISIONS.md) records
   that 2D vector fields should lower to segment/marker backends with source item identity
   preserved.


## Core Direction

Add a first-class `vector` visual family when implementation begins.

`arrow` should be a presentation mode of `vector`, not a separate family. A vector item may render
with no head, one head, or two heads, but the semantic item remains the same origin plus direction
record.

Required invariants:

1. one source vector item maps to one pickable scene item;
2. derived shafts and heads preserve source item identity;
3. vector direction is transformed using scene coordinate semantics, not by screen-space guessing;
4. 2D arrows use screen-space stroke/head styling by default;
5. true 3D arrows, tubes, and streamlines remain separate geometry lanes;
6. dense vector fields should be expressible without CPU-rebuilding arrow geometry every frame.


## Visual Or Composite Object

The preferred public shape is a normal retained visual:

```c
DvzVisual* dvz_vector(DvzScene* scene, uint32_t flags);
```

The visual may internally own generated leaf roles:

```text
"shaft"      segment-like stroke contribution
"head_start" optional marker/glyph/geometry contribution
"head_end"   optional marker/glyph/geometry contribution
```

Those role names are useful for tests and diagnostics, but common user code should configure the
vector visual through vector-specific setters and normal visual data APIs.

A higher-level domain object such as a sampled vector field may later create or update a `vector`
visual as one render view. That field helper should not replace the visual family.


## Item Model

Each item is one vector. The vector can be understood as:

```text
tail = position
head = position + vector * scale
```

`position` is the item anchor in visual/data coordinates. `vector` is a displacement or direction in
the same coordinate system unless the visual declares a different vector-space mode.

The default anchor is the tail. Other anchors change how the displacement is placed around
`position`:

| Anchor | Tail | Head |
|---|---|---|
| `tail` | `position` | `position + vector * scale` |
| `center` | `position - 0.5 * vector * scale` | `position + 0.5 * vector * scale` |
| `head` | `position - vector * scale` | `position` |


## Per-Item Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, anchor point `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |


### `vector`

| Property | Value |
|---|---|
| Type | `vec3`, displacement or direction associated with `position` |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |

For 2D data, `z` should be `0`.


### `color`

Standard visual color attribute. Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.

In scalar color mode, the scalar may be user supplied or derived from vector magnitude by a future
helper. The first visual contract should keep the source explicit rather than hiding an implicit
magnitude upload.


### `stroke_width`

Standard stroke width attribute. Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_GROUP`.

This controls the shaft width. It does not by itself define arrowhead size.


### `head_length`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | determined by `head_size_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |
| Optional | yes |

Length of the arrowhead measured along the vector direction.


### `head_width`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | determined by `head_size_space` |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` |
| Optional | yes |

Width of the arrowhead measured perpendicular to the vector direction.


### `magnitude`

| Property | Value |
|---|---|
| Type | `float32` |
| Accepted sources | `PER_ITEM`, `PER_GROUP` |
| Typical mutability | `dynamic` or `streaming` |
| Optional | yes |

Explicit magnitude channel for color, opacity, filtering, legends, or labels. When absent, examples
may derive magnitude from `vector` during preprocessing. The core visual should not require a
separate magnitude array to render arrows.


### `id`

| Property | Value |
|---|---|
| Type | `uint32` or future stable item-id type |
| Accepted sources | `PER_ITEM` |
| Typical mutability | `static` |
| Optional | yes |

Optional authored item identity for field samples, tracks, or simulation particles. Picking still
returns the visual item index first; richer query payloads may include the authored id.


## Visual-Wide Parameters

### `scale`

| Property | Value |
|---|---|
| Type | `float32` |
| Default | `1.0` |
| Mutability | `dynamic` |

Multiplicative scale applied to every `vector` before tail/head derivation.

Use this for quiver and wind-field UI controls. Updating `scale` should not require re-uploading
the source `position` or `vector` arrays.


### `normalize`

| Property | Value |
|---|---|
| Type | `bool` |
| Default | `false` |
| Mutability | `dynamic` |

When true, the visual uses only vector direction for geometry length and takes arrow length from a
separate length policy. The first implementation may defer this and require pre-normalized vectors.


### `anchor`

| Property | Value |
|---|---|
| Type | enum: `tail`, `center`, `head` |
| Default | `tail` |
| Mutability | `dynamic` |

Defines whether `position` represents the tail, center, or head of the rendered vector.


### `head_placement`

| Property | Value |
|---|---|
| Type | enum: `none`, `end`, `start`, `both` |
| Default | `end` |
| Mutability | `dynamic` |

Controls which arrowheads are rendered. `none` gives a pure vector shaft while preserving the
vector item model.


### `head_style`

| Property | Value |
|---|---|
| Type | shared arrow style enum |
| Default | `filled` |
| Mutability | `dynamic` |

The semantic set should match the arrow vocabulary already used by marker and segment planning:

| Style | Description |
|---|---|
| `filled` | solid triangular arrowhead |
| `open` | two-stroke V arrowhead |
| `stealth` | swept-back chevron arrowhead |
| `circle` | circular endpoint marker with arrow semantics |


### `head_size_space`

| Property | Value |
|---|---|
| Type | enum: `screen` or `data` |
| Default | `screen` |
| Mutability | `dynamic` |

For dense quiver plots, screen-space heads are the right default because they stay legible while
zooming. Data-space heads are useful for physical 3D or engineering arrows, but may require a
geometry lane instead of the first 2D stroke backend.


### `stroke_width_space`

Standard stroke-width space. Default: `screen`.


## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar` | `rgba` |
| `length_mode` | `data`, `screen` | `data` |
| `head_mode` | `none`, `end`, `start`, `both` | `end` |
| `dimension` | `2d`, `3d` | inferred from panel and data |

`length_mode = data` means the shaft endpoint is derived in data/visual coordinates and then
projected. `length_mode = screen` means the vector defines direction but the rendered length is in
screen pixels. The first implementation should start with `data`; `screen` can follow when the
transform and picking behavior are specified.


## Transform Semantics

The scene must compute vector orientation from transformed endpoints, not by treating vector
components as already-screen-space values.

For linear transforms:

```text
tail_data = position
head_data = position + vector * scale
tail_screen = transform(tail_data)
head_screen = transform(head_data)
direction_screen = normalize(head_screen - tail_screen)
```

For nonlinear transforms, the same endpoint rule remains the baseline. Projection-aware helpers may
provide local tangent vectors before upload for map projections or curvilinear domains, but the
visual contract should still receive ordinary `position` and `vector` data.

This matters for geographic wind fields, deformed meshes, and anisotropic scientific coordinates.


## Lowering Strategy

The expected first 2D lowering is:

1. derive shaft endpoint pairs from `position`, `vector`, `scale`, and `anchor`;
2. render shafts through the segment/path stroke backend;
3. render end heads through marker-style arrow glyphs or generated head triangles;
4. keep a derived-item table mapping every shaft/head primitive back to the source vector index.

This keeps the first slice aligned with the existing segment, marker, and GPU query work. It also
avoids a second renderer for arrows.

Open implementation choice:

1. use a generated head mesh/triangle path for maximum control; or
2. use code-SDF marker arrows for simpler styling and antialiasing.

The proposal prefers code-SDF or analytic head rendering for the first 2D slice, and reserves mesh
arrowheads for true 3D arrows.


## Picking And Selection

Picking returns the source vector item index, not the derived shaft or head index.

The query payload may later include a subpart:

| Subpart | Meaning |
|---|---|
| `shaft` | hit landed on the vector shaft |
| `head_start` | hit landed on the start head |
| `head_end` | hit landed on the end head |

Subpart identity is optional. Source item identity is required.

Selection and highlight should apply to both shaft and heads. A selected vector must not highlight
only one generated role.


## Relationship To Other Families

| Situation | Preferred surface |
|---|---|
| independent non-directional line segments | `segment` |
| connected trajectories or contours | `path` |
| simple orientation-only point glyphs | `marker` with arrow shape |
| dense quiver or velocity arrows | `vector` |
| straight engineering load arrows | `vector` first, future 3D geometry when needed |
| curved annotation arrows | future arrow object over `path` plus head roles |
| radius-bearing streamlines or fibers | future `tube` |
| continuous vector field resource | sampled-field/domain helper that can render a `vector` view |


## Minimal First Slice

The smallest useful implementation slice should support:

1. `dvz_vector()` retained visual construction;
2. dense `position`, `vector`, `color`, and `stroke_width` data;
3. visual-wide `scale`, `anchor = tail`, `head_placement = end`, and `head_style = filled`;
4. screen-space `stroke_width` and `head_size_space`;
5. 2D panel lowering to existing stroke plus head rendering;
6. GPU picking that maps shaft and head hits to the same source item;
7. one offscreen example with a sparse synthetic vector field.

Everything else can follow after this slice is validated.


## Deferred Capabilities

1. screen-space vector length mode;
2. per-item head style;
3. start and double-headed arrows;
4. vector gradients from tail color to head color;
5. dashes along vector shafts;
6. 3D mesh arrows with lighting, depth, materials, and SSAO/G-buffer participation;
7. curved arrows over `path`;
8. vector-field sampled resources and GPU-side resampling;
9. adaptive thinning, density control, and level-of-detail;
10. streamline, pathline, and tube generation;
11. vector probe payloads tied to sampled fields;
12. exact head-shape picking for all head styles.


## Example Pressure Tests

This design should be checked against these existing planned examples:

1. [`../examples/scenarios/v04_required/SHOWCASES.md`](../examples/scenarios/v04_required/SHOWCASES.md)
   for the v0.4 wind-field showcase.
2. [`../examples/scenarios/v05/SCIENTIFIC_3D_AND_FIELDS.md`](../examples/scenarios/v05/SCIENTIFIC_3D_AND_FIELDS.md)
   for tracks, tractography, projection-aware wind, and scientific 3D arrows.
3. [Later geospatial and physics scenarios](../examples/scenarios/later/GEOSPATIAL_AND_PHYSICS_LATER.md)
   for later high-complexity field-line and event-display pressure.


## Open Questions

1. Should the first public setter surface expose `head_length` and `head_width`, or one
   `head_size` plus a visual-wide aspect ratio?
2. Should `position` plus `vector` be the only source form, or should `position_start` plus
   `position_end` be accepted as an alternate upload schema?
3. Should scalar color mode default to an explicit scalar array, or should magnitude-derived color
   be a first-class mode?
4. Should `length_mode = screen` belong in the first API, or remain a later capability after
   nonlinear transform behavior is tested?
5. Should curved arrows be part of `vector`, part of `path`, or a higher-level arrow object that
   owns generated `path` plus head roles?


## Recommended Next Step

Before implementation, settle the source schema, head sizing model, and pick identity mapping.

If the open questions lead back to a semantic composite rather than a visual family, move this
contract under `spec/scene/semantics/` and leave `VECTOR.md` as a redirecting note. Until then,
`vector` is the proposed visual-family target.
