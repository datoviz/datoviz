# Scene Text And Glyph Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining retained text, glyph, and equation work after the durable text
>   semantics and shaping/atlas contracts were split into `spec/scene`.


## Current State

Durable text contracts live in:

1. [`../../../spec/scene/semantics/TEXT.md`](../../../spec/scene/semantics/TEXT.md)
2. [`../../../spec/scene/slices/TEXT_RENDERING_SLICE.md`](../../../spec/scene/slices/TEXT_RENDERING_SLICE.md)
3. [`../../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md`](../../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md)
4. [`../../../spec/scene/proposals/active/TEXT_DESIGN.md`](../../../spec/scene/proposals/active/TEXT_DESIGN.md)
5. [`../../../spec/scene/visuals/GLYPH.md`](../../../spec/scene/visuals/GLYPH.md)

Focused execution notes:

1. [`SCENE_TEXT_ATLAS_CACHE_PLAN.md`](SCENE_TEXT_ATLAS_CACHE_PLAN.md) tracks atlas/cache hardening.
2. [`SCENE_HARFBUZZ_SHAPING_PLAN.md`](SCENE_HARFBUZZ_SHAPING_PLAN.md) tracks shaping integration.

The active scene already has retained text and annotation bookkeeping in `include/datoviz/scene/`
and `src/scene/text_annotation.c`. Visible text rendering, glyph visual emission, production font
loading, production shaping, and equation lowering remain follow-up work.

Use this file only for execution sequencing. Do not duplicate stable text semantics here.


## Remaining Text And Glyph Work

Recommended follow-up commits:

1. Land the first visible `DvzText` rendering slice from `TEXT_RENDERING_SLICE.md` through the
   normal scene -> FramePlan -> DRP2 -> vklite/app path.
2. Keep a deterministic simple atlas renderer for dependency-light tests and diagnostics.
3. Add a retained glyph visual only after the first text renderer proves atlas texture, sampler,
   glyph quad, panel scissor, and repeated-frame reuse behavior.
4. Add FreeType font loading behind a feature flag before making bitmap atlas rendering the quality
   path for small labels.
5. Integrate HarfBuzz-shaped glyph ids with atlas growth after the atlas/cache layer can ensure
   glyph resources by `(font face, glyph id)`.
6. Add SDF/MSDF rendering as a production-quality medium/large text path after bitmap text is
   correct and testable.
7. Integrate axes, colorbars, legends, annotations, and pinned readouts once measurement and layout
   can consume text bounds.
8. Keep Slug-style vector GPU text and MicroTeX/equation support as later optional lanes until
   ordinary text and glyph rendering are stable.


## v0.3 Reference

Use v0.3 as behavior reference, not architecture:

1. `v0.3/src/scene/visuals/glyph.c`
2. `v0.3/include/datoviz/scene/atlas.h`
3. `v0.3/src/scene/font.c`
4. `v0.3/include/datoviz/scene/glsl/text_functions.glsl`

Useful ideas to retain:

1. atlas-backed glyph rendering;
2. world-space glyph placement;
3. MSDF shader support;
4. grouped strings and anchors.

Avoid exposing low-level per-glyph atlas plumbing as the primary public text API.


## Validation

For text/glyph implementation work:

```text
just build
just test scene
git diff --check
```

For renderer or runtime-resource changes, add focused text/glyph tests plus an offscreen or bounded
GLFW smoke. For new font/shaping dependencies, keep feature-disabled builds and diagnostics covered.
