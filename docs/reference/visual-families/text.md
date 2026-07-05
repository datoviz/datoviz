# Text

Retained semantic text objects lowered to glyph visuals.

Status: supported.
Backends: native; WebGPU live (`text`, `glyph-atlas`).
Primitive: generated glyph quads.

## Preview And Links

[![Text](../../assets/gallery/v0.4/visuals/visual_text.webp)](../../examples/gallery/visuals/visual_text.md)

- Example: [Text](../../examples/gallery/visuals/visual_text.md)
- How-to: [Add text, labels, and annotations](../../how-to/add-annotations.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Glyph](glyph.md), [Labels](labels.md), [Marker](marker.md)

## Use When

Use text for user-facing annotations, titles, labels, and short strings where Datoviz should manage
font style, placement, layout, and glyph atlas lowering.

## Avoid When

Use [Glyph](glyph.md) only for low-level atlas work. Use [Labels](labels.md) for categorical
sampled fields, not strings.

## Data Model

Create semantic text with `dvz_text(panel, flags)`, not with a public visual constructor. Set UTF-8
content, style, placement, and renderer; the scene prepares the underlying glyph visual at emit
time.

## Attributes

| Kind | Attributes |
| --- | --- |
| Required | string content through `dvz_text_set_string()` |
| Optional | `DvzTextStyle` through `dvz_text_set_style()`; `DvzTextPlacement` through `dvz_text_set_placement()`; renderer through `dvz_text_set_renderer()` |
| Internal lowering | generated glyph visual uses glyph-family attributes internally |

## Picking And Probing

Text is semantic at the retained scene layer but currently lowers to glyph quads for rendering.
Treat interaction as annotation-level unless a feature explicitly exposes glyph-level details.
Text placement `angle` rotates counter-clockwise in rendered y-up coordinates.

## Backend Notes

Native and WebGPU paths are active. The generated gallery route exercises semantic text lowered to
glyph atlas rendering.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/text.c` |
| Gallery | [Text](../../examples/gallery/visuals/visual_text.md) |
| Build | `just example-c visuals/text` |
| Smoke | `./build/examples/c/visuals/text --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[add annotations](../../how-to/add-annotations.md), [Glyph](glyph.md), [Labels](labels.md).
