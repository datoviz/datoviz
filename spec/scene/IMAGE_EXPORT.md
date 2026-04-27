# Image Export And Offscreen Rendering

This document defines the scene-level contract for still image capture, render scale
(supersampling), and panel-as-texture offscreen rendering.

Video export is covered in `ANIMATION.md`.


## Still Image Capture

Capturing a single frame as an image (PNG, RGBA buffer, etc.) is not a scene concern.

The scene's job is to emit a `FramePlan`.
The canvas and runtime are responsible for capturing the rendered output.

The scene-level contract for still image capture is:

1. the application may drive the scene in offline clock mode for a single frame,
2. the scene builds and emits a `FramePlan` as usual,
3. the runtime executes the plan and makes the rendered output available to the canvas,
4. the canvas performs the readback and delivers it to the image sink (file, buffer, callback).

The scene has no knowledge of whether the rendered frame is displayed, saved, or discarded.

**Programmatic single-frame render:**

```text
dvz_scene_frame_advance(scene)   // step the scene by one frame (offline mode)
// canvas/runtime captures the result
```

No special scene API is needed.
The offline clock mode (`DVZ_CLOCK_OFFLINE`) already expresses this correctly.


## Render Scale (Supersampling)

A render scale multiplies the logical canvas size to produce a higher-resolution output.

```text
dvz_figure_set_render_scale(figure, scale)
```

`scale` is a positive `float32`, typically `2.0` or `4.0` for publication-quality output.

**Effect:**

1. the runtime allocates render targets at `scale × logical_size`,
2. the scene emits a `FramePlan` as usual — it does not change its coordinate model,
3. all scene coordinates remain in logical pixels; the scene is unaware of physical resolution,
4. the runtime downsamples the rendered output to the logical size before display or capture.

**Interaction with high-DPI:**

Render scale stacks with device pixel ratio (see `HIGH_DPI.md`).
The effective physical resolution is `dpi_scale × render_scale × logical_size`.
Both scale factors are applied by the runtime, not the scene.

**Render scale scope:** `dvz_figure_set_render_scale` is a figure-level call in v0.4. All
panels in a figure scale together. Per-panel render scale is deferred to v0.4+.

**Downsampling filter:** bilinear for v0.4. Lanczos and other high-quality filters are v0.4+.


## Panel-As-Texture

A panel can be designated as an offscreen texture source.
Its rendered output is available as a logical texture handle that can be used as a
visual attribute anywhere a texture is accepted.


### Declaration

```text
DvzTexture* tex = dvz_panel_set_offscreen(panel, flags)
```

`dvz_panel_set_offscreen` designates `panel` as an offscreen render source and returns
a logical texture handle owned by the scene.

`flags` controls compositing behaviour:

| Flag | Effect |
|---|---|
| `DVZ_PANEL_OFFSCREEN_DEFAULT` (0) | exclusive — panel renders to texture only, not to the main framebuffer |
| `DVZ_PANEL_OFFSCREEN_PIP` | picture-in-picture — panel renders to both the texture and the main framebuffer |

The default (exclusive) mode is the right choice when the offscreen texture is consumed by
another visual or ImGui widget. Use `DVZ_PANEL_OFFSCREEN_PIP` when you want the offscreen
panel to also appear in its normal position on screen.

The returned handle can be passed to any visual that accepts a texture attribute:

```text
dvz_visual_set_texture(image_visual, tex)
```

This allows the rendered output of one panel to appear as a texture in another visual,
in another panel, or in an ImGui image widget (`dvz_gui_image`).


### FramePlan Ordering

The scene tracks the dependency between the offscreen panel and any downstream visual
that references its texture.

The `FramePlan` orders their render nodes correctly:

```text
FramePlan:
  RenderNode  — offscreen panel (renders to texture)
  RenderNode  — downstream panel (samples the texture)
```

This ordering is mandatory.
The scene enforces it during frame planning.
The user does not need to specify it explicitly.


### Constraints

1. **No cycles** — a panel may not (directly or transitively) depend on its own texture.
   The scene detects cycles during validation and reports an error.
2. **Texture size** — by default the offscreen panel renders at its declared logical size.
   A separate size may be declared:

   ```text
   dvz_panel_set_offscreen_size(panel, width, height)
   ```

3. **Format** — the texture format is `rgba_u8` by default.
   Passing a `DvzTexture*` created with a float format (e.g., `VK_FORMAT_R32G32B32A32_SFLOAT`
   via `dvz_texture_2d`) gives an HDR-capable offscreen target.
4. **Multiple consumers** — a single offscreen panel texture may be referenced by multiple
   downstream visuals.
5. **Recursion depth** — more than one level of indirection (A → B → C) is allowed as long
   as there are no cycles.


### Relationship To `dvz_gui_image`

`dvz_gui_image(tex, w, h)` already accepts a logical texture handle (see `EXTERNAL_UI.md`).
An offscreen panel texture obtained from `dvz_panel_set_offscreen` is a valid input:

```text
DvzTexture* tex = dvz_panel_set_offscreen(panel)
dvz_gui_image(tex, width, height)
```

This is the scene-native path for embedding a rendered panel inside an ImGui window.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `ANIMATION.md` | video export workflow using offline clock mode |
| `HIGH_DPI.md` | render scale stacks with device pixel ratio |
| `FRAME_PLAN_IR.md` | offscreen panel ordering in FramePlan |
| `EXTERNAL_UI.md` | `dvz_gui_image` accepts offscreen panel textures |
| `RESOURCE_MODEL.md` | offscreen texture is a scene-owned `TextureResource` |
