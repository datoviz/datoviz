# Image Export And Offscreen Rendering

This document defines the scene-level contract for still image capture and records future
render-scale and panel-as-texture directions.

Video export is covered in `interaction/ANIMATION.md`.


## Still Image Capture

Capturing a single frame as an image (PNG, RGBA buffer, etc.) is not a scene concern.

The scene's job is to emit a `FramePlan`.
The canvas and runtime are responsible for capturing the rendered output.

The scene-level contract for still image capture is:

1. the application creates `dvz_scene()`, `dvz_figure()`, `dvz_app()`, and a `DvzView`,
2. `DvzView` drives one frame with `dvz_view_render_once()` or `dvz_app_run()`,
3. the scene builds a `FramePlan` as usual,
4. the app layer emits a `DvzDrp2CommandStream`,
5. `DvzDrp2Runtime` executes the stream through vklite/canvas,
6. the app/canvas layer performs readback and delivers it to the image sink.

The scene has no knowledge of whether the rendered frame is displayed, saved, or discarded.

**Programmatic single-frame render:**

```text
dvz_view_render_once(win)
dvz_view_capture_png(win, "frame.png")
```

No special scene API is needed. The lower-level canvas capture API remains available through
`dvz_view_canvas(win)` when callers need RGBA buffers instead of PNG files.


## DRP2 Recording And Replay

The active app layer can record and replay the DRP2 command stream for a view:

```text
dvz_view_record_start(win, "capture.dvzr")
dvz_view_render_once(win)
dvz_view_record_stop(win)

dvz_view_replay_start(win, "capture.dvzr")
dvz_view_render_once(win)
dvz_view_replay_stop(win)
```

A `.dvzr` recording is an app/runtime artifact. It records backend-agnostic DRP2 command streams;
it does not make scene semantics depend on export or replay.


## Future Render Scale (Supersampling)

A render scale would multiply the logical canvas size to produce a higher-resolution output. This
is not active in the current view path and should remain a future API direction until runtime
allocation, downsampling, capture, and hosted-surface behavior are implemented together.

```text
dvz_figure_set_render_scale(figure, scale)
```

`scale` is a positive `float32`, typically `2.0` or `4.0` for publication-quality output.

**Intended effect:**

1. the runtime allocates render targets at `scale × logical_size`,
2. the scene emits a `FramePlan` as usual — it does not change its coordinate model,
3. all scene coordinates remain in logical pixels; the scene is unaware of physical resolution,
4. the runtime downsamples the rendered output to the logical size before display or capture.

**Interaction with high-DPI:**

Render scale stacks with device pixel ratio (see `integration/HIGH_DPI.md`).
The effective physical resolution is `dpi_scale × render_scale × logical_size`.
Both scale factors should be applied by the runtime, not the scene.

**Render scale scope:** if activated, `dvz_figure_set_render_scale` should be a figure-level call.
All panels in a figure should scale together. Per-panel render scale remains deferred.

**Downsampling filter:** bilinear remains the first expected filter. Lanczos and other
high-quality filters are later work.


## Future Panel-As-Texture

A panel-as-texture path is not active in the current scene/app implementation. The intended design
is preserved here for later implementation: a panel can be designated as an offscreen texture
source.
Its rendered output is available as a logical texture handle that can be used as a
visual attribute anywhere a texture is accepted.


### Declaration

```text
DvzTexture* tex = dvz_panel_set_offscreen(panel, flags)
```

`dvz_panel_set_offscreen` would designate `panel` as an offscreen render source and return
a logical texture handle owned by the scene.

`flags` would control compositing behaviour:

| Flag | Effect |
|---|---|
| `DVZ_PANEL_OFFSCREEN_DEFAULT` (0) | exclusive — panel renders to texture only, not to the main framebuffer |
| `DVZ_PANEL_OFFSCREEN_PIP` | picture-in-picture — panel renders to both the texture and the main framebuffer |

The default (exclusive) mode would be the right choice when the offscreen texture is consumed by
another visual or ImGui widget. Use `DVZ_PANEL_OFFSCREEN_PIP` when you want the offscreen
panel to also appear in its normal position on screen.

The returned handle would be passed to any visual that accepts a texture attribute:

```text
dvz_visual_set_texture(image_visual, tex)
```

This would allow the rendered output of one panel to appear as a texture in another visual,
in another panel, or in an ImGui image widget (`dvz_gui_image`).


### FramePlan Ordering

The scene would track the dependency between the offscreen panel and any downstream visual
that references its texture.

The `FramePlan` would order their render nodes correctly:

```text
FramePlan:
  RenderNode  — offscreen panel (renders to texture)
  RenderNode  — downstream panel (samples the texture)
```

This ordering would be mandatory.
The scene would enforce it during frame planning.
The user would not need to specify it explicitly.


### Constraints

1. **No cycles** — a panel may not (directly or transitively) depend on its own texture.
   The scene should detect cycles during validation and report an error.
2. **Texture size** — by default the offscreen panel would render at its declared logical size.
   A separate size may be declared:

   ```text
   dvz_panel_set_offscreen_size(panel, width, height)
   ```

3. **Format** — the texture format should be `rgba_u8` by default.
   Passing a `DvzTexture*` created with a float format should give an HDR-capable offscreen target.
4. **Multiple consumers** — a single offscreen panel texture may be referenced by multiple
   downstream visuals.
5. **Recursion depth** — more than one level of indirection (A → B → C) is allowed as long
   as there are no cycles.


### Relationship To `dvz_gui_image`

If `dvz_gui_image(tex, w, h)` accepts a logical texture handle (see
`integration/EXTERNAL_UI.md`), a future offscreen panel texture obtained from
`dvz_panel_set_offscreen` should be a valid input:

```text
DvzTexture* tex = dvz_panel_set_offscreen(panel)
dvz_gui_image(tex, width, height)
```

This remains the intended scene-native path for embedding a rendered panel inside an ImGui window.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `interaction/ANIMATION.md` | video/export timing context |
| `integration/HIGH_DPI.md` | future render scale should stack with device pixel ratio |
| `pipeline/FRAME_PLAN.md` | future offscreen panel ordering in FramePlan |
| `integration/EXTERNAL_UI.md` | future `dvz_gui_image` offscreen-panel consumption |
| `pipeline/RESOURCE_MODEL.md` | future offscreen texture as a scene-owned `TextureResource` |
