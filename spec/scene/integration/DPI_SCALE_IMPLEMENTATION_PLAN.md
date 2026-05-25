# DPI And User Scale Implementation Plan

Status: implementation roadmap.

This plan records the remaining work needed to make Datoviz v0.4 work out of the box on standard
and high-DPI displays while keeping user-controlled UI scaling separate from display density. The
normative coordinate model remains [HIGH_DPI.md](HIGH_DPI.md).


## Target Behavior

1. A `1280 x 720` interactive view has the same logical size on a standard monitor and on a Retina
   display.
2. A Retina-like display allocates and renders to a larger physical framebuffer, usually
   `2 x logical_size`, so graphics and text are crisp.
3. Pointer, wheel, layout, panel query, picking, and controller coordinates are expressed in logical
   pixels at the scene boundary.
4. Screen-space visual quantities that are authored in logical pixels keep the same apparent size on
   standard and high-DPI displays.
5. A dynamic user scale can enlarge or shrink UI-like quantities without changing data/world-space
   quantities.


## Current State

The window/input layer already carries the right primitives:

| Area | Current state |
|---|---|
| GLFW windows | Captures logical window size, physical framebuffer size, and content scale. |
| Window surface | Stores physical `extent` and `scale_x` / `scale_y`. |
| Input events | Resize events carry framebuffer size, logical window size, and content scale. |
| Canvas/swapchain | Uses the window-surface physical extent for presentation. |
| Hosted/wrap backends | Can receive external physical extent and content scale from the host. |

The main gap is consistency above this layer. The live app path currently syncs the figure size from
the physical framebuffer size, while public scene API comments and `HIGH_DPI.md` define figure,
layout, and panel sizes as logical pixels. This makes high-DPI interaction possible in the current
slice, but it blurs the boundary between logical scene coordinates and physical render targets.

There is also a DRP2 contract mismatch to resolve: `spec/drp2/COMMANDS.md` describes viewport and
scissor values as framebuffer coordinates, while the current vklite path treats them as normalized
render-target rectangles.


## Scale Vocabulary

| Name | Owner | Meaning | Dynamic | Applies to |
|---|---|---|---|---|
| `device_scale` | window/runtime | physical pixels per logical pixel | yes, when moving displays | framebuffer realization, DPI-aware rasterization |
| `render_scale` | view/export/runtime | supersampling or export resolution multiplier | yes | render target allocation only |
| `user_scale` | scene/view/application | user preference for UI-like sizing | yes | margins, text, strokes, markers, hit slop |

These factors stack only where explicitly stated. A default interactive view uses:

```text
physical_size = logical_size * device_scale * render_scale
```

`user_scale` must not change physical render-target size.


## Quantity Classes

| Class | Examples | Scale rule |
|---|---|---|
| Layout logical pixels | figure size, panel rects, grid gutters, reserves, padding | authored in logical pixels; runtime maps to physical |
| UI-style pixels | margins, tick gaps, label offsets, text size, line width, point/marker diameter, pixel visual size, selection halo, hit tolerance | multiply by `device_scale * user_scale` when lowering to physical shader/raster units |
| Raster detail | text atlas size, bitmap glyphs, MSDF range, offscreen resolve resources | multiply by `device_scale * render_scale`, and by `user_scale` only if the authored logical size changed |
| Data/world quantities | mesh coordinates, sphere radius in data/world units, volume spacing, camera distance, sampled-field dimensions | never scaled by `user_scale`; only projected by camera/transform |
| Image/export resolution | screenshots, video frames, still image export | controlled by `render_scale` plus any requested output extent |


## Implementation Phases

### 1. Make Size State Explicit

Add explicit size/scale state at the app/view boundary:

| Field | Source |
|---|---|
| `logical_width`, `logical_height` | window size or hosted resize event |
| `framebuffer_width`, `framebuffer_height` | window surface extent |
| `device_scale_x`, `device_scale_y` | window content scale |
| `render_scale` | view/export setting, default `1.0` |
| `user_scale` | view/scene setting, default `1.0` |

Acceptance criteria:

1. `dvz_figure_size()` returns logical size for interactive and hosted views.
2. The canvas frame target remains physical framebuffer size.
3. Moving a window between displays updates `device_scale` and requests a redraw.
4. Existing offscreen paths keep deterministic defaults with `device_scale = 1.0`.


### 2. Fix The Scene-To-DRP2 Viewport Contract

Choose and enforce one DRP2 viewport/scissor representation. The recommended direction is:

1. DRP2 `SetViewport` and `SetScissor` use framebuffer coordinates as specified.
2. Scene `FramePlan` may keep normalized panel descriptors for layout and serialization.
3. Runtime emission converts logical panel rectangles to physical framebuffer rectangles before
   writing DRP2 viewport/scissor commands.
4. Common viewport uniforms used by shaders carry physical pixel dimensions when shader math is in
   framebuffer pixels.

Acceptance criteria:

1. DRP2 tests cover non-fullscreen viewport/scissor in framebuffer coordinates.
2. Scene multi-panel tests verify the emitted DRP2 commands use physical extents.
3. Shader viewport uniforms and raster viewport/scissor commands agree.


### 3. Add Screen-Scale Resolution

Introduce one internal resolver for screen-space quantities:

```text
physical_style_px = logical_style_px * device_scale * user_scale
```

Use it for visual attributes and style blocks that are defined in logical pixels:

1. point and marker `diameter`,
2. point and marker `stroke_width`,
3. pixel `pixel_size`,
4. segment and path `stroke_width`,
5. axis widths, tick lengths, gaps, reserves, and text sizes,
6. colorbar and legend dimensions, offsets, and text sizes,
7. text and annotation sizes, anchors, padding, and bitmap/SDF layout.

Acceptance criteria:

1. Retina and standard-display fixtures produce the same logical layout with doubled physical
   framebuffer dimensions.
2. A user scale change marks only screen-space derived resources dirty.
3. Data-space sphere radius and mesh geometry do not change when `user_scale` changes.


### 4. Public API Surface

Expose scale state without making users manage DPI manually:

1. read-only query for current device scale on a view or figure,
2. read/write user scale on a view or scene,
3. optional callbacks or diagnostics for device-scale changes,
4. hosted-backend resize helper that accepts logical size, framebuffer size, and device scale.

Suggested API shape:

```c
float dvz_view_device_scale(const DvzView* view);
float dvz_view_user_scale(const DvzView* view);
void dvz_view_set_user_scale(DvzView* view, float scale);
```

Scene-level defaults can be added later if multiple views should inherit one user preference.


### 5. Validation Matrix

Add focused tests before broad visual polish:

| Test | Expected result |
|---|---|
| GLFW/window resize fixture with `400 x 300` logical and `800 x 600` framebuffer | window surface extent is physical, resize event retains both sizes |
| app figure sync on high-DPI resize | figure size is `400 x 300`, frame target is `800 x 600` |
| point/marker size lowering at `device_scale = 2` | emitted physical size is doubled |
| `user_scale = 1.5` on normal display | UI-like sizes multiply by `1.5`, data/world sizes unchanged |
| `device_scale` change from `1` to `2` | screen-space resources/text atlas are dirtied and redraw requested |
| multi-panel high-DPI scissor | physical scissor bounds match physical framebuffer panels |


## Open Decisions

| Decision | Recommendation |
|---|---|
| Where should `user_scale` live first? | Start on `DvzView`; add scene default inheritance later. This avoids cross-window surprises when one scene drives several displays. |
| Should `device_scale_x` and `device_scale_y` both be supported? | Store both, but resolve most style quantities with the average or X scale unless a visual needs anisotropic handling. Assert/log when they differ significantly. |
| Should `dvz_figure_resize()` accept logical or physical pixels? | Keep it logical, matching current headers and high-DPI spec. Add separate render-target/output scale state instead of overloading figure size. |
| Should DRP2 viewport/scissor stay normalized for portability? | No. Keep the DRP2 spec framebuffer-coordinate contract and convert normalized scene panel descriptors during emission. |
| Should `user_scale` affect point/marker data attributes? | Yes for attributes documented as screen/logical pixels. No for attributes documented as data/world units, including sphere radius. |
| Should text atlas rasterization include `user_scale`? | Include it indirectly through the resolved logical text size. The atlas key should include the effective physical raster size. |
