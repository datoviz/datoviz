# Mesh Visual

The **Mesh** visual renders 3D surfaces composed of triangles. It supports flat shading, lighting, texturing, contours, and experimental isolines.

<figure markdown="span">
![Mesh visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/mesh.png)
</figure>

---

## Overview

- Supports both **indexed** and **non-indexed** triangle geometry
- Can be constructed manually or from predefined 2D/3D shapes
- Optional features include: lighting, texture mapping, wireframe contours, and isolines
- Ideal for surface meshes, geometry visualization, anatomical data, and simulation output

---

## Construction

There are two ways to create a mesh visual:

* Supply raw mesh data manually with `visual = app.mesh(position=..., color=..., normal=..., index=...)`
* Supply a `ShapeCollection` with `visual = app.mesh(shape_collection, ...)`.

A `ShapeCollection` can be created with:

* Built-in 2D/3D primitives (rectangles, cubes, spheres, etc.)
* OBJ models

See [Advanced → Shapes](../guide/shape.md) for more on creating `ShapeCollection` instances.

---

## Attributes

### Options

| Option        | Type     | Description                                        |
|---------------|----------|----------------------------------------------------|
| `indexed`     | `bool`   | Whether the mesh uses indexing                     |
| `textured`    | `bool`   | Whether to use a texture for rendering             |
| `lighting`    | `bool`   | Whether to use lighting                            |
| `contour`     | `bool`   | Whether to show contour or wireframes              |
| `isoline`     | `bool`   | Whether to show isolines                           |

### Per-item

| Attribute   | Type             | Description                            |
| ----------- | ---------------- | -------------------------------------- |
| `position`  | `(N, 3) float32` | Vertex positions (in NDC)              |
| `normal`    | `(N, 3) float32` | Vertex normals (required for lighting) |
| `color`     | `(N, 4) uint8`   | Vertex color (RGBA)                    |
| `texcoords` | `(N, 4) float32` | Texture coordinates (only if textured) |
| `isoline`   | `(N,) float32`   | Scalar values for isoline rendering    |
| `contour`   | `(N, 4) uint8`   | Optional triangle contour color        |

### Index buffer

When `indexed=True`, the optional `index` argument is a 1D array of `uint32` values, where each group of three consecutive integers represents indices into the `position` array, defining the vertices of a triangle.

### Per-visual (uniform)

| Attribute      | Type     | Description                                    |
| -------------- | -------- | ---------------------------------------------- |
| `light_pos`    | `vec4`   | Light position/direction                       |
| `light_color`  | `cvec4`  | Light color                                    |
| `material_params` | `vec4`| Material parameters                            |
| `shine`        | `float`  | Shine value                                    |
| `emit`         | `float`  | Emission value                                 |
| `edgecolor`    | `cvec4`  | Color of contour edges                         |
| `linewidth`    | `float`  | Width of contour lines                         |
| `density`      | `int`    | Isoline density                                |
| `texture`      | texture  | Texture used when `textured=True`              |

---

## Texturing

When `textured=True`, a texture can be applied to the mesh surface.

* Supply `texcoords` per vertex
* Assign a texture using `visual.set_texture(...)`

Texturing is compatible with lighting and contour rendering.

---

## Lighting

Enable lighting by passing `lighting=True` when creating the mesh. Per-vertex normals are required for lighting to have an effect.

Currently, up to four different lights are supported.

### Light position and direction

This can represent either the 3D position of a point light (`w=1`) or the 3D direction of a directional light (`w=0`), depending on the fourth component `w` of the `vec4` vector.

```python
visual.set_light_pos(pos, index=0)  # index=0..3 is the light index
```

### Light color

This is the RGBA color of the light source.

```python
visual.set_light_color(rgba, index=0)  # index=0..3 is the light index
```

### Material

These are the mesh material RGB values, for four different sets of parameters:

| Index      | Parameter     | Description                                    |
| -------------- | -------- | ---------------------------------------------- |
| 0 | ambient | ambient |
| 1 | diffuse | diffuse |
| 2 | specular | specular |
| 3 | exponent | exponent |


```python
visual.set_material_params(rgb, index=0)  # index=0..3 is the material type index
```

Additional parameters are:

```python
visual.set_shine(value)
visual.set_amit(value)
```

!!! warning

    This section of the documentation is not yet complete.

!!! note

    Lighting is currently basic (flat/Phong-style). Support for more advanced materials is under development.

---

## Contour

When `contour=True`, triangle edges are drawn as outlines (wireframe-like effect).

* Use `edgecolor` to define contour color
* Use `linewidth` to define stroke width

Contours help reveal mesh structure and polygon boundaries.

!!! note

    Contours can be set per triangle or for a subset of triangles, such as quads or polygon boundaries. This feature is not fully documented yet.

---

## Isoline (experimental)

The mesh visual supports experimental **isoline rendering**, i.e., drawing level curves over the surface.

* Provide a per-vertex scalar field via `isoline`
* Set `density` to control the number of isolines

This feature is still in development and may change in future versions.

---

## Example

```python
--8<-- "examples/visuals/mesh.py:15:"
```

---

## Summary

The mesh visual supports versatile rendering of surface geometry with optional enhancements.

* ✔️ Lighting and normals
* ✔️ Texturing and UV mapping
* ✔️ Wireframe and isolines
* ✔️ Indexed and unindexed triangles

See also:

* [**Volume**](volume.md) for volumetric fields
* [**Image**](image.md) for flat texture overlays
* [**Shapes**](../guide/shape.md) for building reusable geometry
