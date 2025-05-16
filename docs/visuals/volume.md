# Volume Visual

The **volume** visual displays 3D scalar fields using ray-based volume rendering. It supports NDC-aligned bounding boxes, axis permutation, basic colormapping, and limited transfer functions.

⚠️ **Note:** Volume rendering is currently a basic implementation. It is neither highly efficient nor visually polished yet. Slicing, advanced transfer functions, and performance improvements are planned for future versions.

---

## Overview

- Renders 3D scalar data in a box defined by NDC bounds
- Basic support for colormapping and transfer functions
- Texture-based rendering with customizable axis permutation
- Useful for initial previews of volumetric data

---

## When to use

Use the volume visual when:
- You want to visualize 3D data like MRI, CT, simulations
- You need a fast volumetric rendering preview
- You are working within NDC and want basic shading support

---

## Attributes

### Per-volume

| Attribute     | Type             | Description                                           |
|---------------|------------------|-------------------------------------------------------|
| `bounds`      | 3 × `(2,) float` | Bounding box in NDC: `xlim`, `ylim`, `zlim`          |
| `texcoords`   | 2 × `(3,) float` | UVW texture coordinates at volume corners            |
| `permutation` | `(3,) int`       | Axis order of the 3D volume texture (default `(0,1,2)`) |
| `slice`       | `int`            | Slice index (**not implemented yet**)                |
| `transfer`    | `vec4`           | Transfer function parameters (limited)               |
| `texture`     | texture          | 3D volume data (e.g. density, intensity)             |

---

## Creation

To create a volume visual:

```python
visual = app.volume(mode='colormap')
```

Supported modes:

| Mode       | Description                                     |
| ---------- | ----------------------------------------------- |
| `colormap` | Apply a colormap to a single-channel 3D texture |
| `mip`      | Maximum intensity projection (**planned**)      |

---

## Bounds and texture mapping

You must define the bounds of the volume in NDC:

```python
visual.set_bounds(xlim, ylim, zlim)
```

Then set the texture coordinates:

```python
visual.set_texcoords(uvw0, uvw1)
```

This maps the 3D texture onto the NDC bounding box.

---

## Axis permutation

To reorder the axes of the 3D texture (e.g. for `ZYX`-ordered data), set:

```python
visual.set_permutation((2, 1, 0))  # wvu
```

This ensures correct alignment of the volume with your data orientation.

---

## Transfer function

You can supply a transfer function parameter (currently very limited):

```python
visual.set_transfer((0.3, 0.5, 0.8, 1.0))  # placeholder values
```

More expressive transfer functions (e.g. opacity maps, histograms) are planned.

---

## Example

```python
--8<-- "examples/visuals/volume.py"
```

---

## Summary

The volume visual provides a starting point for 3D scalar field rendering.

* ✔️ Bounding box in NDC
* ✔️ Texture mapping with colormap
* ✔️ Axis reordering support
* ❌ No slicing or rich transfer functions (yet)

See also:

* [Slice](slice.md): for 2D views of 3D data
* [Mesh](mesh.md): for explicit 3D geometry
