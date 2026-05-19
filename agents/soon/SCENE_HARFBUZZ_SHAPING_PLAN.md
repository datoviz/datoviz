# Scene HarfBuzz Shaping Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** define staged HarfBuzz integration for UTF-8 shaping, glyph ids, clusters, font
>   fallback, layout metrics, and future complex-script support in scene text.


## Context

This note splits the HarfBuzz shaping work out of
[SCENE_TEXT_GLYPH_PLAN.md](SCENE_TEXT_GLYPH_PLAN.md). The atlas/cache details live in
[SCENE_TEXT_ATLAS_CACHE_PLAN.md](SCENE_TEXT_ATLAS_CACHE_PLAN.md).

HarfBuzz is the right final layer for shaping. It should replace direct codepoint-to-glyph mapping
for production text, but it does not replace atlas generation. The shaped output feeds the atlas
system by selected font face and glyph id.


## Goals

- Accept UTF-8 at the public API boundary.
- Preserve original UTF-8 bytes in retained objects and recordings.
- Shape text into glyph ids, clusters, advances, offsets, and run metadata.
- Support OpenType features through a structured internal model.
- Track enough cluster information for future picking, editing, diagnostics, and layout.
- Keep shaping independent of placement, panzoom, and renderer backend.
- Keep HarfBuzz optional at build time with a deterministic fallback path when disabled.
- Fit the active scene -> DRP2 -> vklite/app path, with no parallel renderer.


## Non-Goals For The First Slice

- No rich text editor.
- No full paragraph layout engine.
- No full platform font fallback stack in the first patch.
- No color-font or emoji rendering initially.
- No dependency on ICU unless later Unicode requirements justify it.
- No attempt to hand-roll complex shaping when HarfBuzz is unavailable.


## Why HarfBuzz Is Still Needed

Intermediate Unicode workarounds can improve specific demos, but they do not solve the general
problem. Direct codepoint mapping fails for:

- ligatures;
- combining marks;
- Arabic and Indic shaping;
- OpenType substitutions and positioning;
- right-to-left scripts;
- script-specific glyph selection;
- font-specific glyph ids;
- reliable cluster mapping.

The useful intermediate work is not a competing shaper. The useful intermediate work is preparing
the atlas and layout contracts so HarfBuzz can feed them cleanly.


## Pipeline

The durable shaping pipeline should be:

```text
retained UTF-8 string + style
  -> optional UTF-8 validation and normalization
  -> font fallback and run segmentation
  -> HarfBuzz buffer setup
  -> HarfBuzz shaping
  -> shaped glyph runs
  -> line/box layout
  -> atlas ensure by font face and glyph id
  -> positioned glyph instances
  -> DRP2 resources and draws
```

The first production slice can start with a single font face and left-to-right text. The structs
should still leave room for script, direction, language, features, and fallback runs.


## Build And Dependency Model

HarfBuzz should be optional but preferred when scene text is enabled:

- add a CMake option and compile definition such as `DVZ_HAS_HARFBUZZ`;
- expose clear diagnostics when Unicode shaping is requested but HarfBuzz is unavailable;
- keep the simple deterministic renderer available without HarfBuzz;
- share FreeType font face data where practical;
- avoid making HarfBuzz symbols visible in public Datoviz headers.

The HarfBuzz-backed code can live under `src/scene/` until text becomes a standalone module.


## Internal Data Model

Suggested shape key:

```c
typedef struct DvzTextShapeKey
{
    uint64_t string_hash;
    uint64_t font_chain_hash;
    uint32_t direction;
    uint32_t script;
    uint32_t language_id;
    uint32_t feature_set_id;
    uint32_t normalization_flags;
} DvzTextShapeKey;
```

Suggested shaped run:

```c
typedef struct DvzTextShapedGlyph
{
    uint32_t glyph_id;
    uint32_t cluster;
    int32_t advance_x;
    int32_t advance_y;
    int32_t offset_x;
    int32_t offset_y;
} DvzTextShapedGlyph;

typedef struct DvzTextShapedRun
{
    uint64_t font_face_id;
    uint32_t direction;
    uint32_t script;
    uint32_t language_id;
    uint32_t glyph_count;
    DvzTextShapedGlyph* glyphs;
} DvzTextShapedRun;
```

The exact representation can change. The key rule is that shaped runs are independent from
placement, transforms, and atlas page coordinates.


## Font Fallback

Implement fallback in stages.

Stage 1:

- shape with one selected font face;
- expose missing glyphs explicitly;
- render missing-glyph boxes rather than skipping.

Stage 2:

- user-provided fallback chain;
- split runs when the primary face lacks coverage;
- cache coverage maps per font face where useful.

Stage 3:

- platform or bundled fallback fonts;
- script-aware fallback;
- deterministic test fallback fonts.

Fallback should happen before atlas lookup. The atlas should receive selected `(font face, glyph id)`
pairs, not unsupported codepoints.


## Unicode Normalization

Keep the original UTF-8 bytes in retained objects. Normalization, if used, should be part of the
shape key and should not destroy the original user string.

Practical policy:

- validate UTF-8 early;
- consider optional NFC normalization for common Latin combining-mark cases;
- do not rely on normalization as a substitute for HarfBuzz;
- keep malformed UTF-8 diagnostics explicit.

Normalization can improve equivalence between `cafe` plus combining accent and precomposed `cafe`
with an accented glyph, but shaping and fallback still need to handle arbitrary scripts and fonts.


## Direction, Script, And BiDi

HarfBuzz shapes runs. It does not by itself provide a complete paragraph layout and bidirectional
algorithm for all UI needs.

Staged policy:

- first: default left-to-right with optional internal fields for direction, script, and language;
- next: let callers provide direction/script/language hints where needed;
- later: add Unicode itemization and BiDi support if text blocks and RTL paragraphs become
  first-class requirements.

This distinction matters for tests. A single Arabic word can validate shaping, but complete mixed
LTR/RTL paragraphs need a broader text layout layer.


## Layout Integration

Layout should consume shaped runs and produce positioned glyph instances:

- scale HarfBuzz font-unit advances to pixels/points using the selected font size;
- apply glyph offsets before anchor/alignment transforms;
- compute advance bounds, ink bounds, baseline, ascender, descender, and line height;
- keep layout independent from atlas UVs;
- invalidate layout on size, DPI, alignment, wrapping, or metrics-mode changes;
- do not invalidate shaping when only placement or panzoom changes.

This is also the contract that later math layout should target: math produces normal positioned
glyph runs plus rule/box primitives.


## Atlas Integration

The atlas system should be asked to ensure glyphs after shaping:

```text
shaped run glyph ids
  -> atlas cache lookup by font face and glyph id
  -> dynamic atlas growth for missing entries
  -> glyph instances receive UVs and plane bounds
```

Do not key atlas entries by Unicode codepoint once HarfBuzz is in the path. Different fonts can map
the same codepoint to different glyph ids, and the same glyph id is only meaningful inside one face.


## Caching

Use distinct caches:

- shape cache: UTF-8 bytes, font fallback chain, direction, script, language, features, normalization;
- layout cache: shaped run id, size, DPI, line spacing, wrap width, alignment, metrics mode;
- atlas cache: selected font face, glyph id, renderer backend, size bucket, generator parameters.

This separation is what makes panzoom and placement changes cheap. They should update transforms or
instance positions, not reshape text or rebuild atlases.


## Tests

Early tests:

- ASCII parity with the current simple path;
- `cafe` plus an accented precomposed character when a deterministic test font supports it;
- equivalent combining-mark input if normalization is enabled;
- `A?B`-style missing glyph behavior with visible fallback;
- a ligature-sensitive pair such as `fi` with features on/off if the test font supports it;
- stable cluster indices for simple strings;
- no reshaping when only placement changes.

Later tests:

- Arabic shaping for a short word;
- Hebrew or Arabic direction handling once BiDi is scoped;
- CJK fallback with a deterministic fallback font;
- mixed fallback runs;
- malformed UTF-8 diagnostics;
- DRP2 replay of shaped text.


## Execution Phases

1. Add optional HarfBuzz build wiring and internal wrapper functions.
2. Define shaped-run structs, shape keys, and cache ownership.
3. Shape single-font, left-to-right UTF-8 strings.
4. Replace codepoint advances with HarfBuzz advances and offsets in layout.
5. Ensure atlas entries by selected font face and glyph id.
6. Add explicit missing-glyph rendering and diagnostics.
7. Add user-provided fallback chains.
8. Add direction, script, language, and feature controls.
9. Add Unicode itemization, BiDi helpers, and broader complex-script tests if needed.


## Agentic Work Estimate

The HarfBuzz integration is moderate to large, but it decomposes well:

- build/dependency wiring is a small isolated slice;
- single-font shaping is a focused internal implementation slice;
- layout replacement is a separate scene text slice;
- atlas interop depends on the dynamic atlas work and is the main coupling point;
- fallback, BiDi, and platform font discovery should be later slices with explicit tests.

The best sequence is to land atlas/cache cleanup first or in parallel with a narrow HarfBuzz wrapper,
then connect shaped glyph ids to dynamic atlas growth.


## Open Questions

- Should public text style expose direction, script, language, and features in v0.4, or should those
  remain internal until Unicode hardening?
- Should normalization be enabled by default or only as an opt-in style flag?
- Which deterministic test fonts should be bundled for Latin, fallback, and complex-script tests?
- How much platform font fallback should Datoviz own versus requiring explicit user font chains?
- Should shaped-run cache ownership live at scene scope, app scope, or a future shared text context?
