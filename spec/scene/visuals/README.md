# Scene Visual Family Specs

This directory contains per-family data contracts for the v0.4 visual families.

Each document defines:

1. per-item attribute schema (field name, type, accepted sources, mutability),
2. visual-wide parameters (not per-item),
3. color mode or other variant axes,
4. transform model,
5. stage participation and picking model,
6. fallback notes,
7. relationship to v0.3.

These documents refine `../semantics/VISUAL_FAMILY_RULES.md` and `../semantics/VISUAL_CONTRACT.md` with enough concrete
detail to implement or validate each family.


## Status

| Family | Spec |
|---|---|
| `pixel` | [PIXEL.md](PIXEL.md) |
| `primitive` | [PRIMITIVE.md](PRIMITIVE.md) |
| `point` | [POINT.md](POINT.md) |
| `marker` | [MARKER.md](MARKER.md) |
| `segment` | [SEGMENT.md](SEGMENT.md) |
| `path` | [PATH.md](PATH.md) |
| `glyph` | [GLYPH.md](GLYPH.md) |
| `image` | [IMAGE.md](IMAGE.md) |
| `mesh` | [MESH.md](MESH.md) |
| `sphere` | [SPHERE.md](SPHERE.md) |
| `volume` | [VOLUME.md](VOLUME.md) |
| `errorbar` | [ERRORBAR.md](ERRORBAR.md) |
| `boxplot` | [BOXPLOT.md](BOXPLOT.md) |


## Reading Order

Read `../pipeline/ATTRIBUTE_SOURCES.md` in the parent directory before reading any family spec.
The granularity vocabulary (`CONSTANT`, `PER_ITEM`, `PER_SPAN`, `PER_GROUP`) and mutability hints
(`static`, `dynamic`, `streaming`) are used throughout.


## Active Proposal Inputs

1. [../proposals/MESH_API_DESIGN.md](../proposals/MESH_API_DESIGN.md)
2. [../proposals/MESH_SHADING_DESIGN.md](../proposals/MESH_SHADING_DESIGN.md)
3. [../proposals/MATERIAL_LIGHTING_API.md](../proposals/MATERIAL_LIGHTING_API.md)
4. [../proposals/VOLUME_DESIGN.md](../proposals/VOLUME_DESIGN.md)
