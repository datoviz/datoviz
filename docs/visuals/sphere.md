# Sphere Visual

The **Sphere** visual renders 3D spheres using GPU impostors — efficient 2D quads that simulate shaded spheres in the fragment shader using raymarching. This allows rendering of thousands of spheres with realistic lighting and minimal geometry overhead.

<figure markdown="span">
![Sphere visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/sphere.png)
</figure>

---

## Overview

- Each sphere is a screen-aligned quad rendered as a shaded 3D sphere
- Positions are in 3D NDC space
- Sizes are specified in **NDC units** (not pixels)
- Lighting parameters are customizable per visual (light support will be improved in a future version)

---

## When to use

Use the sphere visual when:
- You want efficient rendering of thousands of 3D spheres
- You don't need true mesh geometry (no collisions or wireframes)
- You want adjustable lighting and shading

---

## Attributes

### Per-item

| Attribute  | Type             | Description                              |
|------------|------------------|------------------------------------------|
| `position` | `(N, 3) float32` | Center of the sphere (in NDC)            |
| `color`    | `(N, 4) uint8`   | RGBA color                               |
| `size`     | `(N,) float32`   | Diameter in NDC units                    |

### Per-visual (uniform)

| Attribute       | Type   | Description                                            |
|------------------|--------|--------------------------------------------------------|
| `light_pos`      | `vec3` | Position of the light source                          |
| `light_params`   | `vec4` | Lighting parameters (diffuse, ambient, specular, shininess) |

---

## Lighting

Spheres are shaded in 3D using basic lighting calculations. You can adjust:

- **`light_pos`**: position of the light in world space
- **`light_params`**: a tuple of floats controlling:
  - Diffuse strength
  - Ambient strength
  - Specular strength
  - Shininess (exponent)

These parameters control how the spheres react to light, giving the impression of 3D volume.

Lighting support will be improved soon, with proper material support.

---

## Example

```python
--8<-- "examples/visuals/sphere.py:14:"
```

---

## Summary

The sphere visual provides efficient, realistic rendering of many shaded spheres using fragment-shader raymarching.

* ✔️ High performance, low geometry cost
* ✔️ Adjustable lighting and shading
* ✔️ Ideal for molecular visualization, 3D scatter plots
* ❌ No physical mesh geometry or edge outlines

See also:

* [**Point**](point.md) for flat, 2D discs
* [**Marker**](marker.md) for symbolic shapes with borders
* [**Mesh**](mesh.md) for full 3D geometry
