# Scene Text Atlas And Cache Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-21`
> - **Purpose:** track remaining atlas generation, cache, growth, embedded-atlas, and persistent
>   cache work for scene text rendering.


## Current State

The durable shaping/layout/atlas/cache contract lives in
[`../../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md`](../../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md).
That file owns atlas key fields, entry metadata, dynamic-growth rules, DRP2 emission expectations,
and renderer resource boundaries.

The current implementation already has a first atlas-growth path:

1. FreeType bitmap, MSDF, and SDF atlas generation share an internal atlas shape.
2. Requested UTF-8 strings are converted into a codepoint set.
3. Existing atlases are reused when they cover the request.
4. Missing codepoints can be appended through a temporary delta atlas and texture growth.
5. Atlas entries are capped by `DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS`.
6. The shader receives atlas pixel range through glyph-specific uniforms.
7. The deterministic 6x8 bitmap path remains useful for tests and fallback diagnostics.
8. Focused tests now cover automatic renderer selection, UTF-8 atlas expansion, missing-glyph
   fallback, font-atlas runtime readback, and app/offscreen visible text.

Use this file only for remaining execution work. Do not duplicate stable atlas rules here.


## Landed / Covered

These items were previously mixed into the follow-up list but are now covered by current code and
focused tests:

1. font-backed bitmap, MSDF, and SDF atlas builders behind feature flags;
2. per-font backend atlas slots for bitmap, SDF, and MSDF;
3. UTF-8 string decoding into a codepoint request set;
4. atlas reuse when an existing atlas covers the requested strings;
5. incremental append/growth through a temporary delta atlas;
6. stable sampled-field reuse across compatible atlas growth;
7. unsupported or unavailable codepoints falling back to `?`;
8. `missing_glyph_count` bookkeeping for unavailable glyphs;
9. automatic renderer selection tests;
10. UTF-8 expansion tests;
11. missing-glyph fallback tests;
12. font-atlas runtime readback and app/offscreen visible-text tests.


## Remaining Atlas Work

Recommended follow-up commits:

1. Add a regression test that asserts old glyph UV stability after growth, accounting for expected
   global rescale when texture dimensions change.
2. Test FreeType bitmap and MSDF append/growth separately, with representative glyphs such as
   `b`, `e`, `g`, `@`, punctuation, and dense lowercase strings.
3. Improve explicit diagnostics for missing glyphs, renderer fallback, and atlas-capacity
   truncation; the fallback behavior exists, but reporting is still too sparse.
4. Formalize atlas/page/entry structs and cache keys so the codepoint path can migrate from
   codepoint requests to `(font face, glyph id)` requests.
5. Add embedded default atlas support for the built-in/default font and common ASCII labels, if this
   still provides measurable startup or dependency-light value beyond embedded font bytes.
6. Add atlas stats and diagnostics before exposing any public cache API.
7. Add optional persistent disk cache only after the in-memory cache/page model is stable; disk
   writes must remain explicit opt-in behavior.


## Validation

For atlas/cache work:

```text
just build
just test text
just test scene
git diff --check
```

For renderer resource changes, add offscreen readback coverage for bitmap and MSDF text. For
persistent-cache work, verify that no disk writes occur unless an explicit cache directory is
configured.
