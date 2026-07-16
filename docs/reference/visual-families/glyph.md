# Glyph

Low-level font-atlas glyph quads rendered from signed-distance-field atlas data.

Status: experimental.
Backends: native; WebGPU live (`glyph`, `glyph-atlas`, `sdf`).
Primitive: atlas-textured quads.

## Preview And Links

[![Glyph](../../assets/gallery/v0.4/visuals/visuals_glyph.webp)](../../examples/gallery/visuals/visuals_glyph.md)

- Example: [Glyph](../../examples/gallery/visuals/visuals_glyph.md)
- How-to: [Add text, labels, and annotations](../../how-to/add-annotations.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Text](text.md), [Labels](labels.md), [Marker](marker.md)

## Use When

Use glyph visuals only when you already have shaped glyph positions and atlas coordinates and need
direct control over low-level text rendering.

## Avoid When

Use [Text](text.md) for retained semantic text. Avoid glyph visuals for ordinary annotations,
labels, or user-facing strings unless you are implementing text infrastructure.

## Item And Data Model

Create with `dvz_glyph(scene, flags)`. The low-level draw contract is vertex-oriented: the
canonical example expands each shaped glyph to six rows forming two triangles. Bind one font atlas
with `dvz_glyph_set_atlas()`. The atlas is borrowed from a scene-owned font and must outlive the
glyph visual.

## Attribute And Resource Contract

| Attribute/resource | Requirement/default | C type and cardinality | Python dtype and shape | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `position` | Required; no documented default | `vec3[V]` | `float32`, `(V, 3)` | Repeated glyph anchor in authored visual coordinates; canonical quads use six rows per glyph. | Dense or range upload. |
| `bounds` | Required; no documented default | `vec4[V]` | `float32`, `(V, 4)` | Local glyph rectangle `(x0, y0, x1, y1)` in logical pixels, repeated for each generated quad vertex. | Dense or range upload. |
| `texcoords` | Required; no documented default | `vec4[V]` | `float32`, `(V, 4)` | Normalized atlas UV bounds `(u0, v0, u1, v1)`, repeated per quad vertex. | Dense or range upload. |
| `color` | Required; no documented default | `DvzColor[V]` | `uint8`, `(V, 4)` | RGBA8; vertices belonging to one glyph normally share a color. | Dense or range upload. |
| `angle` | Required; no documented default | `float[V]` | `float32`, `(V,)` | Radians, positive counter-clockwise in rendered y-up coordinates around the glyph anchor. | Dense or range upload. |
| atlas | Required | borrowed `const DvzTextAtlas*` | ctypes atlas handle | Atlas encoding and distance range configure the shader. Must belong to the same scene and remain alive. | `dvz_glyph_set_atlas()`. |

## Constructor And Options

- Constructor: `dvz_glyph(scene, flags)`; the canonical example passes `flags = 0`.
- Create/ensure an atlas through `dvz_font_atlas_ensure_string()` or related font APIs, then borrow
  it with `dvz_font_atlas()`.
- Alpha blending and disabled depth testing are appropriate for the canonical annotation route but
  are explicit visual-wide choices.
- There is no canonical Python gallery example for manual glyph shaping; use [Text](text.md) in
  normal Python code.

## Verified Usage Pattern

Shape text externally, expand each glyph to six matching attribute rows, upload all five arrays,
bind the atlas, and attach the visual. See the
[canonical C example](../../examples/gallery/visuals/visuals_glyph.md).

## Picking And Probing

Glyph items correspond to shaped glyph quads, not text strings or logical characters. Use semantic
text objects when interaction should be string-based.

`angle` rotates counter-clockwise in rendered y-up coordinates around the glyph anchor.

## Backend Notes

Native and WebGPU paths are active for atlas-backed glyph rendering. The family is experimental and
the canonical example is intended for low-level text work rather than as a first text example.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/glyph.c` |
| Gallery | [Font Atlas Glyphs](../../examples/gallery/visuals/visuals_glyph.md) |
| Build | `just example-c visuals/glyph` |
| Smoke | `./build/examples/c/visuals/glyph --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[add annotations](../../how-to/add-annotations.md), [Text](text.md).
