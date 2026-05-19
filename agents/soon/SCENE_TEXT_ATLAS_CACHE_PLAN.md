# Scene Text Atlas and Cache Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** define the atlas generation, cache, embedded default atlas, dynamic growth, and
>   optional persistent-cache path for scene text rendering.


## Context

This note splits the atlas/cache work out of
[SCENE_TEXT_GLYPH_PLAN.md](SCENE_TEXT_GLYPH_PLAN.md). The umbrella plan still owns the overall
text, glyph, math, and renderer roadmap. This file owns the more concrete resource model for
bitmap, SDF, and MSDF atlas-backed renderers.

Atlas work is required even after HarfBuzz is integrated. HarfBuzz converts UTF-8 text into shaped
glyph ids, clusters, advances, and offsets. The atlas system maps those glyph ids and renderer
settings to GPU texture regions, metrics, cache entries, uploads, and shader parameters.


## Goals

- Make the default text path work out of the box for common labels and UI-like scientific text.
- Use automatic in-memory caches by default.
- Support dynamic atlas growth when text requests glyphs that are not already resident.
- Ship an embedded default atlas for the built-in/default font and common ASCII/Latin-1 labels.
- Keep the current tiny deterministic bitmap path as a debug/test/fallback backend.
- Support FreeType hinted bitmap atlases for small text.
- Support MSDF atlases for scalable medium and large text.
- Keep SDF as a fallback/debug backend if MSDF is unavailable or disabled.
- Avoid automatic writes to the user's filesystem.
- Preserve a clean DRP2 replay path for generated or embedded atlas resources.


## Non-Goals

- Do not solve text shaping here. That belongs to
  [SCENE_HARFBUZZ_SHAPING_PLAN.md](SCENE_HARFBUZZ_SHAPING_PLAN.md).
- Do not hide missing glyphs by silently skipping them.
- Do not introduce implicit platform-specific disk caches.
- Do not require all text renderers to use atlases forever. A later vector path can share the
  cache-key model without sharing the texture-atlas implementation.


## Current State

The current scene text work has real renderer plumbing and a first dynamic-growth path:

- FreeType bitmap, MSDF, and SDF atlas generation share an internal `DvzTextAtlas` shape.
- `_scene_text_atlas_ensure_strings()` builds a requested codepoint set from default printable
  ASCII plus the requested UTF-8 strings.
- `_text_atlas_ensure_set()` reuses an existing atlas when it already covers the request.
- `_text_atlas_try_append()` can append missing codepoints into an existing atlas by building a
  temporary delta atlas, growing the atlas texture, copying old pixels, packing the new glyph
  rectangles, rescaling old UVs, and appending glyph metadata.
- The current growth policy doubles both atlas width and height.
- Atlas entries remain capped by `DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS` (`256` at the time this note was
  updated).
- The shader now receives atlas pixel range through glyph-specific uniforms instead of hardcoding
  MSDF range.
- The older 6x8 bitmap path is deterministic and useful for tests, but it is not the quality path.

This means "dynamic atlas growth" exists, but it is still a first implementation. The remaining work
is to harden, test, and evolve it into a mature cache/page subsystem that can support fallback fonts,
HarfBuzz glyph ids, embedded default atlases, and broader Unicode ranges.


## Atlas Objects

Use explicit objects internally instead of treating the atlas as one texture plus loose arrays.

Suggested internal model:

```c
typedef struct DvzTextAtlasKey
{
    uint64_t font_id;
    uint32_t face_index;
    uint32_t renderer_backend;
    uint32_t size_bucket;
    uint32_t load_flags;
    uint32_t generator_version;
    uint32_t pixel_range_bits;
} DvzTextAtlasKey;

typedef struct DvzTextAtlasGlyphKey
{
    uint64_t font_id;
    uint32_t glyph_id;
    uint32_t renderer_backend;
    uint32_t size_bucket;
} DvzTextAtlasGlyphKey;

typedef struct DvzTextAtlasEntry
{
    uint32_t page_id;
    uint32_t glyph_id;
    float advance;
    float plane_bounds[4];
    float atlas_bounds[4];
    float uv[4];
    float pixel_range;
} DvzTextAtlasEntry;
```

The exact struct names can change, but the important rule is that atlas entries should be keyed by
font face and glyph id, not by Unicode codepoint. Codepoints are useful before shaping and for
debugging, but HarfBuzz output and font fallback operate on glyph ids inside selected font faces.


## Cache Layers

### Embedded Built-In Atlases

Datoviz should eventually ship at least one embedded default atlas:

- default sans regular face;
- ASCII coverage at minimum;
- likely Latin-1 coverage if binary size stays reasonable;
- metadata generated with the same code path as runtime atlases;
- versioned generator parameters;
- no runtime generation cost for normal examples and common plots.

This should not replace the deterministic 6x8 fallback. The embedded atlas is the normal default
quality path when the bundled font and common glyph range are enough. The 6x8 atlas remains the
minimal backend for tests, diagnostics, and dependency-free builds.

### In-Memory Dynamic Cache

The default runtime cache should be automatic and in memory:

- scoped to the scene, app, or renderer runtime;
- shared across text visuals in the same owner;
- keyed by font face, renderer backend, size bucket, and generator parameters;
- able to add missing glyphs after initial atlas creation;
- able to upload only new texture regions where the backend supports it;
- able to report stats for tests and diagnostics.

The first implementation can avoid eviction. A scene with many fonts, sizes, and scripts can allocate
more atlas pages rather than compacting or invalidating UVs. Eviction can be added later once usage
patterns are clear.

### Optional Persistent Disk Cache

Persistent cache should be an explicit advanced-user feature, not automatic library behavior.
Datoviz is a library and should not write cache files into user directories without a direct request.

A later public or semi-public API could look like:

```c
bool dvz_text_cache_set_directory(DvzScene* scene, const char* path, uint32_t flags);
bool dvz_text_cache_clear_disk(DvzScene* scene);
bool dvz_text_cache_stats(DvzScene* scene, DvzTextCacheStats* out);
```

The exact owner may be `DvzApp`, `DvzScene`, or a future renderer context. The important policy is:

- no implicit writes;
- cache format versioned by Datoviz version and generator version;
- safe failure path when the cache is unreadable or stale;
- deterministic fallback to in-memory generation.


## Dynamic Growth

The current implementation already exposes the codepoint-level equivalent of an "ensure glyphs"
operation:

```text
UTF-8 strings
  -> default ASCII + requested codepoint set
  -> reuse existing atlas if covered
  -> append missing codepoints into a grown texture when possible
  -> rebuild or fallback if append fails
```

The desired mature version should generalize this to glyph ids and pages:

```text
shaped run
  -> collect missing (font face, glyph id) pairs
  -> pack missing glyphs into existing pages when possible
  -> allocate new pages when needed
  -> generate bitmap/SDF/MSDF pixels
  -> upload new regions or pages
  -> return stable atlas entries
```

Important constraints:

- Existing UVs should remain stable across growth.
- A text string should never render with missing glyphs because an upload happened one frame late
  unless the renderer explicitly reports an asynchronous pending state.
- Page dimensions, padding, pixel range, and generator settings must be part of cache identity.
- The shader must use the atlas entry's or atlas page's pixel range instead of a hardcoded value.
- Atlas updates must be visible to DRP2 validation and replay.

Near-term hardening for the existing growth path:

- Add CPU tests that create an ASCII atlas, request `é`/`Ω`, and assert atlas growth.
- Assert that old glyph UVs remain stable after growth, modulo the expected global rescale when the
  texture dimensions change.
- Assert `generation` increments when new glyphs are appended and does not increment for cache hits.
- Test MSDF and FreeType bitmap append separately.
- Test capacity behavior near `DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS`.
- Report append/growth/fallback events in diagnostics so visual tools can show what happened.
- Consider a less wasteful growth policy than doubling both width and height on every append.


## Renderer-Specific Notes

### FreeType Bitmap Atlas

Use for small text, tick labels, UI labels, and labels around the hinted size.

Cache key details:

- font face;
- pixel height or size bucket;
- FreeType load flags;
- hinting mode;
- LCD/subpixel policy if ever supported;
- atlas padding.

The shader should sample coverage alpha. This path should not try to scale too far away from the
rasterized size.

### MSDF Atlas

Use for medium and large text, rotated text, and scale-varying labels.

Cache key details:

- font face;
- glyph id;
- generator version;
- pixel range;
- edge coloring parameters;
- miter limit;
- atlas padding;
- nominal size or scale bucket if retained.

Tests should include glyphs that have shown artifacts in practice, especially `b`, `e`, `g`, `@`,
punctuation, and dense lowercase strings. Artifacts in these glyphs usually indicate a mismatch among
atlas bounds, plane bounds, pixel range, edge coloring, shader distance reconstruction, or texture
filtering.

### SDF Fallback

Keep SDF as a fallback/debug backend, but do not make it the quality target if MSDF is available.
It remains useful because it is simpler and can isolate bugs in MSDF edge coloring.


## Missing Glyph Policy

Missing glyphs must be explicit:

- try the selected font face;
- try configured fallback faces once fallback exists;
- use a visible missing-glyph box as the final fallback;
- report diagnostics in debug/test builds;
- expose missing-glyph counts in cache or text stats.

Silently dropping characters is not acceptable. It makes Unicode failures look like layout bugs.


## Embedded Atlas Packaging

Good default behavior probably needs a bundled font and embedded atlas:

- a small permissive sans regular face;
- a generated ASCII atlas in the binary;
- possibly Latin-1 if size is acceptable;
- build-time regeneration script checked into the repo;
- generated metadata checked in only if that is the existing project practice for generated assets.

This avoids recomputing common text every time an example starts, while still allowing arbitrary
custom fonts and rare glyphs through the dynamic runtime path.


## Tests

Focused tests should cover:

- atlas cache key equality and inequality;
- cache reuse across repeated strings;
- dynamic growth when a new glyph appears;
- generation increments on append and stays stable on cache hits;
- stable UVs after growth, with expected rescaling when atlas dimensions change;
- FreeType bitmap append/growth;
- MSDF append/growth;
- capacity behavior near `DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS`;
- custom font path atlas generation;
- embedded atlas lookup for default ASCII;
- missing glyph fallback and diagnostics;
- no disk writes unless an explicit persistent cache directory is configured;
- FreeType bitmap atlas creation when FreeType is enabled;
- MSDF atlas creation when MSDF is enabled;
- offscreen readback with representative glyphs for bitmap and MSDF renderers.

Regression screenshots or readback tests should include:

- `The quick brown fox jumps over 13 lazy glyphs.`
- `UTF-8 fallback: A?B cafe -> ?`
- glyphs `b e g @ S`;
- small text around 8 to 14 px;
- large zoomed text with MSDF.


## Execution Phases

1. Add focused tests for the current codepoint-level dynamic growth path.
2. Add per-glyph atlas metadata diagnostics for requested sample strings.
3. Improve or document the current texture growth policy.
4. Formalize internal atlas/page/entry structs and cache keys.
5. Route current bitmap/SDF/MSDF metadata through the same atlas-entry contract.
6. Add explicit missing-glyph entries and diagnostics.
7. Add embedded default atlas for the default font and common ASCII labels.
8. Generalize dynamic growth from codepoints to glyph ids and atlas pages.
9. Add atlas stats and focused cache tests.
10. Add optional persistent disk cache API only after the in-memory model is stable.
11. Integrate HarfBuzz-shaped glyph ids as the primary atlas request path.


## Open Questions

- Should the primary text cache owner be `DvzScene`, `DvzApp`, or a renderer/runtime context?
- Which bundled default font should be used, and what are its license and binary-size implications?
- Should the embedded atlas include only ASCII or also Latin-1?
- Do we want a public cache stats API in v0.4, or only internal diagnostics first?
- How much atlas data should DVZR record: generated pixels, source font identity, or both?
