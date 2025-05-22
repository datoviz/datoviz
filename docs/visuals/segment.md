# Segment Visual

The **segment** visual renders independent line segments between pairs of 3D positions. Each segment can have its own color, thickness, shift, and custom caps.

Unlike polylines or paths, each segment is treated as a standalone graphical element — ideal for vector fields, edge representations, or overlaid annotations.

---

## Overview

- Each segment is defined by an **initial** and **terminal** 3D point
- Supports **per-segment color, linewidth, shift, and cap**
- Customizable caps: round, square, triangle, butt, etc.
- Fast and flexible for disconnected line data

!!! note

    2D arrows are not yet supported in the segment visual. For now, consider using the [Marker](marker.md) visual with an `arrow` shape, or the 3D [Shape API](../reference/api_py.md) to generate 3D arrows, which can be rendered using the [Mesh](mesh.md) visual.

---

## When to use

Use the segment visual when:
- You want to render many disjoint lines or vectors
- You need variable width, style, or shift per segment
- You want to control endpoint caps individually

---

## Attributes

### Per-segment

| Attribute   | Type             | Description                              |
|-------------|------------------|------------------------------------------|
| `position`  | `(N, 3)` x 2     | Initial and terminal 3D points (NDC)     |
| `color`     | `(N, 4) uint8`   | RGBA color per segment                   |
| `linewidth` | `(N,) float32`   | Line thickness in pixels                 |
| `shift`     | `(N, 4) float32` | Pixel offset applied to both endpoints   |
| `cap`       | `(N,) int32`     | Cap type (for both ends)                |

---

## Cap types

Each segment endpoint can be rendered with a custom **cap** style, defined using an integer code:

| Cap Name       | Code |
|----------------|------|
| `round`        | 1    |
| `triangle_in`  | 2    |
| `triangle_out` | 3    |
| `square`       | 4    |
| `butt`         | 5    |

Use `visual.set_cap(initial, terminal)` to set per-endpoint cap styles. Both arguments are integer arrays of shape `(N,)`.

**Note:** Cap values are currently integer codes — support for string-based enums may be added in the future.

---

## Example

```python
--8<-- "examples/visuals/segment.py"
```

---

## Summary

The segment visual is ideal for rendering many disconnected lines with full styling control per segment.

* ✔️ Initial and terminal points
* ✔️ Per-segment color, thickness, and caps
* ✔️ Supports pixel shifting for layering or emphasis
* ❌ Not suitable for continuous paths (see [Path](path.md) instead)

See also:

* [Path](path.md) for polylines
* [Basic](basic.md) for low-level lines with no per-segment styling
