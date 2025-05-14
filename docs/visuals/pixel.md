# Pixel Visual

The **pixel** visual renders individual square-shaped pixels at arbitrary positions. Each pixel has a fixed size (shared by all vertices in the visual) and a unique color.

This visual is ideal for raster-style plots or large-scale point clouds, where millions of points can be displayed efficiently with minimal styling.

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

## Basic usage

```python
visual = app.pixel(position=position, color=color, size=4)
````

* `position`: a `(N, 3)` array in **Normalized Device Coordinates (NDC)**
* `color`: a `(N, 4)` array of `uint8` RGBA values
* `size`: a scalar integer for pixel size in framebuffer pixels

---

## Enabling depth testing

In 3D visualizations, you can control whether pixels are rendered with or without depth testing:

```python
visual.set_data(position=position, color=color, size=4, depth_test=True)
```

* `depth_test=True`: Pixels respect the 3D depth buffer (closer points appear in front)
* `depth_test=False`: All pixels are rendered in the order they are submitted (useful for 2D overlays)

---

## Example

```python
--8<-- "examples/visuals/pixel.py"
```

---

## Summary

The pixel visual is a fast, lightweight way to display large datasets as colored squares in 2D or 3D space.

* ✔️ Per-point position and color
* ✔️ Efficient rendering of millions of points
* ✔️ Optional depth testing for 3D control
* ❌ No per-point size or shape variation

For symbolic or styled points, see the [Point](point.md) or [Marker](marker.md) visuals.
