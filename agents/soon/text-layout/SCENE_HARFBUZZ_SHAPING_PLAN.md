# Scene HarfBuzz Shaping Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining HarfBuzz integration work for UTF-8 shaping, glyph ids, clusters,
>   fallback, and layout metrics.


## Current State

The durable shaping/layout/atlas/cache contract lives in
[`../../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md`](../../../spec/scene/implementation/TEXT_SHAPING_ATLAS.md).
That file owns the stable shaping boundaries: original UTF-8 preservation, shaped-run metadata,
font fallback before atlas lookup, `(font face, glyph id)` atlas requests, and placement-only
invalidation rules.

HarfBuzz should be optional but preferred when production scene text is enabled. The deterministic
simple text path should remain available for tests, diagnostics, and dependency-light builds.

Use this file only for HarfBuzz execution sequencing. Do not duplicate stable shaping rules here.


## Remaining HarfBuzz Work

Recommended follow-up commits:

1. Add optional HarfBuzz build wiring and a private wrapper under `src/scene/`, guarded by a compile
   definition such as `DVZ_HAS_HARFBUZZ`.
2. Define shaped-run structs, shape keys, selected font-face identity, glyph ids, clusters,
   advances, offsets, and run metadata.
3. Shape single-font left-to-right UTF-8 strings first, preserving original UTF-8 bytes in retained
   objects and recordings.
4. Replace codepoint advances with HarfBuzz advances and offsets in layout without changing atlas
   ownership.
5. Connect shaped glyph ids to atlas ensure/growth once atlas requests support `(font face,
   glyph id)` pairs.
6. Add explicit missing-glyph rendering and diagnostics.
7. Add user-provided fallback chains before attempting platform font fallback.
8. Add direction, script, language, and feature controls after the single-font path is stable.
9. Add Unicode itemization, BiDi helpers, broader complex-script tests, or ICU only when scoped by a
   concrete text-layout requirement.


## Validation

For HarfBuzz integration:

```text
just build
just test scene
git diff --check
```

Focused tests should cover ASCII parity, combining marks when a deterministic font supports them,
ligature feature toggles, stable cluster indices, missing glyph diagnostics, malformed UTF-8, and no
reshaping when only placement changes.
