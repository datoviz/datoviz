# Marker Visual

The **Marker** visual is a versatile and customizable point visual that supports symbolic shapes, rotation, borders, and texture-based rendering. It is ideal for labeled scatter plots, categorized data points, or any visualization that needs distinct marker styles.

<figure markdown="span">
![Marker visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/marker.png)
</figure>

---

## Overview

- Renders **symbolic shapes** (e.g., disc, triangle, square)
- Supports **custom SVGs**, **bitmap textures**, and **SDF/MSDF** formats
- Each marker has per-vertex position, color, size, and rotation
- Can be **filled**, **bordered**, or **outlined-only**
- Supports uniform control of border width, edge color, and texture scaling

!!! note

    Currently, all markers within a single visual share the same shape. To display multiple shapes, use a separate visual for each marker type and group the points accordingly.

---

## When to use

Use the marker visual when:

- You want distinct shapes for different data categories
- You need rotation, border, or outline styling per marker
- You want to use custom textures (SVG, bitmap, SDF/MSDF)
- You need high flexibility for scatter or symbol-based plots

---

## Attributes

### Per-vertex

| Attribute  | Type             | Description                          |
|------------|------------------|--------------------------------------|
| `position` | `(N, 3) float32` | Marker position in NDC               |
| `color`    | `(N, 4) uint8`   | Fill color per marker                |
| `size`     | `(N,) float32`   | Diameter in framebuffer pixels       |
| `angle`    | `(N,) float32`   | Rotation angle in radians            |

### Uniform

| Attribute   | Type     | Description                                           |
|-------------|----------|-------------------------------------------------------|
| `shape`     | enum     | Marker shape (only used when `mode='code'`)          |
| `mode`      | enum     | Rendering mode (`code`, `bitmap`, `sdf`, `msdf`)     |
| `aspect`    | enum     | Aspect ratio behavior                                |
| `linewidth` | float    | Outline width in pixels                              |
| `edgecolor` | cvec4    | Outline color (applied uniformly)                    |
| `tex_scale` | float    | Global scaling factor for all markers                |
| `texture`   | texture  | Texture object for bitmap/SDF/MSDF modes             |

---

## Rendering modes

The visual supports four rendering modes:

| Mode     | Description                                     |
|----------|-------------------------------------------------|
| `code`   | Uses GPU-coded built-in shapes (fastest)        |
| `bitmap` | Uses a bitmap texture                           |
| `sdf`    | Uses a signed distance field texture            |
| `msdf`   | Uses a multichannel signed distance field        |

!!! note

    For SDF and MSDF, Datoviz bundles the [Multi-channel signed distance field generator C++ library](https://github.com/Chlumsky/msdfgen).

### Code

The following predefined marker shapes, implemented on the GPU, are supported when using `mode='code'`:

| Marker | Value | Image |
| ---- | ---- | ---- |
| `disc` | 0 | ![marker_disc](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_disc.png) |
| `asterisk` | 1 | ![marker_asterisk](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_asterisk.png) |
| `chevron` | 2 | ![marker_chevron](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_chevron.png) |
| `clover` | 3 | ![marker_clover](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_clover.png) |
| `club` | 4 | ![marker_club](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_club.png) |
| `cross` | 5 | ![marker_cross](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_cross.png) |
| `diamond` | 6 | ![marker_diamond](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_diamond.png) |
| `arrow` | 7 | ![marker_arrow](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_arrow.png) |
| `ellipse` | 8 | ![marker_ellipse](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_ellipse.png) |
| `hbar` | 9 | ![marker_hbar](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_hbar.png) |
| `heart` | 10 | ![marker_heart](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_heart.png) |
| `infinity` | 11 | ![marker_infinity](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_infinity.png) |
| `pin` | 12 | ![marker_pin](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_pin.png) |
| `ring` | 13 | ![marker_ring](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_ring.png) |
| `spade` | 14 | ![marker_spade](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_spade.png) |
| `square` | 15 | ![marker_square](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_square.png) |
| `tag` | 16 | ![marker_tag](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_tag.png) |
| `triangle` | 17 | ![marker_triangle](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_triangle.png) |
| `vbar` | 18 | ![marker_vbar](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_vbar.png) |

### Bitmap

A bitmap texture with the marker to render.

### SDF

The SDF mode (Signed Distance Field) encodes the distance from the marker's outline in a single channel. This allows for resolution-independent rendering with crisp edges and efficient antialiasing.

### MSDF

The MSDF mode (Multi-channel Signed Distance Field) encodes distance separately in RGB channels, enabling sharper rendering of complex shapes (e.g. icons) with fewer artifacts.

!!! warning

    The documentation for the rendering modes above is incomplete. Contributions are welcome.

---

## Aspect

The `aspect` attribute controls how each marker is rendered, using `aspect='filled'` for example:

| Aspect value | Description                                    | Image |
|--------------|------------------------------------------------|-------|
| `filled`     | Fill only, no border (default)                 | ![marker aspect filled](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_filled.png) |
| `stroke`     | Border only, transparent interior              | ![marker aspect stroke](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_stroke.png) |
| `outline`    | Fill with border (filled + stroke combined)    | ![marker aspect outline](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/marker_outline.png) |

These aspects are customized with the following properties:

- `color`: fill color (per marker)
- `edgecolor`: outline color (uniform)
- `linewidth`: outline thickness (uniform, in pixels)

---

## Example

```python
--8<-- "examples/visuals/marker.py:14:"
```

---

## Summary

The marker visual combines flexibility, performance, and clarity for symbolic point visualization.

* ✔️ Custom shapes and rotation
* ✔️ Support for vector or bitmap textures
* ✔️ Per-marker styling and global controls
* ❌ Not intended for ultra-dense clouds (use [**Point**](point.md) or [**Pixel**](pixel.md) for that)

See also:

* [**Point**](point.md): simple discs with per-point size and color
* [**Basic**](basic.md): raw primitives without symbolic shapes
