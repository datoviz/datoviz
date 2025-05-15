# Marker Visual

The **marker** visual is a versatile and customizable point visual that supports symbolic shapes, rotation, borders, and texture-based rendering. It is ideal for labeled scatter plots, categorized data points, or any visualization that needs distinct marker styles.

---

## Overview

- Renders **symbolic shapes** (e.g., disc, triangle, square)
- Supports **custom SVGs**, **bitmap textures**, and **SDF/MSDF** formats
- Each marker has per-vertex position, color, size, and rotation
- Can be **filled**, **bordered**, or **outlined-only**
- Supports uniform control of border width, edge color, and texture scaling

---

## When to use

Use the marker visual when:
- You want distinct shapes for different data categories
- You need rotation, border, or outline styling per marker
- You want to use custom textures (SVG, bitmap, SDF/MSDF)
- You need high flexibility for scatter or symbol-based plots

---

## Built-in shapes

The following predefined marker shapes are supported when using `mode='code'`:

- disc (default)
- arrow
- asterisk
- chevron
- clover
- club
- cross
- diamond
- ellipse
- hbar
- heart
- infinity
- pin
- ring
- spade
- square
- tag
- triangle
- vbar
- rounded rect

These shapes are rendered on the GPU and are efficient for large datasets.

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

## Marker modes

The visual supports four rendering modes:

| Mode     | Description                                     |
|----------|-------------------------------------------------|
| `code`   | Uses GPU-coded built-in shapes (fastest)        |
| `bitmap` | Uses a bitmap texture                           |
| `sdf`    | Uses a signed distance field texture            |
| `msdf`   | Uses a multichannel signed distance field        |

Use `set_mode("bitmap")`, `set_mode("sdf")`, or `set_mode("msdf")` when using custom textures.

### Texture types: SDF and MSDF

When using textured markers, you can supply custom shapes using:

- **SDF (Signed Distance Field)**: encodes the distance from the marker's outline in a single channel. This allows for resolution-independent rendering with crisp edges and efficient antialiasing.
- **MSDF (Multi-channel Signed Distance Field)**: encodes distance separately in RGB channels, enabling sharper rendering of complex shapes (e.g. icons) with fewer artifacts.

Datoviz bundles the [Multi-channel signed distance field generator C++ library](https://github.com/Chlumsky/msdfgen).


---

## Fill and Outline Modes

The `aspect` attribute controls how each marker is rendered:

| Aspect value | Description                                    |
|--------------|------------------------------------------------|
| `filled`     | Fill only, no border (default)                 |
| `stroke`     | Border only, transparent interior              |
| `outline`    | Fill with border (filled + stroke combined)    |

This affects how `color`, `edgecolor`, and `linewidth` are interpreted:

- `color`: fill color (per marker)
- `edgecolor`: outline color (uniform)
- `linewidth`: outline thickness (uniform, in pixels)

### Examples:

```python
visual.set_aspect('filled')
visual.set_aspect('stroke')
visual.set_aspect('outline')
```

---

## Example

```python
--8<-- "examples/visuals/marker.py"
```

---

## Summary

The marker visual combines flexibility, performance, and clarity for symbolic point visualization.

* ✔️ Custom shapes and rotation
* ✔️ Support for vector or bitmap textures
* ✔️ Per-marker styling and global controls
* ❌ Not intended for ultra-dense clouds (use [Point](point.md) or [Pixel](pixel.md) for that)

See also:

* [Point](point.md): simple discs with per-point size and color
* [Basic](basic.md): raw primitives without symbolic shapes
