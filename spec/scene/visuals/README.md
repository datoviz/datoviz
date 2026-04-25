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

These documents refine `VISUAL_MINI_CONTRACTS.md` and `VISUAL_CONTRACT.md` with enough concrete
detail to implement or validate each family.


## Status

| Family | Spec |
|---|---|
| `pixel` | [PIXEL.md](PIXEL.md) |
| `primitive` | [PRIMITIVE.md](PRIMITIVE.md) |
| `point` | [POINT.md](POINT.md) |
| `marker` | [MARKER.md](MARKER.md) |
| `segment` | [SEGMENT.md](SEGMENT.md) |
| `point` | planned |
| `marker` | planned |
| `segment` | planned |
| `path` | planned |
| `glyph` | planned |
| `image` | planned |
| `mesh` | planned |
| `sphere` | planned |
| `volume` | planned |


## Reading Order

Read `ATTRIBUTE_SOURCES.md` in the parent directory before reading any family spec.
The granularity vocabulary (`CONSTANT`, `PER_ITEM`, `PER_GROUP`) and mutability hints
(`static`, `dynamic`, `streaming`) are used throughout.
