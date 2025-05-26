# Path Visual

The **Path** visual renders continuous polylines — sequences of connected line segments that form a single open or closed path. It supports batch rendering with multiple disconnected paths per visual, per-vertex styling and optional variable thickness, making it well-suited for contours, trajectories, and line-based annotations.

!!! note

    This visual is powerful and high-quality, but not optimized or scalable to millions of points. For very large datasets, use the lower-quality but more scalable [**Basic**](basic.md) visual instead, with the `line_strip` primitive and groups.


<figure markdown="span">
![Path visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/path.png)
</figure>


---

## Overview

- Renders connected polylines from vertex sequences
- Supports **per-vertex color and linewidth**
- Each group of vertices forms an independent path
- Optional cap and join styles for line ends and corners

!!! warning

    Dashed paths are not yet implemented.

---

## When to use

Use the path visual when:

- You want to draw 2D or 3D trajectories or contours
- You need continuous, styled polylines with thickness
- You want to render multiple independent paths in one visual

---

## Attributes

### Per-item

| Attribute   | Type             | Description                          |
|-------------|------------------|--------------------------------------|
| `position`  | `(N, 3) float32` | Vertex positions in NDC              |
| `color`     | `(N, 4) uint8`   | RGBA color per vertex                |
| `linewidth` | `(N,) float32`   | Line thickness in pixels             |

### Per-visual (uniform)

| Attribute | Type | Description                                   |
|-----------|------|-----------------------------------------------|
| `cap`     | enum | Cap style at the start/end of each path       |
| `join`    | enum | Join style between connected segments         |

Cap and join styles are defined by enums from the Vulkan line rendering system.


## Cap types

Each path endpoint can be rendered with a custom **cap** style:

| Cap Name       | Image |
|----------------|------|
| `round`        | ![cap_round](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_round.png)    |
| `triangle_in`  | ![cap_triangle_in](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_triangle_in.png)    |
| `triangle_out` | ![cap_triangle_out](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_triangle_out.png)    |
| `square`       | ![cap_square](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_square.png)    |
| `butt`         | ![cap_butt](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_butt.png)    |


## Joint styles

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

## Large-scale paths

For very large paths (e.g. time series with millions of points), you may prefer the [**Basic**](basic.md) visual with `line_strip` topology. This will be more efficient but offers no line width or styling.

---

## Example

```python
--8<-- "examples/visuals/path.py:14:"
```

---

## Summary

The path visual is ideal for rendering styled, continuous line sequences.

* ✔️ Variable thickness and color
* ✔️ Multiple independent paths per visual
* ✔️ Custom caps and joins
* ❌ No dashed line support yet

See also:

* [**Segment**](segment.md) for unconnected lines
* [**Basic**](basic.md) for large 1-pixel polylines
