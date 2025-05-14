# Visuals Overview

In Datoviz, a **visual** is a GPU-accelerated graphical primitive — a low-level building block used to represent data in visual form. Each visual type supports specific properties (e.g. color, size, shape) and is optimized for rendering performance and flexibility.

All visuals expect positions in **3D Normalized Device Coordinates (NDC)** — values in the range `[-1, +1]` on all three axes.

For 2D rendering, set the Z component to zero and use a 2D interaction mode such as pan and zoom (see the Interactivity section).

![](../images/cds2.svg)

This page gives an overview of the available visual types. Each has its own dedicated documentation page with usage examples and options.


---

## Available Visuals

### [Basic](../visuals/basic.md)

Low-level Vulkan primitives: points, line strips, triangles, etc.
No custom shaders — just raw primitives with uniform color.

### [Pixel](../visuals/pixel.md)

Individual with custom position, size, and color.
Not arranged in a grid — useful for sparse images or raster-style rendering.
Useful for heatmaps, raster-style rendering, or sparse images.

### [Point](../visuals/point.md)

Uniformly colored, circular discs with no border.
Fast and ideal for simple scatter plots with minimal styling.

### [Marker](../visuals/marker.md)

Flexible symbolic points with:
- Optional borders
- Predefined shapes (circle, square, cross, etc.)
- Support for custom SVGs or bitmap images

Perfect for labeled or categorized scatter plots.

### [Segment](../visuals/segment.md)

Individual line segments with:
- Arbitrary width
- Optional caps

Used for vector fields, network edges, or custom line-based visuals.

Arrows and dashed lines will be implemented later.

### [Path](../visuals/path.md)

Continuous polylines with optional variable thickness, optionally closed.
Best for trajectories, contours, or medium-size time series.

Dashed paths will be implemented later.

For large-scale time-series (more than 10,000s of points), you may want to use a Basic visual with a `line_strip` topology (1 pixel-width lines only).

### [Image](../visuals/image.md)

2D images (RGBA or single-channel with colormap support).
Anchored at panel coordinates.

### [Text](../visuals/text.md)

Glyph-based text rendering.
Supports positioning, anchoring, and sizing — ideal for annotations or labels.

### [Mesh](../visuals/mesh.md)

3D triangle meshes with options for:
- Flat shading
- Basic or advanced lighting
- Textures
- Contours
- Experimental isolines

Suitable for geometric surfaces, anatomy, or field data.

### [Volume](../visuals/volume.md)

Basic volume rendering for dense 3D scalar fields.
Use this for volumetric data like CT/MRI scans or simulation outputs.

### [Slice](../visuals/slice.md)

2D slices through a 3D volume, rendered as images.
Supports slicing along X, Y, or Z.

---

## Working with visuals

All visuals are created using the `app.<type>()` functions:

```python
visual = app.point(position=..., color=..., size=...)
panel.add(visual)
````

Attributes can also be set or modified as follows:

```python
visual.set_position(position)
visual.set_color(color)
...
```

The `position` is usually an `(N, 3)` NumPy array (converted to `float32` if needed), and `color` is an `(N, 4)` array of RGBA values in the 0–255 range, automatically cast to `uint8`.

Visuals must be added to a panel. All visuals require positions in 3D **NDC**, and most support additional attributes like color, size, or shape depending on the visual type.

For more details, see each visual's documentation page.

