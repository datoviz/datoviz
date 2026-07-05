# Marker

Screen-space code-SDF symbols with per-item shape, fill, stroke, size, and rotation.

Status: supported.
Backends: native; WebGPU live (`marker`, `panzoom`).
Primitive: instanced screen-space quads.

## Preview And Links

[![Marker](../../assets/gallery/v0.4/visuals/visual_marker.webp)](../../examples/gallery/visuals/visual_marker.md)

- Example: [Marker](../../examples/gallery/visuals/visual_marker.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Point](point.md), [Pixel](pixel.md), [Glyph](glyph.md), [Text](text.md)

## Use When

Use marker visuals for scatter marks that need categorical symbol shapes, per-item rotation, or
stroke/fill styling while keeping each mark screen-sized.

## Avoid When

Use [Point](point.md) for simple circular marks, [Pixel](pixel.md) for square cells, or
[Glyph](glyph.md) for low-level font-atlas glyph quads.

## Data Model

Create with `dvz_marker(scene, flags)`. Upload one item per marker. Built-in code-SDF shapes cover
the v0.3 marker vocabulary plus target symbols. `position` is the center of the marker's
screen-space bounding box, and `diameter_px` is that box's width and height in pixels.

## Attributes

| Kind | Attributes |
| --- | --- |
| Required | `position` (`vec3` center), `color` (RGBA8 fill), `diameter_px` (`float`, pixels), `angle` (`float`, radians), `shape` or `symbol` (`uint32_t` built-in symbol id) |
| Optional | `item_state` for retained hover/selection styling; visual-wide symbol set through `dvz_marker_set_symbols()`; one built-in symbol through `dvz_marker_set_symbol()`; stroke/fill style through `dvz_marker_set_style()`; alpha mode; depth test; transform |

`angle` rotates counter-clockwise in rendered y-up coordinates around the bounding-box center.
`DVZ_MARKER_SHAPE_TRIANGLE` at `angle = 0` has marker-local vertices `(-1, -1)`, `(+1, -1)`,
and `(0, +1)`, matching Matplotlib's `^` marker up to scale.

## Picking And Probing

Marker picking reports source marker items. Stroke and shape styling do not create additional user
items.

## Backend Notes

Native and WebGPU paths are active. Marker size is in pixels, so zoom changes item positions but not
screen diameter_px unless the data is updated.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/marker.c` |
| Gallery | [Marker](../../examples/gallery/visuals/visual_marker.md) |
| Build | `just example-c visuals/marker` |
| Smoke | `./build/examples/c/visuals/marker --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use panzoom](../../how-to/use-panzoom.md), [Point](point.md), [Glyph](glyph.md).
