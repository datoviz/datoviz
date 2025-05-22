# Path Visual

The **path** visual renders continuous polylines — sequences of connected line segments that form a single open or closed path. It supports batch rendering with multiple disconnected paths per visual, per-vertex styling and optional variable thickness, making it well-suited for contours, trajectories, and line-based annotations.

!!! note

    This visual is powerful and high-quality, but not optimized or scalable to millions of points. For very large datasets, use the lower-quality but more scalable [Basic](basic.md) visual instead, with the `line_strip` primitive and groups.

---

## Overview

- Renders connected polylines from vertex sequences
- Supports **per-vertex color and linewidth**
- Each group of vertices forms an independent path
- Optional cap and join styles for line ends and corners

---

## When to use

Use the path visual when:
- You want to draw 2D or 3D trajectories or contours
- You need continuous, styled polylines with thickness
- You want to render multiple independent paths in one visual

---

## Attributes

### Per-vertex

| Attribute   | Type             | Description                          |
|-------------|------------------|--------------------------------------|
| `position`  | `(N, 3) float32` | Vertex positions in NDC              |
| `color`     | `(N, 4) uint8`   | RGBA color per vertex                |
| `linewidth` | `(N,) float32`   | Line thickness in pixels             |

### Uniform

| Attribute | Type | Description                                   |
|-----------|------|-----------------------------------------------|
| `cap`     | enum | Cap style at the start/end of each path       |
| `join`    | enum | Join style between connected segments         |

Cap and join styles are defined by enums from the Vulkan line rendering system.


### Cap types

| Cap Name       |
|----------------|
| `round`        |
| `triangle_in`  |
| `triangle_out` |
| `square`       |
| `butt`         |


### Joint styles

| Cap Name       |
|----------------|
| `square`       |
| `round`        |


---

## Grouping paths

Each visual can include multiple independent paths. Use `visual.set_position()` to specify how the data is grouped. You can pass either:

* A **list of arrays**, where each array defines one path
* A single position array, and an additional argument `groups` which is either:

  * `int`: number of paths (the position array is split in that number of equal size sub-paths)
  * `np.ndarray`: an array of group size integers

Each group becomes a separate, continuous polyline.

---

## Advanced options

* Use different `cap` values (`butt`, `round`, `square`, etc.) to style path ends
* Use different `join` types (`miter`, `bevel`, `round`) for sharp corners
* Dashed paths are **planned**, but not yet implemented

---

## Large-scale paths

For very large paths (e.g. time series with millions of points), you may prefer the [`basic`](basic.md) visual with `line_strip` topology. This will be more efficient but offers no line width or styling.

---

## Example

```python
--8<-- "examples/visuals/path.py"
```

---

## Summary

The path visual is ideal for rendering styled, continuous line sequences.

* ✔️ Variable thickness and color
* ✔️ Multiple independent paths per visual
* ✔️ Custom caps and joins
* ❌ No dashed line support yet

See also:

* [Segment](segment.md) for unconnected lines
* [Basic](basic.md) for large 1-pixel polylines
