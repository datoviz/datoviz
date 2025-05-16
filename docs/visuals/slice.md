# Slice Visual

The **slice** visual is designed to render 2D slices extracted from a 3D volume. It is useful for inspecting cross-sections of volumetric data such as MRI, CT, or simulation fields.

---

## Status

The slice visual is implemented in the **Datoviz C library**, but is **not yet available in the Python API**.

Python bindings will be added in a future release. Once available, this visual will support slicing along arbitrary axes with customizable placement, size, and texture coordinates.

---

## Planned features

- Display 2D cross-sections of 3D volumes
- Support for slicing along X, Y, or Z or tilted axes
- Compatible with the same 3D textures used by the [Volume](volume.md) visual

---

## See also

- [Volume](volume.md): full 3D scalar field rendering
- [Image](image.md): for 2D texture display
