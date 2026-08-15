# Text Rendering Slice

This slice records the first rendered text path built around retained semantic `DvzText` objects.

The active implementation provides the smallest useful text renderer while preserving the broader
text semantics in
[../semantics/TEXT.md](../semantics/TEXT.md) and the glyph visual contract in
[../visuals/GLYPH.md](../visuals/GLYPH.md).

The implementation-facing shaping, layout, atlas, and cache contract lives in
[../implementation/TEXT_SHAPING_ATLAS.md](../implementation/TEXT_SHAPING_ATLAS.md).


## Current Implementation Status

Status: first rendered slice landed.

The first rendered text implementation has landed and the installed v0.4 public surface now uses
retained semantic text objects:

1. `dvz_text()` returns a `DvzText*` handle attached to a panel,
2. text content is supplied with `dvz_text_set_string()`,
3. style, placement, and renderer selection use `dvz_text_set_style()`,
   `dvz_text_set_placement()`, and `dvz_text_set_renderer()`,
4. text lowers internally to scene-owned glyph visuals and atlas resources,
5. offscreen/runtime readback tests verify visible bitmap and SDF-backed text.

Glyph visuals remain derived implementation output or an explicit low-level escape hatch; they are
not the normal public path for labels, annotations, axes, legends, colorbars, or readouts.


## Scope

Support visible text attached to a panel through a retained semantic `DvzText` object.

The first semantic text slice supports:

1. UTF-8 strings stored on a retained text object,
2. a built-in fallback bitmap font and font-backed SDF/MSDF-capable atlas behavior,
3. run/per-string color and size through text style descriptors,
4. explicit screen-space placement in logical pixels,
5. panel viewport/scissor participation through the normal render path,
6. retained string, style, placement, renderer, visibility, resize, and destroy updates,
7. offscreen app rendering and runtime readback through scene -> `FramePlan` -> DRP2 ->
   vklite/canvas.

Remaining first-slice follow-ups:

1. finish data/world-space retained text placement and depth policy,
2. harden DPI behavior and panel clipping edge cases,
3. improve explicit diagnostics for renderer fallback, missing glyphs, and unsupported modes,
4. add or record bounded GLFW/manual smoke coverage.


## Non-Goals

Do not implement these in the first text rendering slice:

1. glyph-level picking,
2. substring selection,
3. rich text style runs,
4. bidirectional shaping or complex script shaping,
5. equation layout,
6. color emoji,
7. collision avoidance,
8. public atlas manipulation APIs.

Unsupported text content or style features must either render through a documented fallback or emit a
scene diagnostic.


## Public API Boundary

Use the installed semantic public APIs for this slice:

1. `dvz_font()`,
2. `dvz_font_destroy()`,
3. `dvz_text()` returning `DvzText*`,
4. `dvz_text_destroy()`,
5. `dvz_text_set_string()`,
6. `dvz_text_set_style()`,
7. `dvz_text_set_placement()`,
8. `dvz_text_set_renderer()`.

`dvz_glyph()` may remain as a low-level visual constructor or internal derived visual path, but it
must not be the normal public API for labels, annotations, axes, legends, or readouts.


## Retained State

The retained state in `DvzText` is the semantic source:

1. `scene`,
2. `panel`,
3. `string`,
4. `style`,
5. `placement`,
6. `flags`.

The current implementation stores semantic text state on `DvzText` and uses internal derived glyph
visuals. Derived glyph visuals and caches remain internal implementation details:

1. font face identity,
2. glyph metrics,
3. atlas page image and dirty regions,
4. per-text layout generation,
5. per-panel derived quad ranges.

These caches must be internal and invalidated from semantic state changes.


## Dirty Rules

Text invalidation must distinguish:

1. string changes: layout and glyph coverage dirty,
2. style changes: layout dirty when size/font changes; upload-only dirty when color changes,
3. placement changes: geometry dirty, not atlas dirty,
4. panel size or DPI changes: screen-space geometry dirty and atlas may become dirty,
5. font destruction: dependent text objects become invalid or fall back with diagnostics.

The first implementation may rebuild all text geometry for the affected panel when any text object
changes. It should not rebuild unrelated visual-family resources.


## Validation

Validate before planning:

1. text panel is still live and belongs to the text scene,
2. referenced font is NULL or belongs to the same scene,
3. font size is positive and finite,
4. placement coordinates are finite,
5. string storage is valid UTF-8 or is downgraded with a diagnostic,
6. required runtime capability for sampled atlas text is available.


## FramePlan Contribution

The first renderer lowers visible text objects into glyph visual contributions.

The contribution contains:

1. atlas texture resource reference,
2. sampler reference,
3. glyph quad vertex buffer,
4. glyph index buffer or non-indexed quad expansion,
5. text parameter uniform for projection/viewport if needed,
6. draw range per panel,
7. z-order after ordinary data visuals unless explicit text flags request another layer.

The contribution must remain scene-owned producer data and must not expose Vulkan handles.


## DRP2 Lowering

The first DRP2 lowering emits the standard generic resources for glyph visuals:

1. atlas texture create/write commands,
2. sampler create command,
3. vertex/index buffer create/write commands for glyph quads,
4. shader module and render pipeline for atlas text,
5. bind group layout and bind group for atlas sampler/texture plus uniforms,
6. render-pass draw commands inside the panel viewport/scissor.

Use stable logical ids so unchanged atlas and pipeline resources are reused across frames.


## Shader Contract

The first shader contract should be deliberately small:

1. vertex input: position, UV, color,
2. uniform input: panel transform or final clip-space transform,
3. sampled atlas texture,
4. alpha output multiplied by run/glyph color.

The first implementation may use a monochrome alpha atlas. SDF/MSDF quality upgrades are allowed
later without changing the public text model.


## Tests

Existing focused tests cover:

1. bitmap text realization into glyph visual attributes,
2. SDF/MSDF-capable text realization and atlas encoding,
3. automatic renderer selection,
4. UTF-8 atlas growth and field reuse,
5. missing-glyph fallback,
6. many-label batching through one glyph visual,
7. runtime/offscreen readback of visible text,
8. app/offscreen visible-pixel smoke for bitmap and SDF-backed text.

Remaining focused tests to add or harden:

1. text emission produces atlas, sampler, pipeline, buffer, and draw contributions,
2. changing a string updates text geometry and atlas coverage,
3. changing color avoids unnecessary atlas rebuild when practical,
4. screen-space text remains panel-clipped,
5. data-space text follows panzoom transform,
6. destroying a semantic text object removes it from emission,
7. cross-scene font binding is rejected.


## Acceptance

This slice is complete enough for v0.4 explanatory-object integration when:

1. text appears in offscreen and bounded GLFW/manual app rendering,
2. public atlas/glyph internals are hidden behind the semantic API or restricted to a clearly
   low-level `dvz_glyph()` escape hatch,
3. text updates work across repeated frames,
4. focused tests cover emission, update, destroy, and validation paths,
5. API readiness notes record text as a rendered first slice whose v0.4 target surface is
   `DvzText*`.


## Remaining Release-Facing Work

1. Keep `DvzText*` as the public semantic text surface and `dvz_glyph()` low-level or derived.
2. Wire axes, colorbars, legends, and readouts through semantic text rather than visual-attribute
   glyph calls.
3. Finish data/world placement and depth policy.
4. Harden DPI scaling and panel scissor clipping.
5. Keep the simple renderer available for diagnostics; keep FreeType optional until fallback
   diagnostics are solid.
6. Keep vector text and equation rendering as later optional lanes.
