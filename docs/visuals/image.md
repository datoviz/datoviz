# Image Visual

The **Image** visual displays 2D colored or single-channel images (with colormaps) anchored at a position in 3D space. It supports flexible anchoring, sizing in pixels or normalized device coordinates (NDC), rescaling behavior, and optional square or rounded borders.

<figure markdown="span">
![Image visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/image.png)
</figure>

---

## Overview

- Displays RGBA or single-channel images in a 2D panel
- Anchored at a 3D NDC position using a configurable alignment point
- Size can be set in either **pixels** or **NDC units**
- Supports borders, round corners, and rescaling behavior
- Image data can be full color or colormapped from single-channel textures

---

## When to use

Use the image visual when:

- You need to overlay 2D raster data (e.g. camera frames, microscope images)
- You want precise control over alignment and size
- You need to visualize single-channel arrays with a colormap

---

## Attributes

Each item in the visual is a single image. Multiple images can be efficiently displayed at different positions in the same visual.

!!! warning

    Currently, all images in a given visual must share the same texture image, though they can use different texture coordinates.

### Options

| Option        | Type     | Description                                        |
|---------------|----------|----------------------------------------------------|
| `unit`        | `enum`   | Unit of the image size                             |
| `mode`        | `enum`   | Color mode                                         |
| `rescale`     | `enum`   | Rescale mode                                       |

### Per-item

| Attribute     | Type                 | Description                                      |
|---------------|----------------------|--------------------------------------------------|
| `position`    | `(N, 3) float32`     | Anchor point for the image (in NDC coordinates) |
| `size`        | `(N, 2) float32`     | Image width and height                          |
| `anchor`      | `(N, 2) float32`     | Relative point in the image attached to position |
| `texcoords`   | `(N, 4) float32`     | Texture coordinates (default is `(0, 0, 1, 1)`)  |
| `facecolor`   | `(N, 4) uint8`       | Fill color (used in `fill` mode)                |

### Per-visual (uniform)

| Attribute     | Type     | Description                                        |
|---------------|----------|----------------------------------------------------|
| `border`      | `bool`   | Show or hide a border around the image             |
| `edgecolor`   | `cvec4`  | Color of the border edge                           |
| `linewidth`   | `float`  | Width of the border in pixels                      |
| `radius`      | `float`  | Border corner radius (for rounded edges)           |
| `colormap`    | `enum`   | Colormap used in `colormap` mode                   |
| `permutation` | `ivec2`  | Axis swizzle for texture sampling (e.g. `(1, 0)`)  |
| `texture`     | texture  | Texture object (RGBA or single-channel)            |

---

## Image unit

Use the `unit` attribute to specify the unit in which the image sizes are expressed:

| Unit    | Description                                |
|---------|--------------------------------------------|
| `pixels` *(default)* | Size is in framebuffer pixels |
| `ndc`    | Size is relative to panel NDC coordinates |

---

## Color mode

Use the `mode` attribute to define how the image is rendered:

| Mode       | Description                                      |
|------------|--------------------------------------------------|
| `rgba` *(default)* | Full RGBA image                         |
| `colormap` | Single-channel image with a colormap            |
| `fill`     | Fill with uniform `facecolor` (no texture needed) |

---

## Rescale mode

Use the `rescale` attribute to define how the image is resized:

| Mode         | Description                                     |
|--------------|-------------------------------------------------|
| `None` *(default)* | Fixed size, no scaling                    |
| `rescale`    | Image scales with pan-zoom                      |
| `keep_ratio` | Image scales while maintaining its aspect ratio |

---

## Border

Use the `border` attribute to indicate whether a border should be shown. If enabled, the following attributes can be used to customize the image border:

| Attribute   | Description                                   |
|-------------|-----------------------------------------------|
| `edgecolor` | Color of the border                           |
| `linewidth` | Thickness in pixels                           |
| `radius`    | Corner rounding radius                        |

---

## Anchor point

The `anchor` attribute defines which part of the image rectangle is attached to the `position` coordinate in NDC space.

The anchor is specified as a 2D vector in the range `[-1, +1]`.

![Image title](../images/anchor.svg){ width="500" }
/// caption
Image anchor point
///

Here are a few examples:

| Anchor     | Description          |
|------------|----------------------|
| `[-1, -1]` | bottom-left corner   |
| `[0, 0]`   | center of the image  |
| `[+1, +1]` | top-right corner     |

This allows precise placement of the image relative to your coordinate system.

!!! info

    The [anchor feature example](../gallery/features/anchor.md) illustrates how the behavior of the anchor in the **Image** visual.

---

## Texture swizzling

The `permutation` attribute controls how texture axes are interpreted.
For example, `(0, 1)` is the default (UV); `(1, 0)` uses VU instead.

This can be useful when image data is stored with flipped or transposed axes.

---

## Example

```python
--8<-- "examples/visuals/image.py:15:"
```

---

## Summary

The image visual provides flexible placement and styling of 2D textures.

* ✔️ Position, anchor, and size control
* ✔️ Borders, round corners, and rescaling
* ✔️ Support for colormaps and single-channel data

See also:

* [**Pixel**](pixel.md): for sparse or point-based raster plots
* [**Volume**](volume.md): for 3D scalar field visualization
