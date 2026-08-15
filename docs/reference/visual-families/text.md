# Text

Retained semantic text objects lowered to glyph visuals.

Status: supported.
Backends: native; WebGPU live (`text`, `glyph-atlas`).
Primitive: generated glyph quads.

## Preview And Links

[![Text](../../assets/gallery/v0.4/visuals/visuals_text.webp)](../../examples/gallery/visuals/visuals_text.md)

- Example: [Text](../../examples/gallery/visuals/visuals_text.md)
- How-to: [Add text, labels, and annotations](../../how-to/add-annotations.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Glyph](glyph.md), [Labels](labels.md), [Marker](marker.md)

## Use When

Use text for user-facing annotations, titles, labels, and short strings where Datoviz should manage
font style, placement, layout, and glyph atlas lowering.

## Avoid When

Use [Glyph](glyph.md) only for low-level atlas work. Use [Labels](labels.md) for categorical
sampled fields, not strings.

## Item And Data Model

Create semantic text with `dvz_text(panel, flags)`, not with a visual constructor. One text object
contains `N` semantic text items and is already attached to its panel. `dvz_text_set_items()`
atomically replaces the collection and copies all item data and UTF-8 strings before returning;
`item_count = 0` clears it.

For one item, `dvz_text_set_string()` and `dvz_text_set_position()` are convenient. For an existing
collection, the per-property setters require exactly the current item count.

## Text Item Contract

| Property | Requirement/default | C type and cardinality | Python representation | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- | --- |
| `string` | Required for visible content; `NULL` clears one-item text | UTF-8 `const char*` per item | `bytes` in `DvzTextItem`; raw string-array setter needs ctypes-compatible pointers | UTF-8 content is copied. | `dvz_text_set_items()`, `dvz_text_set_string()`, or `dvz_text_set_strings()`. |
| `position` | Required placement input; zero-initialized item is `(0,0,0)` | `double[N][3]` | `float64`, `(N, 3)` with adapted `dvz_text_set_positions()` | Interpreted by current placement mode: logical panel pixels in screen mode, authored data coordinates in data mode. | Atomic items, one-item position, or array setter. |
| `offset` | Optional; default `(0,0)` | `float[N][2]` | `float32`, `(N, 2)` | Logical-pixel offset in every placement mode. | Atomic items or `dvz_text_set_offsets()`. |
| `anchor` | Optional; `(0,0)` resolves to the placement text anchor when one is set | `float[N][2]` | `float32`, `(N, 2)` | Normalized per-item text anchor. | Atomic items or `dvz_text_set_anchors()`. |
| `size_px` | Optional per item; values `<= 0` resolve to the object style size | `float[N]` | `float32`, `(N,)` | Logical-pixel text size. | Atomic items or `dvz_text_set_sizes()`. |
| `color` | Optional per item; all-zero RGBA resolves to the object style color | `DvzColor[N]` | `uint8`, `(N, 4)` with adapted setter | RGBA8. Use a nonzero alpha to request an explicit item color. | Atomic items or `dvz_text_set_colors()`. |
| `angle` | Optional; default `0` | `float[N]` | `float32`, `(N,)` | Radians, positive counter-clockwise in rendered y-up coordinates. | Atomic items or `dvz_text_set_angles()`. |

## Style, Placement, And Layout

- `dvz_text_style()` returns white, small-bitmap-atlas text with `size_px = 0`; zero size resolves
  from the scene font defaults. Pass the descriptor to `dvz_text_set_style()` after modification.
- `dvz_text_placement()` defaults to panel-local screen placement anchored at panel top-left.
  `DVZ_TEXT_PLACEMENT_DATA` anchors positions to data coordinates. Placement offset remains logical
  pixels in either mode.
- `dvz_text_layout()` defaults to line height `1`, zero extra gap, no wrapping, and left alignment.
- `dvz_text_set_renderer()` changes the renderer directly. Atlas availability may cause documented
  internal fallback; use semantic text unless low-level atlas control is required.
- Current examples pass `flags = 0`. Destroy explicitly with `dvz_text_destroy()` when removing the
  retained text before panel/scene teardown.

## Fonts And Scientific Glyphs

Datoviz works offline without font configuration: retained text defaults to embedded Source Sans 3, monospace consumers default to Source Code Pro, and mathematical or technical codepoints missing from Source fall back per glyph to embedded Noto Sans Math. Unsupported codepoints render with the visible `?` fallback. Font family names do not trigger platform font discovery.

To use a custom TTF or compatible OpenType face, create a scene-owned font from an explicit path and select it in the text style:

```c
DvzFontDesc font_desc = dvz_font_desc();
font_desc.path = "/absolute/or/application-owned/font.ttf";
font_desc.face_index = 0;
DvzFont* font = dvz_font(scene, &font_desc);

DvzTextStyle style = dvz_text_style();
style.font = font;
style.size_px = 18.0f;
dvz_text_set_style(text, &style);
```

Datoviz copies descriptor strings and loads the selected file into scene-owned storage when the font is first used. Keep the font alive while text objects or atlases refer to it; scene teardown releases remaining fonts after their dependents. A custom primary face still uses the built-in Noto Sans Math scientific fallback when needed.

## Verified Usage Pattern

Create the panel-owned object, configure descriptors returned by the default helpers, populate an
initialized `DvzTextItem[N]`, and call `dvz_text_set_items()`. The complete
[C and Python example](../../examples/gallery/visuals/visuals_text.md) uses this route.

## Picking And Probing

Text is semantic at the retained scene layer but currently lowers to glyph quads for rendering.
Treat interaction as annotation-level unless a feature explicitly exposes glyph-level details.

## Backend Notes

Native and WebGPU paths are active. The generated gallery route exercises semantic text lowered to
glyph atlas rendering.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/text.c` |
| Gallery | [Text](../../examples/gallery/visuals/visuals_text.md) |
| Build | `just example-c visuals/text` |
| Smoke | `./build/examples/c/visuals/text --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[add annotations](../../how-to/add-annotations.md), [Glyph](glyph.md), [Labels](labels.md).
