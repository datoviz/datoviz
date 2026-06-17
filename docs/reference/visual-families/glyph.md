# Glyph

Low-level font-atlas glyph quads rendered from signed-distance-field atlas data.

Status: experimental.
Backends: native; WebGPU live (`glyph`, `glyph-atlas`, `sdf`).
Primitive: atlas-textured quads.

## Use When

Use glyph visuals only when you already have shaped glyph positions and atlas coordinates and need
direct control over low-level text rendering.

## Avoid When

Use [Text](text.md) for retained semantic text. Avoid glyph visuals for ordinary annotations,
labels, or user-facing strings unless you are implementing text infrastructure.

## Data Model

Create with `dvz_glyph(scene, flags)`. Upload one item per glyph quad and bind a font atlas with
`dvz_glyph_set_atlas()`. The atlas remains owned by the font's scene and must outlive the glyph
visual.

## Attributes

Required: `position` (`vec3` anchor), `bounds` (`vec4` local pixel bounds), `texcoords` (`vec4`
atlas UV bounds), `color` (RGBA8), `angle` (`float`, radians), font atlas.

Optional: alpha mode, depth test, transform, and visual-wide scale bindings.

## Picking And Probing

Glyph items correspond to shaped glyph quads, not text strings or logical characters. Use semantic
text objects when interaction should be string-based.

## Backend Notes

Native and WebGPU paths are active for atlas-backed glyph rendering. The family is experimental and
the canonical example is not agent copy-safe.

## Canonical Example

- Source: `examples/c/visuals/glyph.c`
- Gallery: [Font Atlas Glyphs](../../examples/gallery/visuals/visual_glyph.md)
- Build: `just example-c visuals/glyph`
- Smoke: `./build/examples/c/visuals/glyph --png`
- Validation: `smoke+screenshot`
- Agent copy-safe: no

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[add annotations](../../how-to/add-annotations.md), [Text](text.md).
