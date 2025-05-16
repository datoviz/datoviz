# Mesh Visual

The **mesh** visual renders 3D surfaces composed of triangles. It supports flat shading, lighting, texturing, contours, and experimental isolines, making it a powerful and flexible tool for scientific surface visualization.

---

## Overview

- Supports both **indexed** and **non-indexed** triangle geometry
- Can be constructed manually or from predefined 2D/3D shapes
- Optional features include: lighting, texture mapping, wireframe contours, and isolines
- Ideal for surface meshes, geometry visualization, anatomical data, and simulation output

---

## Construction methods

### `app.mesh(...)`

Use this method when supplying raw mesh data manually:

```python
visual = app.mesh(indexed=True, textured=False, lighting=True)
visual.set_data(position=..., color=..., normal=..., index=...)
```

### `app.mesh_shape(...)`

Use this method to create a mesh from a `ShapeCollection`, including:

* Built-in 2D/3D primitives (rectangles, cubes, spheres, etc.)
* Imported OBJ models

This is useful for quick prototyping or geometric rendering.

```python
visual = app.mesh_shape(shape, lighting=True, textured=True)
```

See [Advanced → Shapes](../guide/shape.md) for more on creating `ShapeCollection` instances.

---

## Attributes

### Per-vertex

| Attribute   | Type             | Description                            |
| ----------- | ---------------- | -------------------------------------- |
| `position`  | `(N, 3) float32` | Vertex positions (in NDC)              |
| `normal`    | `(N, 3) float32` | Vertex normals (required for lighting) |
| `color`     | `(N, 4) uint8`   | Vertex color (RGBA)                    |
| `texcoords` | `(N, 4) float32` | Texture coordinates (only if textured) |
| `isoline`   | `(N,) float32`   | Scalar values for isoline rendering    |
| `contour`   | `(N, 4) uint8`   | Optional triangle contour color        |

### Per-mesh (uniform)

| Attribute      | Type     | Description                                    |
| -------------- | -------- | ---------------------------------------------- |
| `index`        | `uint32` | Triangle indices (optional if non-indexed)     |
| `light_dir`    | `vec3`   | Direction of lighting                          |
| `light_color`  | `cvec4`  | Color of the light                             |
| `light_params` | `vec4`   | Lighting coefficients (ambient, diffuse, etc.) |
| `edgecolor`    | `cvec4`  | Color of contour edges                         |
| `linewidth`    | `float`  | Width of contour lines                         |
| `density`      | `int`    | Isoline density                                |
| `texture`      | texture  | Texture used when `textured=True`              |

---

## Lighting

Enable lighting by passing `lighting=True` when creating the mesh. Per-vertex normals are required for lighting to have an effect.

Use the following attributes to adjust lighting behavior:

* `light_dir`: light direction vector
* `light_color`: RGBA color of the light source
* `light_params`: customizable lighting parameters (e.g. ambient, diffuse factors)

Lighting is currently basic (flat/Phong-style). Support for more advanced materials is under development.

---

## Texturing

When `textured=True`, a texture can be applied to the mesh surface.

* Supply `texcoords` per vertex
* Assign a texture using `visual.set_texture(...)`

Texturing is compatible with lighting and contour rendering.

---

## Contours

When `contour=True`, triangle edges are drawn as outlines (wireframe-like effect).

* Use `edgecolor` to define contour color
* Use `linewidth` to define stroke width

Contours help reveal mesh structure and polygon boundaries.

---

## Isolines (experimental)

The mesh visual supports experimental **isoline rendering**, i.e., drawing level curves over the surface.

* Provide a per-vertex scalar field via `isoline`
* Set `density` to control the number of isolines

This feature is still in development and may change in future versions.

---

## Example

```python
--8<-- "examples/visuals/mesh.py"
```

---

## Summary

The mesh visual supports versatile rendering of surface geometry with optional enhancements.

* ✔️ Lighting and normals
* ✔️ Texturing and UV mapping
* ✔️ Wireframe and isolines
* ✔️ Indexed and unindexed triangles

See also:

* [Volume](volume.md) for volumetric fields
* [Image](image.md) for flat texture overlays
* [Shapes](../guide/shape.md) for building reusable geometry
