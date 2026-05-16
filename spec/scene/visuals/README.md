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


## Active Implementation Status

| Family | Spec | Active implementation status |
|---|---|---|
| `pixel` | [PIXEL.md](PIXEL.md) | active retained visual |
| `primitive` | [PRIMITIVE.md](PRIMITIVE.md) | active retained visual |
| `point` | [POINT.md](POINT.md) | active retained visual |
| `marker` | [MARKER.md](MARKER.md) | spec only |
| `segment` | [SEGMENT.md](SEGMENT.md) | spec only |
| `path` | [PATH.md](PATH.md) | active first slice as line/strip |
| `glyph` | [GLYPH.md](GLYPH.md) | spec only; text bookkeeping exists above the visual slice |
| `image` | [IMAGE.md](IMAGE.md) | active retained visual with `SampledField` binding |
| `mesh` | [MESH.md](MESH.md) | active retained visual |
| `sphere` | [SPHERE.md](SPHERE.md) | active impostor visual |
| `volume` | [VOLUME.md](VOLUME.md) | active first slice with sampled-field-backed volume state |
| `errorbar` | [ERRORBAR.md](ERRORBAR.md) | spec only |
| `boxplot` | [BOXPLOT.md](BOXPLOT.md) | spec only |


## Reading Order

Read `../pipeline/ATTRIBUTE_SOURCES.md` in the parent directory before reading any family spec.
The granularity vocabulary (`CONSTANT`, `PER_ITEM`, `PER_SPAN`, `PER_GROUP`) and mutability hints
(`static`, `dynamic`, `streaming`) are used throughout.


## Active Proposal Inputs

1. [../proposals/MESH_API_DESIGN.md](../proposals/MESH_API_DESIGN.md)
2. [../proposals/MESH_SHADING_DESIGN.md](../proposals/MESH_SHADING_DESIGN.md)
3. [../proposals/MATERIAL_LIGHTING_API.md](../proposals/MATERIAL_LIGHTING_API.md)
4. [../proposals/VOLUME_DESIGN.md](../proposals/VOLUME_DESIGN.md)
