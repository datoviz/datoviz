# Text Rendering Slice

This slice defines the first implementation-ready path for rendering retained `DvzText` objects.

It implements the smallest useful text renderer while preserving the broader text semantics in
[../semantics/TEXT.md](../semantics/TEXT.md) and the glyph visual contract in
[../visuals/GLYPH.md](../visuals/GLYPH.md).


## Scope

Implement visible text for retained `DvzText` objects attached to a panel.

The first slice supports:

1. UTF-8 strings stored on `DvzText`,
2. one font reference per text object, with a built-in fallback font when no font is supplied,
3. one run-level color and size per text object,
4. screen-space and data-space placement modes already exposed by `DvzTextPlacement`,
5. panel scissor/viewport clipping,
6. retained string, style, and placement updates,
7. offscreen and GLFW app rendering through scene -> `FramePlan` -> DRP2 -> vklite/canvas.


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

Use the installed APIs:

1. `dvz_font()`,
2. `dvz_font_destroy()`,
3. `dvz_text()`,
4. `dvz_text_destroy()`,
5. `dvz_text_set_string()`,
6. `dvz_text_set_style()`,
7. `dvz_text_set_placement()`.

Do not add a direct public glyph visual constructor in this slice.


## Retained State

The existing retained state in `DvzText` is the semantic source:

1. `scene`,
2. `panel`,
3. `string`,
4. `style`,
5. `placement`,
6. `flags`.

The implementation may add internal scene-owned caches:

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

The first renderer may lower all visible `DvzText` objects in a panel into one panel-local glyph
contribution.

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

The first DRP2 lowering should emit:

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

Add focused scene tests for:

1. text emission produces atlas, sampler, pipeline, buffer, and draw contributions,
2. changing a string updates text geometry and atlas coverage,
3. changing color avoids unnecessary atlas rebuild when practical,
4. screen-space text remains panel-clipped,
5. data-space text follows panzoom transform,
6. destroying a text object removes it from emission,
7. cross-scene font binding is rejected.

Add one app/offscreen smoke that captures a frame with visible text.


## Acceptance

This slice is complete when:

1. a retained `DvzText` appears in offscreen and GLFW app rendering,
2. no public atlas/glyph internals leak into the API,
3. text updates work across repeated frames,
4. focused tests cover emission, update, destroy, and validation paths,
5. `API_IMPLEMENTATION_READINESS.md` can move text from "retained only" to "rendered first slice".

