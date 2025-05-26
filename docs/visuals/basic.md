# Basic Visual

The **basic** visual in Datoviz provides direct access to low-level GPU rendering using core Vulkan primitives.

This visual is well suited for rendering large-scale data as points, thin **non-antialiased** lines, or triangles, with one color per vertex.

<figure markdown="span">
![Basic visual with `line_strip` topology](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/basic.png)
</figure>

!!! note

    Crude antialiasing techniques may be added in a future version. In the meantime, these visuals are heavily aliased but remain highly efficient.

---

## Overview

- Uses core **Vulkan primitive topologies**: `point`, `line_strip`, `line_list`, `triangle_list`, and `triangle_strip`
- Accepts **3D NDC positions**
- Supports **per-vertex color**
- Supports **uniform size** but not per-vertex size
- Supports a `group` attribute to separate primitives within a single visual

---

## When to use

Use the basic visual when you need:

- Large-scale, uniform visual primitives
- Full control over rendering topology
- Minimal overhead and high performance

---

## Attributes

### Per-item

| Attribute  | Type             | Description                                 |
|------------|------------------|---------------------------------------------|
| `position` | `(N, 3) float32` | Vertex positions in NDC                     |
| `color`    | `(N, 4) uint8`   | RGBA color per vertex                       |
| `group`    | `(N,) float32`   | Group ID used to separate primitives (`line_strip` and `triangle_strip`) |

### Per-visual (uniform)

| Attribute | Type   | Description                     |
|-----------|--------|---------------------------------|
| `size`    | `float`  | Pixel size for `point` topology |

---

## Grouping

The `group` attribute allows you to encode multiple disconnected geometries in one visual.

For example, rendering many time series as separate line strips:

```python
visual = app.basic('line_strip', position=position, color=color, group=group)
```

* Each group must be identified by a **consecutive integer ID**: `0, 0, ..., 1, 1, ..., 2, 2, ...`
* Transitions between groups break the connection in line-based topologies.

This approach is memory-efficient and avoids multiple draw calls.

---

## Topologies

Here, we show the different primitive topologies for the **Basic** visual using the same set of positions, colors, and groups.

The 2D points follow the pattern `sin(dt * i) + dy`, where `dy` is positive if `i` is odd and negative if `i` is even. The points therefore alternate between the lower and upper sine curves. The points are split in two groups with equal size (the `group` is 0 if `i < N/2` and 1 if `i >= N/2`).

### Points

Renders one dot per vertex. Similar to the [**Pixel**](pixel.md) visual.

```python
visual = app.basic('point_list', position=position, color=color, size=5)
```

<figure markdown="span">
![Basic visual, `point_list` topology](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/basic_point_list.png)
</figure>

---

### Line list

Each pair of vertices forms a separate segment: `A-B`, `C-D`, `E-F`, etc.

```python
visual = app.basic('line_list', position=position, color=color)
```

<figure markdown="span">
![Basic visual, `line_list` topology](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/basic_line_list.png)
</figure>

---

### Line strip

Connects vertices into a continuous path: `A-B-C-D` becomes a single polyline.

```python
visual = app.basic('line_strip', position=position, color=color, group=group)
```

!!! note

    The optional `group` attribute can be used to draw multiple line strips in one call, which is best for performance. Note the gap in the middle, corresponding to the transition between the two groups.

<figure markdown="span">
![Basic visual, `line_strip` topology](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/basic_line_strip.png)
</figure>

---

### Triangle list

Each group of 3 vertices forms a triangle: `A-B-C`, `D-E-F`, etc.

```python
visual = app.basic('triangle_list', position=position, color=color)
```

<figure markdown="span">
![Basic visual, `triangle_list` topology](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/basic_triangle_list.png)
</figure>

---

### Triangle strip

Renders connected triangles using a strip pattern: `A-B-C`, `B-C-D`, `C-D-E`, etc.

```python
visual = app.basic('triangle_strip', position=position, color=color, group=group)
```

!!! note

    The optional `group` attribute can be used to draw multiple triangle strips in one call, which is best for performance. Note the gap in the middle, corresponding to the transition between the two groups.

<figure markdown="span">
![Basic visual, `triangle_strip` topology](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/basic_triangle_strip.png)
</figure>

---

## Example

This example displays a GUI to change the topology of the **Basic** visual.

<figure markdown="span">
![Basic topology widget](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/basic_topology.png)
</figure>

```python
--8<-- "examples/features/basic_topology.py:14:"
```

---

## Summary

The `basic` visual provides direct access to the core rendering capabilities of Datoviz. It's powerful, fast, and flexible — ideal for high-volume data when you don’t need complex styling.

For advanced visuals with lighting, texturing, or custom shapes, see the other visuals.
