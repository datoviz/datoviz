# Glyph Visual

The **glyph** visual provides low-level, high-performance text rendering using individual positioned glyphs. It supports fine control over placement, scaling, color, and layout — ideal for custom labeling, annotations, or scientific overlays where flexibility matters.

This visual operates at the glyph level and can render multiple strings using groups of glyphs.

This visual is currently implemented with multichannel signed distance fields

<figure markdown="span">
![Glyph visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/glyph.png)
</figure>

---

## Overview

- Renders glyphs as positioned 2D quads in 3D NDC space
- Supports per-glyph: color, size, position, shift, rotation, scaling
- Strings are created by grouping glyphs with the same position
- Anchoring is relative to the entire string, not each glyph
- Highly flexible, low-level control — not a high-level text API
- Uses a builtin texture font atlas — font and atlas customization will be implemented and documented later

---

## When to use

Use the glyph visual when:
- You want fine-grained control over text rendering (e.g. per-character styling)
- You need rotated, scaled, or colored strings at arbitrary 3D positions
- You’re rendering structured labels, not paragraphs

---

## Attributes

### Per-glyph

| Attribute     | Type               | Description                                           |
|----------------|--------------------|-------------------------------------------------------|
| `position`     | `(N, 3) float32`   | Anchor position for each glyph (in NDC)              |
| `axis`         | `(N, 3) float32`   | Not implemented yet                                  |
| `size`         | `(N, 2) float32`   | Glyph size in framebuffer pixels                     |
| `shift`        | `(N, 2) float32`   | Offset of each glyph in framebuffer pixels (layout)  |
| `texcoords`    | `(N, 4) float32`   | Texture UV for each glyph                            |
| `group_size`   | `(N, 2) float32`   | Size of the full string the glyph belongs to         |
| `scale`        | `(N,) float32`     | Per-glyph scaling factor                             |
| `angle`        | `(N,) float32`     | Rotation angle (radians) per glyph                   |
| `color`        | `(N, 4) uint8`     | Glyph fill color                                     |

### Uniform

| Attribute   | Type    | Description                       |
|-------------|---------|-----------------------------------|
| `bgcolor`   | `vec4`  | Background color behind each glyph |
| `texture`   | texture | Glyph atlas texture               |

---

## Grouping and string layout

Glyphs are grouped into **strings** by sharing the same anchor `position`.
They are placed using:

- `shift`: pixel offset of the glyph relative to its anchor
- `size`: glyph quad size in pixels
- `group_size`: size of the entire string, used for anchor alignment

Anchoring is applied relative to the full string, not each glyph — for example, `anchor = [0, 0]` centers the full string at the anchor point.

---

## Helper: `set_strings()`

To simplify text rendering, use `set_strings()` to define multiple strings at once:

```python
visual.set_strings(
    strings=['Hello', 'World'],
    string_pos=positions,      # (M, 3) array of anchor points
    scales=scales,             # (M,) array of scale factors
    color=(255, 255, 255, 255),  # RGBA
    anchor=(0, 0),             # center anchor
    offset=(0, 0)              # optional pixel offset
)
```

After calling `set_strings()`, you can still customize glyph-level attributes using:

```python
visual.set_color(per_glyph_colors)
visual.set_angle(per_glyph_angles)
...
```

This provides an efficient way to define structured text and then refine it per glyph if needed.

---

## Font and MSDF Rendering

Datoviz uses **MSDF (Multi-channel Signed Distance Field)** rendering for high-quality, scalable glyph rendering. A pre-generated MSDF glyph atlas is bundled with the library.

- The default font is **Roboto**
- The atlas is generated using [msdfgen](https://github.com/Chlumsky/msdfgen)
- When calling `app.glyph()`, you can specify the `font_size` keyword to select the glyph atlas resolution

```python
visual = app.glyph(font_size=30)
```

---

## Example

```python
--8<-- "examples/visuals/glyph.py:15:"
```

---

## Summary

The glyph visual gives you total control over text rendering at the glyph level.

* ✔️ Precise 2D/3D placement and styling
* ✔️ Per-glyph color, rotation, shift, and size
* ✔️ Grouping into strings with anchor-aware layout
* ❌ No automatic layout, alignment, or wrapping

See also:

* [Image](image.md) for 2D raster overlays
* [Marker](marker.md) for symbolic shapes with rotation
