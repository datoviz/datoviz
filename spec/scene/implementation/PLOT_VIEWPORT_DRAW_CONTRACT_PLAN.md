# Plot Viewport Draw Contract Plan

Status: implemented for native scene/DRP2 emission; WebGPU adapter parity tightened.

Implementation note, June 2026: native frame-plan metadata, DRP2 draw packets, runtime
viewport/scissor emission, viewport uniforms, and axis grid plot-pixel basis are implemented. The
remaining maintenance burden is validation: keep WebGPU fixture/live paths accepting non-full
`SetViewport`/`SetScissor`, and keep the manual grid-shimmer smoke in release evidence.

This plan records the fix direction after the `path_axes_2d` grid shimmer regression. The
regression was bisected to `1c951c6b6` (`Use plot viewport for plot-clipped draws`). That commit
was not arbitrary: it tried to make plot-clipped `VIEW`/`DATA` draws use a plot viewport and plot
scissor consistently. The regression exposed that scene draw emission still lacks an explicit
per-draw viewport contract.


## Problem

Scene emission currently conflates several concepts:

1. viewport rectangle: how NDC maps to framebuffer pixels;
2. scissor rectangle: where fragments are clipped;
3. visual coordinate space: `DATA`, `VIEW`, or `PANEL`;
4. controller mode: fixed, apply, or view-projection;
5. CPU-expanded pixel geometry: quads for grid lines, ticks, spines, and similar strokes.

`DVZ_FRAME_PLAN_CLIP_RECT_PLOT` is currently used as a proxy for both plot clipping and plot
viewport selection. That is too implicit. It works for some `DATA`/`VIEW` visuals, but it breaks
CPU-expanded pixel geometry when the width is computed against panel pixels and then rasterized
through a smaller plot viewport.

Axis grids are the concrete failure mode:

```text
axis grid is plot-clipped
grid stroke width is expanded on CPU using panel pixel span
runtime switches plot-clipped draws to plot viewport
final rasterized grid width is no longer the requested pixel width
```

The symptom is grid lines that do not disappear completely but become thinner or shimmer while
panning by subpixel amounts.


## Target Contract

Every draw packet must carry independent viewport and scissor selections.

```c
typedef enum DvzFramePlanViewportRect
{
    DVZ_FRAME_PLAN_VIEWPORT_PANEL,
    DVZ_FRAME_PLAN_VIEWPORT_PLOT,
    DVZ_FRAME_PLAN_VIEWPORT_TARGET,
} DvzFramePlanViewportRect;
```

The existing `DvzFramePlanClipRect` remains a scissor selection, not a viewport selection.

```text
draw viewport = panel | plot | target
draw scissor  = panel | plot | target/none
```

Runtime emission must switch viewport from `viewport_rect` and scissor from `clip_rect`. It must not
infer one from the other.


## Visual Routing

Use this default routing unless a visual family has a documented reason to override it.

1. `DATA` and `VIEW` visuals that are authored for plot-local NDC use plot viewport and plot
   scissor.
2. Axis grid visuals use plot viewport and plot scissor once their CPU pixel expansion uses the
   plot pixel basis.
3. Axis ticks, spines, labels, panel backgrounds, borders, colorbar/legend derived visuals, and
   fixed overlays use panel viewport and panel scissor.
4. Full-target passes such as resolve/postprocess use target viewport and target scissor semantics.

This keeps the `1c951c6b6` data-visual motivation without forcing every plot-clipped visual into
the same implicit viewport behavior.


## Viewport Uniform Rule

The shader viewport uniform must match the selected draw viewport, not merely the panel render node.

This matters for visuals or shaders that compute pixel-sized geometry in shader code, such as
points, markers, impostors, strokes, or any future GPU-expanded lines. A draw rendered through a
plot viewport must not receive a panel viewport uniform unless that visual explicitly operates in
panel space.


## Axis Grid Rule

Axis-generated geometry must split by viewport basis.

1. Grid lines are `VIEW`/`DATA` aligned and plot-scissored.
2. Grid line stroke width, endpoint padding, and pixel snapping must use the plot pixel span when
   the grid draw uses plot viewport.
3. Ticks, spines, and labels are fixed panel overlays.
4. Tick and spine stroke width, text placement, and fixed-coordinate snapping must use the panel
   pixel span.
5. Endpoint overrun should remain pixel-derived; it should not be used to compensate for an
   incorrect viewport basis.

The temporary grid raster-stability workaround should be replaced by this contract. A `+1 px`
width guard may be useful as a final rasterization policy, but it should not hide a panel-versus-
plot pixel-basis mismatch.


## Implementation Status

Prefer small commits with visible validation for future follow-up work.

1. **Frame-plan ABI:** done. `DvzFramePlanViewportRect` is stored in render visual metadata and
   draw packets, with panel defaults for missing metadata.
2. **Scene routing:** done. `scene_emit/panel.c` sets `viewport_rect` and `clip_rect`
   independently.
3. **Runtime emission:** done. `render_emit_draws.c` switches viewport from `viewport_rect` and
   scissor from `clip_rect`.
4. **Viewport uniform:** done. Common bind groups are keyed by viewport selection and upload the
   matching viewport uniform.
5. **Axis grid basis:** done. Axis grid widths and snapping use plot-pixel basis; fixed axis
   visuals continue to use panel-pixel basis.
6. **Remove workaround:** resolved by the plot-pixel basis and pixel-phase grid snapping tests.
   Do not reintroduce larger endpoint margins or broad stroke inflation as a substitute.
7. **WebGPU parity:** active validation surface. The tracked WebGPU runner applies dynamic
   viewport/scissor commands, and the DVZR adapter must preserve non-full rectangles.


## Validation

Narrow loop:

```sh
just build
just test scene,axis
just example-c features/axes_2d --png
git diff --check
```

Manual proof:

```sh
just example-c features/axes_2d --live
```

Pan slowly by a few pixels. Grid lines should neither disappear nor change apparent thickness.

Focused automated coverage:

1. plot-clipped `DATA`/`VIEW` visuals emit plot viewport and plot scissor;
2. panel overlays emit panel viewport and panel scissor;
3. axis grid emits plot viewport and plot scissor;
4. axis ticks/spines emit panel viewport and panel scissor;
5. viewport uniforms match each draw's selected viewport rectangle;
6. CPU-expanded axis grid widths are computed from plot pixel spans;
7. CPU-expanded fixed axis widths are computed from panel pixel spans;
8. regression coverage preserves panning/grid alignment within an explicit subpixel tolerance.


## Non-Goals

1. Do not revert to a global panel viewport for all plot-clipped draws unless data-visual plot
   viewport requirements are proven unnecessary.
2. Do not migrate axis grids to `dvz_segment` as the first fix. It may be a later simplification,
   but it would bypass the missing viewport contract rather than fixing it.
3. Do not use larger endpoint margins or arbitrary stroke inflation as the architectural fix.
4. Do not add visual-family switches in runtime emission. Routing belongs in scene/frame-plan
   metadata; runtime should consume explicit draw state.
