# Pixel

Screen-space square sprites for per-item pixel-like marks.

Status: supported.
Backends: native; WebGPU live (`pixel`, `panzoom`).
Primitive: instanced screen-space quads.

## Use When

Use pixel visuals for square marks with explicit pixel sizes, especially sparse raster samples or
selection overlays where each item remains an independently addressable mark.

## Avoid When

Use [Point](point.md) for circular marks, [Marker](marker.md) for symbolic glyphs, or
[Image](image.md) for dense regular sampled fields.

## Data Model

Create with `dvz_pixel(scene, flags)`. Upload one item per pixel mark. The canonical example uses
scalar values for `color` and binds a color scale.

## Attributes

Required: `position` (`vec3` center), `color` (RGBA8 or configured scalar), `pixel_size` (`float`,
pixels).

Optional: `item_state` for retained hover/selection styling; alpha mode, depth test, transform, and
visual-wide scale bindings.

## Picking And Probing

Pixel visuals preserve item identity through the screen-space quad lowering. Use them when picking
or selection must report the original item index.

## Backend Notes

Native and WebGPU paths are active. WebGPU lowers each item to an instanced quad. The example
disables depth testing for a 2D panzoom panel.

## Canonical Example

- Source: `examples/c/visuals/pixel.c`
- Gallery: [Pixel](../../examples/gallery/visuals/visual_pixel.md)
- Build: `just example-c visuals/pixel`
- Smoke: `./build/examples/c/visuals/pixel --png`
- Validation: `smoke+screenshot`
- Agent copy-safe: yes

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[select items](../../how-to/select-items.md), [Point](point.md), [Image](image.md).
