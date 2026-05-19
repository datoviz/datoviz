# Segment Visual

The **Segment** visual renders independent line segments between pairs of 3D positions. It is the
right stroke family for disconnected lines, rulers, graph edges, error bars, vector shafts, and
other endpoint-pair geometry.

<figure markdown="span">
![Segment visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/segment.png)
</figure>

---

## Overview

- Each segment is defined by a `position_start` and `position_end` 3D point.
- Supports per-segment color and `stroke_width`.
- Supports visual-wide endpoint caps: none, round, square, triangle-in, triangle-out, and butt.
- Renders as analytic screen-space stroked quads in the current GLSL/Vulkan path.

!!! note

    Arrow caps, dashes, endpoint shifts, `color_end` gradients, segment picking, and WGSL segment
    lowering are deferred in v0.4-dev.

---

## When to use

Use the segment visual when:

- You want to render many disjoint lines or vectors.
- You need screen-space stroke width and endpoint caps.
- You do not need connected path joins or ordered polyline semantics.

---

## Properties

### Per-item

| Attribute | Type | Description |
|---|---|---|
| `position_start` | `(N, 3) float32` | Start endpoint in visual space |
| `position_end` | `(N, 3) float32` | End endpoint in visual space |
| `color` | `(N, 4) uint8` | RGBA color per segment |
| `stroke_width` | `(N,) float32` | Stroke width in screen pixels |

### Per-visual (uniform)

| Parameter   | Type        | Description                             |
|-------------|-------------|-----------------------------------------|
| `cap_start`, `cap_end` | `enum` | Endpoint cap types set by `dvz_segment_set_caps()` |

---

## Cap types

Each segment endpoint can be rendered with a custom **cap** style:

| Cap Name       | Image |
|----------------|------|
| `round`        | ![cap_round](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_round.png)    |
| `triangle_in`  | ![cap_triangle_in](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_triangle_in.png)    |
| `triangle_out` | ![cap_triangle_out](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_triangle_out.png)    |
| `square`       | ![cap_square](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_square.png)    |
| `butt`         | ![cap_butt](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_butt.png)    |

Use `dvz_segment_set_caps(visual, start_cap, end_cap)` to set cap styles in the v0.4 C API.

---

## Example

```python
--8<-- "cleaned/visuals/segment.py"
```

---

## Summary

The segment visual is ideal for rendering many disconnected lines with full styling control per segment.

* yes: independent start/end endpoint pairs
* yes: per-segment color and `stroke_width`
* yes: visual-wide endpoint caps
* deferred: arrows, dashes, endpoint shifts, gradients, picking, WGSL parity
* not suitable for continuous joined paths; see [**Path**](path.md)

See also:

* [**Path**](path.md) for polylines
* [**Basic**](basic.md) for low-level lines with no per-segment styling
