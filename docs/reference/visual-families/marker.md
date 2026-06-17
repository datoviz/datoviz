# Marker

Screen-space code-SDF symbols with per-item shape, fill, stroke, size, and rotation.

Status: supported.
Backends: native; WebGPU live (`marker`, `panzoom`).
Primitive: instanced screen-space quads.

## Use When

Use marker visuals for scatter marks that need categorical symbol shapes, per-item rotation, or
stroke/fill styling while keeping each mark screen-sized.

## Avoid When

Use [Point](point.md) for simple circular marks, [Pixel](pixel.md) for square cells, or
[Glyph](glyph.md) for low-level font-atlas glyph quads.

## Data Model

Create with `dvz_marker(scene, flags)`. Upload one item per marker. Built-in code-SDF shapes cover
the v0.3 marker vocabulary plus target symbols.

## Attributes

Required: `position` (`vec3` center), `color` (RGBA8 fill), `diameter` (`float`, pixels), `angle`
(`float`, radians), `shape` or `symbol` (`uint32_t` built-in symbol id).

Optional: `item_state` for retained hover/selection styling; visual-wide symbol set through
`dvz_marker_set_symbols()`, one built-in symbol through `dvz_marker_set_symbol()`, stroke/fill
style through `dvz_marker_set_style()`, alpha mode, depth test, and transform.

## Picking And Probing

Marker picking reports source marker items. Stroke and shape styling do not create additional user
items.

## Backend Notes

Native and WebGPU paths are active. Marker size is in pixels, so zoom changes item positions but not
screen diameter unless the data is updated.

## Canonical Example

- Source: `examples/c/visuals/marker.c`
- Gallery: [Marker](../../examples/gallery/visuals/visual_marker.md)
- Build: `just example-c visuals/marker`
- Smoke: `./build/examples/c/visuals/marker --png`
- Validation: `smoke+screenshot`
- Agent copy-safe: yes

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use panzoom](../../how-to/use-panzoom.md), [Point](point.md), [Glyph](glyph.md).
