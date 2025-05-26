# Segment Visual

The **Segment** visual renders independent line segments between pairs of 3D positions. Each segment can have its own color, thickness, shift.

<figure markdown="span">
![Segment visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/segment.png)
</figure>

---

## Overview

- Each segment is defined by an **initial** and **terminal** 3D point
- Supports **per-segment color, linewidth, and shift**
- Customizable caps: round, square, triangle, butt, etc.
- Fast and flexible for disconnected line data

!!! note

    2D arrows are not yet supported in the segment visual. For now, consider using the [**Marker**](marker.md) visual with an `arrow` shape, or the 3D [Shape API](../reference/api_py.md) to generate 3D arrows, which can be rendered using the [**Mesh**](mesh.md) visual.

---

## When to use

Use the segment visual when:

- You want to render many disjoint lines or vectors
- You need variable width, style, or shift per segment

---

## Attributes

### Per-item

| Attribute   | Type             | Description                              |
|-------------|------------------|------------------------------------------|
| `position`  | `(N, 3)` x 2     | Initial and terminal 3D points (NDC)     |
| `color`     | `(N, 4) uint8`   | RGBA color per segment                   |
| `linewidth` | `(N,) float32`   | Line thickness in pixels                 |
| `shift`     | `(N, 4) float32` | Pixel offset applied to both endpoints   |

### Per-visual (uniform)

| Attribute   | Type        | Description                             |
|-------------|-------------|-----------------------------------------|
| `cap`       | `enum` x 2  | Cap type (for both ends)                |

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

Use `visual.set_cap(initial, terminal)` to set cap styles.

---

## Example

```python
--8<-- "examples/visuals/segment.py:15:"
```

---

## Summary

The segment visual is ideal for rendering many disconnected lines with full styling control per segment.

* ✔️ Initial and terminal points
* ✔️ Per-segment color and thickness
* ✔️ Uniform cap
* ✔️ Supports pixel shifting for layering or emphasis
* ❌ Not suitable for continuous paths (see [**Path**](path.md) instead)

See also:

* [**Path**](path.md) for polylines
* [**Basic**](basic.md) for low-level lines with no per-segment styling
