# Text, Glyph, And LaTeX/Math Rendering

> **Example status:** informative pressure test
> **Target:** staged C/Python scene feature and gallery examples
> **Data:** bundled fonts/assets, deterministic inline strings
> **Validation:** smoke, screenshot/readback, manual visual checklist

See [../SHARED_POLICIES.md](../SHARED_POLICIES.md) for shared example policy. API names below are
pressure tests, not final public API.


## Summary

Build bundled text, glyph, and LaTeX-style math rendering for Datoviz without depending on system
LaTeX, TeX Live, MiKTeX, `latex`, `pdflatex`, `dvipng`, or system-installed fonts. The first slice
should render deterministic labels and common math expressions with a bounded font/atlas path; later
slices add shaping, MSDF/MTSDF quality, and glyph-box math layout.


## User-Visible Result

- Text labels, titles, legends, annotations, and Unicode strings render out of the box.
- Math expressions such as `\frac{a}{b}`, `\sqrt{x^2+y^2}`, and
  `\int_0^\infty e^{-x^2}\,dx` render as labels or overlays.
- Text can be placed in screen, panel-normalized, world/data, or mixed coordinates where the anchor
  follows data and glyph size remains screen-stable.
- Users can change string, color, size, anchor, font, coordinate mode, and math backend policy
  without knowing whether the implementation uses bitmap or glyph-box rendering.


## Feature Pressure Points

- Scene-level font lookup, fallback, shaping, glyph cache, glyph atlas, math parsing, layout, dirty
  tracking, and conversion to ordinary scene resources.
- Backend-portable glyph rendering through buffers, textures, samplers, shader modules, pipelines,
  bind groups, and instanced draw commands.
- Atlas reuse and incremental dirty-rectangle uploads rather than whole-atlas or per-string rebuilds.
- DPI-aware text layout, anchors, multiline layout, rotation, opacity, color, and thousands of
  glyphs in a batch.
- Math rendering with an initial bitmap fallback and a later glyph-box path that reuses the glyph
  atlas.
- Vulkan/WebGPU parity for shader source and resource layout.


## Required Data And Resources

Default distributions should include a small license-reviewed font bundle:

| Role | Candidate families |
| --- | --- |
| UI/sans | Inter, Noto Sans, or similar permissive font |
| Monospace | Noto Sans Mono, JetBrains Mono, or similar |
| Math | Latin Modern Math, STIX Two Math, or Libertinus Math |
| Symbols | Optional mathematical/Unicode fallback font |

Recommended dependencies:

| Dependency | Purpose |
| --- | --- |
| FreeType | Load font faces, outlines, glyph metrics, and raster fallback |
| HarfBuzz | Unicode shaping, kerning, ligatures, clusters, complex scripts |
| MSDF/MTSDF generator | High-quality scalable glyph atlas entries |
| MicroTeX or equivalent | Parse and lay out math-mode LaTeX subset without external LaTeX |

Runtime asset discovery should prefer user-provided paths, then application assets, then bundled
Datoviz fonts. Optional system-font fallback must not be required for basic text/math.


## Minimal Implementation Target

First useful milestone:

```text
FreeType + bundled default font
single glyph atlas, alpha or simple SDF
screen-space TextVisual
bitmap MathVisual fallback from MicroTeX or equivalent
no external LaTeX or system font dependency
```

Required initial text features:

- single-line and multiline layout,
- left/center/right and top/center/baseline/bottom anchors,
- pixel or point size,
- color, opacity, rotation, and DPI scaling,
- atlas cache reuse.

Later features:

- MSDF/MTSDF atlas format,
- HarfBuzz shaping and fallback fonts,
- rich text spans, wrapping, ellipsis, underlines/strikethrough,
- glyph-box math backend,
- vector/path fallback for complex symbols.


## Scene And Runtime Behavior

The scene layer owns text-specific state: layout, math layout, font cache, glyph atlas, dirty
tracking, and visual-local text state. The DRP/runtime layer receives only normal resources and draw
commands; see [../../pipeline/FRAME_PLAN.md](../../pipeline/FRAME_PLAN.md),
[../../pipeline/RESOURCE_MODEL.md](../../pipeline/RESOURCE_MODEL.md), and
[../../drp2/](../../../drp2/).

Dirty rules:

- changing the string relayouts and updates glyph instances;
- missing glyphs add atlas entries and dirty texture rectangles;
- font size/style changes may relayout and allocate atlas entries;
- position, color, opacity, and selection should update instance/style data where possible;
- bitmap math is cached by expression, style, size, DPI, color-baking mode, and backend mode.


## Rendering Model

TextVisual/GlyphVisual should render one quad per glyph, usually instanced. Each instance needs the
semantic equivalent of anchor position, glyph offset, quad size, color, atlas UV rectangle, rotation,
and flags for coordinate/decorations. The final memory layout may differ.

MathVisual should keep one public abstraction with interchangeable backends:

| Mode | Path | Use |
| --- | --- | --- |
| Bitmap fallback | math string -> layout -> CPU alpha/RGBA bitmap -> texture quad/image visual | first working math labels |
| Glyph boxes | math string -> layout boxes -> glyph instances -> glyph atlas | sharper long-term path |

The glyph shader should support screen and data/world anchors, rotation, viewport/DPI handling, and
grayscale SDF first. MSDF later uses median RGB distance and derivative correction.


## Implementation Phases

1. Minimal glyph/text visual: bundled font, glyph metrics, alpha/SDF atlas, glyph shader,
   screen-space text, ASCII/basic Unicode, minimal C/Python surface.
2. Text quality: MSDF/MTSDF, HarfBuzz, kerning, ligatures, fallback fonts, multiline and DPI polish.
3. MicroTeX bitmap math: vendor/build engine, bundle math assets, cache equation bitmaps, draw via
   image/textured quad with transparent background.
4. Math glyph boxes: extract structured layout, map symbols to bundled math fonts, support
   baselines, superscripts/subscripts, fractions, common operators, and automatic bitmap fallback.
5. Advanced math/vector paths: OpenType MATH data, stretchy delimiters, radicals, accents, large
   operators, optional path extraction.


## Validation

Unit coverage should include bundled font loading, missing-font fallback, UTF-8 decoding, glyph
metrics, atlas packing, dirty rectangle updates, anchors, multiline baselines, shaping smoke tests,
and MicroTeX parse/render smoke tests.

Rendering checks should cover ASCII, Unicode symbols, monospace text, rotation, multiline text,
screen-space and world-space labels, math equations, high-DPI scaling, transparent math texture, and
thousands-of-labels stress.

Gallery candidates:

- text labels attached to scatter points,
- plot title/axis math labels/legend,
- formula overlay annotations,
- 3D world-anchored labels with pixel-size glyphs,
- bundled font browser.


## Risks And Open Questions

- Whether the selected MicroTeX implementation exposes structured layout for glyph-box rendering.
- How much OpenType MATH support is needed for high-quality delimiters, radicals, accents, and large
  operators.
- License and attribution review for all bundled libraries and fonts.
- Binary/wheel size from bundled fonts and math engine.
- Exact shader/resource layout for Vulkan and WebGPU parity.
