# Point Visual

The **Point** visual renders circular, borderless filled discs at 2D or 3D positions. It provides a simple and efficient way to visualize large sets of unstyled points.

<figure markdown="span">
![Point visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/point.png)
</figure>

---

## Overview

- Renders **filled circular discs**
- **Per-vertex**: position, color, size
- No border, shape, or texture

---

## When to use

Use the point visual when:

- You need basic, fast scatter plots with custom colors and sizes
- You don’t need symbolic shapes or outlines (use [**Marker**](marker.md) for that)
- You want simple 3D point clouds with minimal overhead (but more overhead than [**Pixel**](pixel.md))

---

## Attributes

### Per-item

| Attribute | Type           | Description                      |
|-----------|----------------|----------------------------------|
| `position` | `(N, 3) float32` | 3D positions in NDC space       |
| `color`    | `(N, 4) uint8`   | Per-point RGBA color            |
| `size`     | `(N,) float32`   | Per-point diameter in pixels    |

All attributes are per-vertex and required.

---

## Example

```python
--8<-- "cleaned/visuals/point.py"
```

---

## Summary

The point visual is the fastest way to render a large number of colored, sized circular points.

* ✔️ Efficient for scatter plots and point clouds
* ✔️ Fully GPU-accelerated
* ❌ No support for borders, shapes, or textures (use [**Marker**](marker.md) instead)
