# Pixel Visual

The **Pixel** visual renders individual square-shaped pixels at arbitrary positions. Each pixel has a given size (shared by all vertices in the visual) and a color.

This visual is ideal for raster-style plots or large-scale point clouds, where millions of points can be displayed efficiently with minimal styling.

<figure markdown="span">
![Pixel visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/pixel.png)
</figure>

---

## Overview

- Renders square, filled pixels at 3D NDC positions
- **Per-vertex**: position and color
- **Uniform**: pixel size (applies to all vertices)
- Supports **depth testing** for 3D layering
- Efficient for dense, unstructured data

---

## When to use

Use the pixel visual when:

- You want to display raw 2D or 3D data points without borders or variable sizing
- You need to visualize millions of points efficiently

---

## Attributes

### Per-item

| Attribute  | Type             | Description                     |
|------------|------------------|---------------------------------|
| `position` | `(N, 3) float32` | 3D positions in NDC             |
| `color`    | `(N, 4) uint8`   | RGBA color per pixel            |

### Per-visual (uniform)

| Attribute | Type  | Description                                 |
|-----------|-------|---------------------------------------------|
| `size`    | float | Side length of each pixel in framebuffer pixels |

---

## Basic usage

```python
visual = app.pixel(position=position, color=color, size=5)
```

* `position`: a `(N, 3)` array in **Normalized Device Coordinates (NDC)**
* `color`: a `(N, 4)` array of `uint8` RGBA values
* `size`: a scalar integer for pixel size in framebuffer pixels

---

## Example

```python
--8<-- "cleaned/visuals/pixel.py"
```

---

## Summary

The pixel visual is a fast, lightweight way to display large datasets as colored squares in 2D or 3D space.

* ✔️ Per-point position and color
* ✔️ Efficient rendering of millions of points
* ✔️ Optional depth testing for 3D control
* ❌ No per-point size or shape variation

For symbolic or styled points, see the [**Point**](point.md) or [**Marker**](marker.md) visuals.
