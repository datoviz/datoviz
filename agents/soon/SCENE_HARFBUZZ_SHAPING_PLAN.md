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

The durable shaping/layout/atlas/cache contract now lives in
[../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md](../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md).
Keep this file focused on the HarfBuzz integration sequence, tests, and unresolved choices.


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


## Durable Contract Summary

Do not duplicate the full shaping contract here. Use
[../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md](../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md)
for stable rules:

- scene text preserves original UTF-8 bytes;
- shaping output is independent from placement, panzoom, camera transforms, and atlas UVs;
- shaped runs carry selected font face, glyph ids, clusters, advances, offsets, and run metadata;
- fallback happens before atlas lookup;
- atlas lookup receives `(font face, glyph id)` pairs;
- placement-only changes should not force reshaping.

The first production slice can still start with one font face and left-to-right text, but internal
structs should leave room for script, direction, language, features, and fallback runs.


## Build And Dependency Model

HarfBuzz should be optional but preferred when scene text is enabled:

- add a CMake option and compile definition such as `DVZ_HAS_HARFBUZZ`;
- expose clear diagnostics when Unicode shaping is requested but HarfBuzz is unavailable;
- keep the simple deterministic renderer available without HarfBuzz;
- share FreeType font face data where practical;
- avoid making HarfBuzz symbols visible in public Datoviz headers.

The HarfBuzz-backed code can live under `src/scene/` until text becomes a standalone module.


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
