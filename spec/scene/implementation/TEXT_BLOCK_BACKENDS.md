# Text Block Raster Backends

This note defines the implementation-facing direction for formatted text blocks that are laid out
and rasterized on the CPU, then displayed in the scene as ordinary image-like quads.

It complements [TEXT_SHAPING_ATLAS.md](TEXT_SHAPING_ATLAS.md). The shaping/atlas note remains the
contract for GPU glyph visuals and many small labels. This note owns paragraph-like rich text,
bitmap math, and future CPU-rendered text engines that naturally produce one texture per block.


## Normative Status

This document is an implementation note for a future v0.4 text-block backend. It is not the active
first rendered text slice, but it is the preferred architecture for rich paragraphs and bitmap math
instead of adding ad hoc markup handling to legends, axes, or glyph visuals.

The semantic source remains [../semantics/TEXT.md](../semantics/TEXT.md). The first rendered glyph
slice remains [../slices/TEXT_RENDERING_SLICE.md](../slices/TEXT_RENDERING_SLICE.md).


## Core Direction

Formatted text blocks lower through this pipeline:

```text
UTF-8 source + optional markup + style + layout constraints
  -> backend-specific parse/shaping/layout
  -> CPU raster result
  -> RGBA8 or alpha texture field
  -> image-like scene visual with one quad
```

The output contract is intentionally the same shape as other sampled image visuals: a texture, an
extent, an anchor, opacity/blending state, and normal FramePlan/DRP2 resource updates. The runtime
must not need text-specific Vulkan, WebGPU, or DRP2 commands for the first text-block slice.


## Relationship To Glyph Text

Use the existing glyph visual path for:

1. many small labels,
2. axes, tick labels, colorbars, and ordinary legends,
3. screen-stable labels that need crisp SDF/MSDF scaling,
4. object-level text picking and future per-string identity,
5. cases where changing only position or color should avoid rerasterizing text.

Use text-block raster backends for:

1. paragraph annotations,
2. plot titles, subtitles, captions, and explanatory overlays,
3. rich tooltips and pinned readouts,
4. inline bold, italic, underline, and colored spans,
5. bitmap LaTeX/math fallback,
6. formatted content where one logical block is naturally cached as a texture.

Legends should not depend on rich text blocks just to bold a highlighted category. Whole-label bold
belongs in the text style path first; a legend should only mark which entry is highlighted.


## Backend Interface

Each backend should expose the same internal role:

1. validate source text, style, constraints, and backend-specific options,
2. report required capabilities and visible fallbacks,
3. measure the block in logical pixels,
4. rasterize to RGBA or alpha pixels at the requested physical scale,
5. return baseline, ink bounds, advance bounds, and texture extent metadata,
6. provide a cache key for source, style, layout, DPI, font, and backend version.

The first implementation can keep this interface private under `src/scene/`. Public API naming
should wait until one backend and one consumer are validated.


## Backend Families

Planned backend families:

| Backend | Purpose | First expected output |
|---|---|---|
| FreeType rich bitmap | Rich UTF-8 blocks, simple markup, font-face styling | RGBA8 texture |
| FreeType + HarfBuzz | Complex Unicode shaping, ligatures, fallback runs | RGBA8 texture |
| Math/LaTeX bitmap | Equations and scientific labels without external TeX | Alpha or RGBA texture |
| Markdown-like frontend | Optional parser that emits rich text runs | Rich text block input |

The backend registry should advertise capabilities rather than hard-coding behavior in callers:

1. Unicode input,
2. shaping,
3. bidirectional text,
4. color glyphs,
5. rich markup,
6. math layout,
7. fallback fonts.


## Rich Text Scope

The first rich text block should support a deliberately small markup subset:

1. `<b>` and `</b>`,
2. `<i>` and `</i>`,
3. optional `<u>` and `</u>` if underline is implemented as a decoration pass,
4. optional color spans only after the base run model is stable,
5. escaping for literal `<`, `>`, and `&`.

Parsing should produce style runs over original UTF-8 byte ranges. Invalid markup must either render
as literal text with a diagnostic or fail validation according to an explicit mode. Do not silently
drop text.


## Unicode, HarfBuzz, And Emoji

The source format is UTF-8. The backend must preserve byte ranges so diagnostics, spans, and future
selection can refer back to the original string.

FreeType alone is enough for a first deterministic rich bitmap backend, but it is not enough for
production-quality complex scripts. HarfBuzz is the preferred follow-up for shaping, clusters,
ligatures, combining marks, Arabic and Indic scripts, and OpenType features. Bidirectional text
should be added only with a scoped layout requirement and tests.

Color emoji are possible only when the selected backend and font format support color glyphs. The
architecture should allow RGBA output from day one so color spans, emoji, and colored math fallback
are not blocked. The first slice may render monochrome fallback glyphs with diagnostics when color
glyph support is unavailable.


## Font And Style Policy

Whole-block and run-level style should resolve to real font faces where possible:

1. regular,
2. bold,
3. italic,
4. bold-italic.

Synthetic emboldening or slanting is an allowed fallback only when the backend reports that no
matching face exists. The fallback must be visible in diagnostics because it changes typographic
semantics.

Font fallback chains should be explicit before platform fallback is attempted. Bundled or
application-provided fonts are preferred for deterministic tests and reproducible gallery output.


## Layout Contract

Text-block layout should support:

1. maximum width in logical pixels,
2. optional maximum height and overflow policy,
3. wrapping,
4. line height,
5. horizontal alignment,
6. paragraph padding,
7. block anchor and baseline metadata,
8. measured ink and layout bounds.

The first slice can restrict itself to horizontal left-to-right text and simple wrapping. The
internal data structures should not assume one Unicode codepoint maps to one glyph.


## Scene Lowering

The scene lowering should produce an image-like retained contribution:

1. sampled field or equivalent texture payload,
2. transparent RGBA or alpha format,
3. image quad extent in logical pixels,
4. anchor and placement metadata,
5. alpha blending enabled,
6. ordinary texture create/write commands in FramePlan/DRP2,
7. normal panel viewport/scissor and z-layer behavior.

Text-block resources must be invalidated by changes to source, markup mode, style, font, layout
constraints, DPI/screen scale, backend version, and color-glyph capability. Placement-only changes
should update the quad transform or position without rerasterizing when possible.


## LaTeX And Math Relationship

Do not wait for LaTeX before implementing rich text blocks. Rich text and math share the same output
contract, and the rich text-block path makes the later math backend easier:

```text
math source -> math backend layout/raster -> texture -> image-like quad
```

The first math backend may be bitmap-only. A later glyph-box math backend can reuse
`TEXT_SHAPING_ATLAS.md` by emitting positioned glyph runs, rules, boxes, and transforms instead of a
single raster texture.


## First Implementation Slice

Recommended first slice:

1. private text-block object or annotation mode,
2. FreeType-backed UTF-8 rasterization,
3. fixed max-width paragraph layout,
4. `<b>` and `<i>` style runs,
5. real font-face selection when available,
6. RGBA8 texture output,
7. one image-like quad per block,
8. DPI-aware rerasterization,
9. focused offscreen readback smoke test.

Keep public API exposure narrow until the cache keys, dirty rules, and placement behavior are stable.


## Validation

Focused coverage should include:

1. plain UTF-8 block rasterization,
2. bold, italic, and bold-italic face resolution,
3. malformed markup diagnostics,
4. wrapping and measured extent,
5. DPI scale changes rerasterize the texture,
6. placement-only changes avoid rerasterization when practical,
7. transparent RGBA upload and nonblank offscreen rendering,
8. fallback when color glyphs or requested font faces are unavailable,
9. parity of image-like FramePlan/DRP2 resource emission across repeated frames.
