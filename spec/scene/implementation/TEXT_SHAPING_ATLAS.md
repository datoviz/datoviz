# Text Shaping, Layout, Atlas, and Cache

This note defines the implementation-facing contract between retained scene text semantics,
shaping/layout, glyph resource generation, and DRP2 emission.

It does not replace the semantic text model in [../semantics/TEXT.md](../semantics/TEXT.md), the
first rendered text slice in [../slices/TEXT_RENDERING_SLICE.md](../slices/TEXT_RENDERING_SLICE.md),
or the rationale in [../proposals/active/TEXT_DESIGN.md](../proposals/active/TEXT_DESIGN.md).

CPU-rasterized rich paragraphs and bitmap math use the separate
[TEXT_BLOCK_BACKENDS.md](TEXT_BLOCK_BACKENDS.md) contract. They should not be forced through the
glyph-atlas path when one texture-backed block is the simpler semantic output.


## Normative Status

This document is an implementation note for the current v0.4 text path. It is authoritative for
scene-internal text processing boundaries unless it conflicts with the semantic text spec, public
headers, or DRP2 protocol specs.

Current execution order belongs in `agents/now/STATUS.md`; durable text contracts stay here or in
the semantic text spec.


## Core Contract

Text processing is split into stable stages:

```text
UTF-8 content + style + placement
  -> validation and optional normalization
  -> font fallback and run segmentation
  -> shaping
  -> line and box layout
  -> glyph resource ensure
  -> positioned glyph instances
  -> FramePlan and DRP2 buffers/textures/draws
```

Rules:

1. retained `DvzText` objects preserve the original UTF-8 bytes and own semantic text state,
2. shaping output is independent from placement, panzoom, camera transforms, and atlas UVs,
3. layout output is independent from atlas page allocation,
4. atlas entries are keyed by selected font face and glyph id, not only by Unicode codepoint,
5. renderer resources are internal implementation details and must not become the primary public
   meaning of text,
6. `DvzText` state is the stable semantic source; glyph visuals and atlas resources are derived
   implementation details.


## Shaping Contract

Production text shaping should use HarfBuzz when available. The deterministic simple renderer may
remain available without HarfBuzz for tests, diagnostics, and dependency-light builds.

Required shaped-run data:

1. selected font face identity,
2. glyph ids,
3. clusters or byte-range mapping back to the original UTF-8 string,
4. advances,
5. glyph offsets,
6. direction, script, language, and feature metadata when available.

The first production slice may support one font face and left-to-right text, but internal structs
should leave room for fallback runs, script metadata, direction, language, and OpenType features.

Do not add a competing hand-written complex shaper. Without HarfBuzz, unsupported shaping features
should produce explicit diagnostics or documented fallback rendering.


## Font Fallback

Fallback happens before atlas lookup.

Staged policy:

1. shape with the selected font face first,
2. report missing glyphs explicitly,
3. render a visible missing-glyph box rather than silently dropping characters,
4. support user-provided fallback chains next,
5. add bundled or platform fallback only after deterministic tests cover it.

Atlas lookup receives `(font face, glyph id)` pairs. Unsupported codepoints are diagnostics and
fallback inputs, not stable atlas keys for production shaping.


## Layout Contract

Layout consumes shaped runs and produces positioned glyph instances in local label coordinates.

Required layout data:

1. advance bounds,
2. ink bounds,
3. baseline,
4. ascender and descender,
5. line height,
6. anchor offsets,
7. per-glyph local positions and optional rotations.

Layout invalidation depends on size, DPI/framebuffer scale, line spacing, wrap policy, alignment,
anchor, and metrics mode. Placement-only changes should not force reshaping, and panzoom/camera-only
changes should not force atlas rebuilds.

Math or equation layout should target this same contract by producing positioned glyph runs plus
rule, line, box, or background primitives.


## Atlas and Renderer Resource Contract

Atlas-backed renderers use explicit internal atlas objects rather than loose texture plus array
state.

Recommended key fields:

1. font face identity,
2. glyph id,
3. renderer backend,
4. size bucket or pixel height when the backend is size-specific,
5. load flags and hinting mode,
6. SDF/MSDF generator parameters,
7. pixel range,
8. generator version.

Required atlas entry data:

1. page id,
2. glyph id,
3. advance,
4. plane bounds,
5. atlas bounds,
6. UV rectangle,
7. renderer-specific pixel range or distance parameters.

The simple monospace renderer can keep its deterministic 6x8 atlas as a debug/test backend. The
normal quality path should be FreeType bitmap, SDF/MSDF, or a later vector renderer.


## Dynamic Growth

The mature atlas operation is "ensure shaped glyphs":

```text
shaped run glyph ids
  -> collect missing (font face, glyph id) pairs
  -> pack missing glyphs into existing pages when possible
  -> allocate new pages when needed
  -> generate bitmap/SDF/MSDF pixels
  -> upload new regions or pages
  -> return stable atlas entries
```

Rules:

1. existing atlas entries should remain stable across growth whenever possible,
2. any UV rescale or page migration must be visible to the generation counter and diagnostics,
3. text must not render with silently missing glyphs because an upload is pending,
4. page dimensions, padding, pixel range, and generator settings are cache identity,
5. atlas writes must be representable through normal FramePlan and DRP2 resource/update nodes.


## Cache Layers

Use distinct caches with independent invalidation:

1. font face cache: font path or embedded font id, face index, loading flags, variation settings,
2. shape cache: UTF-8 bytes, fallback chain, direction, script, language, features, normalization,
3. layout cache: shaped-run id, size, DPI, line spacing, wrap width, alignment, metrics mode,
4. atlas cache: font face, glyph id, renderer backend, size bucket, generator parameters,
5. emission cache: panel-local positioned quads, draw ranges, and DRP2 logical resource ids.

Panzoom and placement changes should update transforms or instance positions, not reshape text or
rebuild atlases.


## Embedded and Persistent Caches

Datoviz should eventually ship an embedded default atlas for a bundled default face and common ASCII
or Latin-1 labels if binary size permits. Embedded atlas metadata must be generated by the same code
path as runtime atlases and versioned by generator parameters.

Persistent disk cache is an explicit advanced-user feature, not default library behavior.

Rules:

1. no implicit writes to user directories,
2. cache files are versioned by Datoviz version and generator version,
3. stale or unreadable caches fail safely,
4. in-memory generation remains the deterministic fallback.


## DRP2 and FramePlan Emission

The first text implementation should use existing DRP2 primitives:

1. buffers for glyph vertices, instances, uniforms, and indices,
2. textures and texture writes for atlas pages,
3. samplers,
4. bind group layouts and bind groups,
5. scene shader registry modules,
6. render-pass draw commands.

Do not add text-specific DRP2 commands unless the vector text backend proves generic
buffers/textures/draws are insufficient for validation, replay, or portability.

Text emission must respect panel viewport/scissor, z-layer, depth policy, alpha mode,
DPI/framebuffer scale, and renderer capability checks.


## Renderer Roles

Renderer backends consume the same shaped/layout contract:

1. simple monospace atlas: deterministic fallback and tests,
2. FreeType bitmap atlas: small hinted text and compatibility fallback,
3. SDF/MSDF atlas: first production-quality scalable atlas path,
4. vector GPU text: later high-quality scalable path using outline or curve resources.

Renderer selection is a style or scene option. Automatic defaults are acceptable only when capability
adaptation reports visible semantic changes.


## Tests and Diagnostics

Focused coverage should include:

1. cache-key equality and inequality,
2. ASCII parity with the simple renderer,
3. dynamic atlas growth for new glyphs,
4. generation counters for append versus cache hit,
5. missing-glyph diagnostics and visible fallback,
6. no reshaping when only placement changes,
7. no atlas rebuild when only panzoom changes,
8. FreeType bitmap and MSDF append/growth paths,
9. capacity behavior near atlas limits,
10. DRP2 replay of text resource creation, atlas writes, and draw emission.

Diagnostics should expose requested glyphs, missing glyphs, growth events, fallback decisions,
cache hits, cache misses, and atlas capacity failures.


## Implementation Backlog

HarfBuzz:

1. add an optional private wrapper guarded by `DVZ_HAS_HARFBUZZ`;
2. define shaped-run keys and structs carrying selected font face, glyph ids, clusters, advances,
   offsets, direction, script, language, and features;
3. start with single-font left-to-right UTF-8 while preserving original UTF-8 bytes;
4. connect shaped glyph ids to atlas ensure/growth after atlas requests support `(font face,
   glyph id)`;
5. add user fallback chains before platform font fallback;
6. defer BiDi, ICU, and broad complex-script support until a concrete requirement activates them.

Atlas/cache:

1. add UV-stability regression coverage after atlas growth, allowing expected global rescale when
   texture dimensions change;
2. test FreeType bitmap and MSDF append/growth separately with representative glyphs such as `b`,
   `e`, `g`, `@`, punctuation, and dense lowercase strings;
3. formalize atlas/page/entry structs before migrating fully to `(font face, glyph id)` requests;
4. add atlas stats before any public cache API;
5. keep persistent disk cache explicit opt-in only.
