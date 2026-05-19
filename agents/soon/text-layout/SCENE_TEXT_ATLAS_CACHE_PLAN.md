# Scene Text Atlas And Cache Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
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

Use this file only for remaining execution work. Do not duplicate stable atlas rules here.


## Remaining Atlas Work

Recommended follow-up commits:

1. Add focused tests for the current codepoint-level growth path, including ASCII reuse,
   non-ASCII append, generation increments, and cache hits.
2. Assert old glyph UV stability after growth, accounting for expected global rescale when texture
   dimensions change.
3. Test FreeType bitmap and MSDF append/growth separately, with representative glyphs such as
   `b`, `e`, `g`, `@`, punctuation, and dense lowercase strings.
4. Add explicit missing-glyph entries and diagnostics instead of silently skipping unsupported
   characters.
5. Formalize atlas/page/entry structs and cache keys so the codepoint path can migrate to
   `(font face, glyph id)` requests.
6. Add embedded default atlas support for the built-in/default font and common ASCII labels.
7. Add atlas stats and diagnostics before exposing any public cache API.
8. Add optional persistent disk cache only after the in-memory cache/page model is stable; disk
   writes must remain explicit opt-in behavior.


## Validation

For atlas/cache work:

```text
just build
just test scene
git diff --check
```

For renderer resource changes, add offscreen readback coverage for bitmap and MSDF text. For
persistent-cache work, verify that no disk writes occur unless an explicit cache directory is
configured.
