# Shape Collections

The `ShapeCollection` is a utility that lets you build complex 2D or 3D geometry by combining predefined shapes, custom meshes, and transformed objects. These collections are typically used with `app.mesh_shape()` to create a mesh visual.

---

## Overview

- Add basic geometric shapes: squares, cubes, spheres, etc.
- Transform each shape independently (offset, scale, 4×4 matrix)
- Load shapes from `.obj` files
- Generate surfaces from height maps or triangulated polygons
- Export to a single GPU mesh with lighting, textures, contours, or isolines

---

## Usage

```python
sc = dvz.ShapeCollection()
sc.add_cube(offset=(0, 0, 0), scale=1.0)
visual = app.mesh_shape(sc, lighting=True)
```

After use, destroy the shape collection to free resources:

```python
sc.destroy()
```

---

## Supported Shapes

| Shape        | Method               | Notes                                 |
| ------------ | -------------------- | ------------------------------------- |
| Square       | `add_square()`       | 2D quad in XY plane                   |
| Disc         | `add_disc()`         | Circular disc with resolution         |
| Sector           | `add_sector()`      | Arc segment between two angles (2D disc slice) |
| Cube         | `add_cube()`         | Axis-aligned 3D cube                  |
| Sphere       | `add_sphere()`       | Specify `rows`, `cols` for resolution |
| Tetrahedron  | `add_tetrahedron()`  | Platonic solid                        |
| Hexahedron   | `add_hexahedron()`   | Cube variant                          |
| Octahedron   | `add_octahedron()`   | Platonic solid                        |
| Dodecahedron | `add_dodecahedron()` | Platonic solid                        |
| Icosahedron  | `add_icosahedron()`  | Platonic solid                        |
| Cone         | `add_cone()`         | Cone with cap                         |
| Cylinder     | `add_cylinder()`     | Cylinder with caps                    |
| Arrow            | `add_arrow()`       | Cylinder + cone composite, total length = 1     |
| Torus            | `add_torus()`       | Donut shape with tubular cross-section          |


---

## Custom Geometry

### Polygon

```python
sc.add_polygon(points, contour='wire', indexing='auto')
```

* `points`: Nx2 array of polygon vertices
* Uses **earcut** triangulation internally
* Contour rendering is experimental

### Surface

```python
sc.add_surface(heights, colors)
```

* Heights: 2D array of Z-values
* Colors: 2D RGBA array (same shape)
* Generates a triangulated surface patch

---

## OBJ Import

Load external 3D geometry from a Wavefront OBJ file:

```python
sc.add_obj("models/teapot.obj", scale=0.5)
```

This loads all vertices and faces into the shape collection.

---

## Transformations

Each shape can be transformed individually:

* `offset`: a 3D translation `(x, y, z)`
* `scale`: a uniform scalar
* `transform`: a 4×4 transformation matrix (overrides offset/scale)

---

## Colors

Each basic shape accepts a `color` argument.

---

## Finalizing

After building your collection, use:

```python
visual = app.mesh_shape(sc, lighting=True, contour=True)
```

And optionally:

```python
sc.merge()     # Optional: flattens internal shape list
sc.destroy()   # Frees memory
```

---

## Example

```python
--8<-- "examples/features/shapes.py"
```

---

## Summary

The ShapeCollection system is a powerful way to build reusable geometry for the `mesh` visual.

* ✔️ Add and transform 2D/3D primitives
* ✔️ Generate and combine triangulated surfaces
* ✔️ Apply contours, colors, lighting
* ❌ Limited support for UVs and textures (for now)

See also:

* [Mesh](../visuals/mesh.md) for rendering options
* [Volume](../visuals/volume.md) for dense scalar fields
