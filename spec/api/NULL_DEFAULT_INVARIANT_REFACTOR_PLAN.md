# NULL Default Invariant Refactor Plan

Status: approved v0.4 API cleanup plan.

This plan defines a cross-module API invariant and the concrete refactor work needed before the
v0.4 public API freezes. It is written for a follow-up implementation agent.


## Invariant

For every public function whose documentation says that a pointer-passed config, descriptor, style,
or request may be `NULL` for defaults, `NULL` must be semantically equivalent to passing the public
canonical initializer result:

```c
dvz_thing_create(NULL);

DvzThingConfig cfg = dvz_thing_config();
dvz_thing_create(&cfg);
```

The same rule applies to `Desc`, `Style`, and `Request` records:

```c
dvz_thing_set_style(thing, NULL);

DvzThingStyle style = dvz_thing_style();
dvz_thing_set_style(thing, &style);
```

If those two calls should not be equivalent, the API should not document `NULL` as "defaults".
Instead, split the operation into explicit concepts such as clear, disable, inherit, reset, or
context-resolved defaults.


## Design Rules

1. Public initializer functions return the canonical public default record.
2. Public `NULL for defaults` entry points resolve through that initializer, not through duplicated
   field literals.
3. Context inheritance must not depend on whether the caller passed `NULL`.
4. Config and descriptor structs should not mix local options with inherited app, scene, or runtime
   policy.
5. If a field needs contextual inheritance, use an unresolved sentinel value in the public record
   and a single internal resolver for both `NULL` and explicit default records.
6. If `NULL` means clear or disable, document that directly and keep it out of this invariant.
7. Required data descriptors should reject `NULL`; do not add fake defaults just for uniformity.


## Confirmed Semantic Violations

These cases currently produce different behavior for `NULL` and an explicit initialized default
record. Fix these first.

### 1. GUI Font Defaults

Current behavior:

```c
dvz_view_gui(view, NULL);
```

inherits `win->app->config.font_defaults`, but:

```c
DvzGuiConfig cfg = dvz_gui_config();
dvz_view_gui(view, &cfg);
```

uses the built-in `dvz_font_defaults()` embedded in `DvzGuiConfig`.

Relevant files:

- `include/datoviz/gui.h`
- `src/gui/gui.cpp`
- `src/app/app.c`
- `src/gui/tests/test_gui.cpp`
- examples using `DvzGuiConfig gui_config = dvz_gui_config();`

Recommended API break:

1. Remove `DvzFontDefaults font_defaults` from `DvzGuiConfig`.
2. Resolve GUI fonts from app or scene policy in the GUI creation path.
3. Keep `DvzGuiConfig` limited to GUI-local options such as GUI flags and `.ini` path.
4. If GUI-specific font override is still required, add an explicit API instead of burying it in
   the general config:

```c
int dvz_view_gui_set_font_defaults(DvzView* view, const DvzFontDefaults* defaults);
```

or defer the override until a real user need appears.

Implementation notes:

1. Change `dvz_view_gui()` to compute:

```c
DvzGuiConfig cfg = config != NULL ? *config : dvz_gui_config();
DvzFontDefaults fonts = _view_gui_font_defaults(view);
```

2. Pass the resolved fonts separately to the internal GUI creation code.
3. Update generated docs and raw binding snapshots after the struct change.
4. Replace examples that do not customize GUI options with `dvz_view_gui(view, NULL)`.
5. Add a test proving `NULL` and `dvz_gui_config()` behave the same when app fonts are customized.


### 2. Retained Text Style Font Size

Current behavior:

```c
dvz_text_set_style(text, NULL);
```

uses `_text_default_style(text->scene)` and inherits `scene->font_defaults.text_size_px`, but:

```c
DvzTextStyle style = dvz_text_style();
dvz_text_set_style(text, &style);
```

uses `dvz_font_defaults().text_size_px`.

Relevant files:

- `include/datoviz/scene/text.h`
- `src/scene/annotation/text.c`
- `src/scene/tests/interaction.c`

Recommended API break:

1. Make `DvzTextStyle` an unresolved public style descriptor.
2. Change `dvz_text_style()` so `size_px == 0.0f` means "inherit scene font size".
3. Add one resolver used by text creation, `NULL`, and explicit default style records:

```c
static DvzTextStyle _text_resolve_style(const DvzScene* scene, const DvzTextStyle* style);
```

4. Keep explicit nonzero `size_px` as a hard override.
5. Add tests with custom scene font defaults proving `NULL` and `dvz_text_style()` are equivalent.

Preferred behavior after refactor:

```c
DvzTextStyle style = dvz_text_style(); // unresolved/inheriting size
style.size_px = 18.0f;                 // explicit override only when nonzero
```


### 3. Overlay Card Descriptor

Current behavior:

```c
dvz_overlay_card(overlay, NULL);
```

keeps `_scene_card_init()` defaults such as `offset_px = {12, 12}`. Passing:

```c
DvzOverlayCardDesc desc = dvz_overlay_card_desc();
dvz_overlay_card(overlay, &desc);
```

uses the all-zero descriptor and overwrites placement, anchor, and offset fields with zero values.

Relevant files:

- `include/datoviz/scene/overlay.h`
- `src/scene/interaction/core.c`
- overlay/card tests in `src/scene/tests/interaction.c`

Recommended API break:

1. Decide whether `DvzOverlayCardDesc` is a full state descriptor or a sparse override descriptor.
2. Preferred: make it a full state descriptor and make `dvz_overlay_card_desc()` contain the same
   placement and offset defaults as `_scene_card_init()`.
3. Add a resolver:

```c
static DvzOverlayCardDesc _overlay_card_resolve_desc(const DvzOverlayCardDesc* desc);
```

4. Apply the resolved descriptor unconditionally.
5. Add a test comparing a `NULL` card with a card created from `dvz_overlay_card_desc()`.

Alternative if sparse override semantics are preferred:

1. Split creation options into a smaller descriptor that only contains explicit optional fields.
2. Do not call all-zero `dvz_overlay_card_desc()` a complete default record.


### 4. Canvas Video Sink Context Inheritance

Current behavior:

```c
dvz_canvas_configure_video_sink(canvas, true, NULL);
```

uses `dvz_video_sink_config()` and then inherits encoder width, height, and FPS from the stream.
Passing:

```c
DvzVideoSinkConfig cfg = dvz_video_sink_config();
dvz_canvas_configure_video_sink(canvas, true, &cfg);
```

keeps the sink's built-in 1080p60 encoder dimensions.

Relevant files:

- `include/datoviz/canvas.h`
- `include/datoviz/video.h`
- `src/canvas/canvas_stream.c`
- `src/video/video_sink.c`
- canvas/video sink tests

Recommended API break:

1. Make `DvzVideoEncoderConfig.width`, `height`, and `fps` unresolved when zero.
2. Change `dvz_video_encoder_config()` and `dvz_video_sink_config()` to use zero for inheritable
   stream dimensions and FPS if they are used through a stream/canvas.
3. Add a single resolver:

```c
static DvzVideoSinkConfig
_canvas_resolve_video_sink_config(const DvzStreamConfig* stream_cfg, const DvzVideoSinkConfig* cfg);
```

4. Use this resolver for both `NULL` and explicit configs.
5. If standalone video encoder creation still needs concrete 1080p60 defaults, add a separate
   standalone helper or resolve zero in `dvz_video_encoder_create()`.
6. Update docs: zero means inherit in stream/canvas use, concrete values force encoder dimensions.


## Drift Risks To Clean Up

These are probably equivalent today, but they implement `NULL` defaults with duplicated literals or
field-by-field branches. They should be normalized after the confirmed semantic bugs.

### Scale, Colormap, Colorbar, Legend

Relevant files:

- `src/scene/annotation/scale.c`
- `src/scene/annotation/colormap.c`
- `src/scene/annotation/colorbar.c`
- `src/scene/annotation/legend.c`

Refactor pattern:

```c
DvzColorbarDesc resolved = desc != NULL ? *desc : dvz_colorbar_desc();
```

Then normalize derived or sentinel fields from `resolved`.

Avoid:

```c
desc != NULL ? desc->field : SAME_LITERAL_AS_DEFAULT_HELPER
```

Tests:

1. Create each object with `NULL`.
2. Create the same object with the public initializer.
3. Compare retained fields that are part of public behavior.


### Visual Attachment

Relevant files:

- `include/datoviz/scene.h`
- `src/scene/visuals/families.c`

Current code uses inline literals for `z_layer`, `controller_mode`, and `coord_space`. Replace with
`dvz_visual_attach_desc()` in the `NULL` path.


### Text Placement

Relevant files:

- `include/datoviz/scene/text.h`
- `src/scene/annotation/text.c`

This appears equivalent today because `_text_default_placement()` delegates to
`dvz_text_placement()`. Keep the resolver but make the invariant explicit in tests.


### Axis And Visual Styles

Relevant files:

- `src/scene/annotation/axis.c`
- `src/scene/visuals/point/api.c`
- `src/scene/visuals/marker/api.c`
- `src/scene/visuals/vector/api.c`
- `src/scene/visuals/material.c`

Most paths already call the public style or desc initializer for `NULL`. Keep them as examples of
the desired pattern. Add invariant tests only where the test cost is low.


## APIs That Should Stay Out Of The Invariant

Do not force the invariant onto APIs where `NULL` means something other than defaults:

1. `NULL to clear`: labels, strings, field bindings, symbol bindings, shader/transform slots.
2. `NULL to disable`: EDL, MSAA, SSAO, depth cueing, volume occlusion, scene occlusion.
3. `NULL for zero`: grid margins, panel reserves, padding.
4. `NULL output pointer`: optional out parameters.
5. Required descriptors: scene buffers, scene compute, polygon geometry, many low-level runtime
   constructors.

For these APIs, keep documentation direct. Avoid phrases like "optional defaults" when the behavior
is clear, disable, zero, or required.


## Implementation Order

Use one commit per logical slice after checks pass.

1. Keep the invariant wording in `spec/api/PUBLIC_API_CONVENTIONS.md` reconciled with this plan.
2. Fix GUI font defaults and examples.
3. Fix retained text style inheritance.
4. Fix overlay card default descriptor.
5. Fix canvas/video sink inheritance.
6. Normalize scale, colormap, colorbar, legend, and visual attachment defaulting.
7. Add or extend invariant tests.
8. Regenerate affected public C API docs and binding snapshots if the touched structs changed.


## Suggested Tests

Add focused tests rather than broad integration tests:

1. `test_gui_null_default_equivalence`: custom app font defaults, create GUI via `NULL` and explicit
   `dvz_gui_config()` in separate views, assert resolved GUI font policy matches.
2. `test_text_style_null_default_equivalence`: custom scene font default size, apply `NULL` and
   `dvz_text_style()` to two text objects, assert retained style resolves identically.
3. `test_overlay_card_null_default_equivalence`: create two overlay cards and compare placement,
   offset, padding, min size, renderer, and visibility.
4. `test_canvas_video_sink_null_default_equivalence`: configure one canvas with `NULL` and one with
   `dvz_video_sink_config()`, assert resolved sink encoder config matches after stream inheritance.
5. `test_scene_adornment_null_default_equivalence`: colorbar and legend created with `NULL` and
   explicit defaults retain the same placement, anchor, reserve, gap, text renderer, and flags.

For runtime-heavy tests, keep skip behavior consistent with existing Vulkan/GUI availability checks.


## Documentation Updates

Update comments for each refactored API:

1. For true defaults:

```c
@param config configuration, or NULL for dvz_thing_config()
```

2. For contextual resolution:

```c
@param style style override, or NULL for dvz_text_style(); unresolved fields inherit scene policy
```

3. For clear/disable:

```c
@param desc descriptor to enable the feature, or NULL to disable it
```

Do not use "optional" alone when the meaning of `NULL` matters.


## Acceptance Criteria

1. Every public "NULL for defaults" function either delegates to the public initializer or uses a
   resolver shared by `NULL` and explicit initializer paths.
2. No context inheritance depends on the pointer being `NULL`.
3. Confirmed violations above have focused tests.
4. Public headers and generated docs describe `NULL` behavior precisely.
5. `git diff --check`, relevant focused tests, and the necessary generated-doc checks pass.
