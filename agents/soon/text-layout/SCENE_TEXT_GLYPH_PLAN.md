# Scene Text And Glyph Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-21`
> - **Purpose:** track remaining text/glyph integration, API cleanup, shaping, and equation work
>   after the first rendered text slice landed and the durable semantics and shaping/atlas
>   contracts were split into `spec/scene`.


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
and `src/scene/text_annotation.c`. The first visible `dvz_text()` path is active: text lowers to
scene-owned glyph visuals, uses bitmap/SDF/MSDF-capable atlas resources, emits through the normal
scene -> FramePlan -> DRP2 -> vklite/app path, and has focused scene/app readback coverage.

The remaining work is no longer first proof of visibility. It is API/spec alignment, integration
with explanatory objects, data/world placement, depth policy, DPI/clipping hardening, production
shaping, richer font fallback, diagnostics, and equation lowering.

Use this file only for execution sequencing. Do not duplicate stable text semantics here.


## Landed / Covered

These items were previously mixed into the active follow-up list but are now covered by current code,
tests, or specs:

1. retained text, font, and annotation bookkeeping exists in the active scene module;
2. `dvz_text()` is a visible visual-backed API and lowers internally to scene-owned glyph visuals;
3. `dvz_glyph()` emits atlas-backed glyph quads through the scene -> DRP2 -> runtime path;
4. annotation labels use the current glyph/text realization path;
5. built-in bitmap, FreeType bitmap, SDF, and MSDF-capable atlas rendering paths exist behind
   feature flags and fallbacks;
6. retained text realization handles strings, per-string size/color/angle/anchor attributes, simple
   multiline text, visibility, destroy, and resize invalidation;
7. focused tests cover bitmap and SDF/MSDF-backed realization, automatic renderer selection,
   UTF-8 atlas growth, missing-glyph fallback, many-label batching, glyph emission, runtime
   readback, and app/offscreen visible text;
8. the transitional `DvzVisual* dvz_text()` / `DvzVisual* dvz_glyph()` surface is documented in
   `spec/scene/visuals/GLYPH.md`.


## Remaining Text And Glyph Work

Recommended follow-up commits:

1. Reconcile the planned semantic `DvzText*` API with the current public
   `DvzVisual* dvz_text()` and `DvzVisual* dvz_glyph()` surface, or explicitly keep the
   transitional API for v0.4 with a narrow public contract.
2. Wire axes, colorbars, legends, and pinned readouts to the current text path without
   rewriting text internals.
3. Finish data/world placement instead of hiding non-screen retained text modes.
4. Honor depth policy for data/world text instead of always forcing generated glyph visuals to
   depth-test disabled.
5. Harden DPI scaling and panel scissor edge cases; resize invalidation is covered, but the text
   path still needs focused high-DPI and clipping validation.
6. Keep the deterministic simple atlas renderer for dependency-light tests and diagnostics.
7. Keep FreeType font loading behind a feature flag and add focused fallback diagnostics before
   making bitmap atlas rendering the default quality path for small labels.
8. Integrate HarfBuzz-shaped glyph ids with atlas growth after the atlas/cache layer can ensure
   glyph resources by `(font face, glyph id)`.
9. Improve explicit missing-glyph, renderer fallback, and atlas-capacity diagnostics.
10. Keep Slug-style vector GPU text and MicroTeX/equation support as later optional lanes until
   ordinary text, labels, and annotation integration are stable.


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
just test text
just test scene
git diff --check
```

For renderer or runtime-resource changes, add focused text/glyph tests plus an offscreen or bounded
GLFW smoke. For new font/shaping dependencies, keep feature-disabled builds and diagnostics covered.
