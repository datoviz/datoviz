# Visuals Overview

High-performance rendering in Datoviz is based on the concept of **visuals** — GPU-accelerated primitives such as points, lines, images, and 3D meshes. Each visual represents a *collection of items* of a specific type, rendered in a single batch for maximum efficiency.

Datoviz is designed to render millions of visual items at interactive framerates, assuming items in a visual share the same transformation (e.g., coordinate space). This makes the grouping and batching model crucial to understand.

---

## Available visuals

<div class="grid cards" markdown="1">

<div class="card">
<a href="../../visuals/basic/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/basic.png" alt="Basic" />
<div class="card-title">🔺 <strong>Basic</strong></a>:
Low-level Vulkan primitives: points, line strips, triangles, etc.</div>
</div>

<div class="card">
<a href="../../visuals/pixel/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/pixel.png" alt="Pixel" />
<div class="card-title">🟦 <strong>Pixel</strong></a>:
Individual pixels with custom position, size, and color (for rasters, point clouds, etc.).</div>
</div>

<div class="card">
<a href="../../visuals/point/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/point.png" alt="Point" />
<div class="card-title">⚪ <strong>Point</strong></a>:
Borderless circular discs (for simple scatter plots with minimal styling).
</div>
</div>

<div class="card">
<a href="../../visuals/marker/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/marker.png" alt="Marker" />
<div class="card-title">✳️ <strong>Marker</strong></a>:
Symbols with predefined (circle, square, cross, etc.) or custom (SVG, bitmap) shapes and optional borders.
</div>
</div>

<div class="card">
<a href="../../visuals/segment/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/segment.png" alt="Segment" />
<div class="card-title">➖ <strong>Segment</strong></a>:
Line segments with arbitrary width and customizable caps (no arrows nor dashes for now).
</div>
</div>

<div class="card">
<a href="../../visuals/path/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/path.png" alt="Path" />
<div class="card-title">➰ <strong>Path</strong></a>:
Continuous polylines with optional variable thickness, optionally closed (no arrows nor no dashes for now).
</div>
</div>

<div class="card">
<a href="../../visuals/image/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/image.png" alt="Image" />
<div class="card-title">🖼️ <strong>Image</strong></a>:
User-facing 2D images (RGBA or single-channel with colormap) anchored in world space.
</div>
</div>

<div class="card">
<a href="../../visuals/glyph/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/glyph.png" alt="Glyph" />
<div class="card-title">🔤 <strong>Glyph</strong></a>:
Minimally-customizable text (to be improved in future versions).
</div>
</div>

<div class="card">
<a href="../../visuals/mesh/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/mesh.png" alt="Mesh" />
<div class="card-title">🧊 <strong>Mesh</strong></a>:
Triangulations in 2D or 3D, used for polygons or surface meshes, with optional lighting, texture, contour, wireframes (isolines documented soon).
</div>
</div>

<div class="card">
<a href="../../visuals/sphere/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/sphere.png" alt="Mesh" />
<div class="card-title">🧊 <strong>Sphere</strong></a>:
3D spheres with lighting.
</div>
</div>

<div class="card">
<a href="../../visuals/volume/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/volume.png" alt="Volume" />
<div class="card-title">🌫️ <strong>Volume</strong></a>:
Basic volume rendering for dense 3D scalar fields (RGBA or single-channel with colormap).
</div>
</div>

<!-- <div class="card">
<a href="../../visuals/slice/">
<img src="https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/slice.png" alt="Slice" />
<div class="card-title">🪓 <strong>Slice</strong></div>
</a>
</div> -->

</div>
















---

## Data model: items and groups

Each visual manages a collection of **items**. The definition of an item depends on the visual type:

* **Pixel**, **Point**, **Marker**: 1 item = 1 point or marker
* **Segment**: 1 item = 1 line segment
* **Path**: 1 item = 1 point in a polyline (multiple disjoint paths can be batched)
* **Image**: 1 item = 1 image
* **Glyph**: 1 item = 1 character, grouped into strings by position
* **Mesh**: 1 item = 1 vertex (connectivity defined by faces, a separate list of indices)
* **Sphere**: 1 item = 1 3D sphere
* **Volume**: 1 item = the full volume (typically only 1)

Items are grouped into batches that share:

* The same **visual type**
* The same **data transformation** (specific to a panel)
* Optional style attributes, either shared (uniform) or per-item

This model is key to Datoviz's performance: many visual instances are submitted in a single GPU draw call, with tight memory layout and no redundant state changes.

---

## Positioning data: 3D NDC

All visuals expect positions in [**3D Normalized Device Coordinates (NDC)**](common.md#positioning-data-ndc-model), where each axis ranges from `-1` to `+1`.

For 2D rendering, simply set the Z coordinate to `0` and use a 2D camera or interaction mode (e.g. pan-zoom). This keeps your data in the XY plane while leveraging the full GPU pipeline.

---

## Working with visuals

All visuals are created using the `app.visual_name()` functions:

```python
visual = app.point(position=..., color=..., size=...)
```

Attributes can also be set or modified as follows:

```python
visual.set_position(position)
visual.set_color(color)
...
```

The `position` is usually an `(N, 3)` NumPy array (automatically cast to `float32`, which the GPU expects), and `color` is an `(N, 4)` array of RGBA values in the 0–255 range, automatically cast to `uint8`.

Visuals must then be added to a panel:

```python
panel.add(visual)
```

For more details, see each visual's documentation page.
