# Live HiDPI Window Implementation Plan

Status: implementation plan for v0.4 pre-RC.

This document records the Datoviz-side fix for live GLFW high-DPI sizing and the related text
anchoring regression found during GSP_API backend review. It extends the coordinate model in
[HIGH_DPI.md](HIGH_DPI.md) and supersedes the ambiguous live-window parts of
[DPI_SCALE_IMPLEMENTATION_PLAN.md](DPI_SCALE_IMPLEMENTATION_PLAN.md).


## Problem

On a high-DPI X11 desktop, a requested Datoviz live view of `900 x 650` reports:

| Metric | Observed |
|---|---:|
| logical size | `900 x 650` |
| framebuffer size | `900 x 650` |
| device scale | `~1.4549` |
| native X11 client | `900 x 650` |

Matplotlib/Qt with the same logical size creates a native client around `1313 x 1005`, matching the
display scale. Datoviz therefore detects the display scale but does not allocate or present a
scaled live surface.

The current GLFW path collapses the model:

1. `dvz_view()` resolves logical and framebuffer sizes.
2. GLFW creation receives only `resolved.logical_width` and `resolved.logical_height`.
3. `DvzWindowConfig.width` / `height` are passed directly to `glfwCreateWindow()`.
4. The backend reports a framebuffer equal to the native window size, so logical and physical sizes
   stay identical even when `device_scale > 1`.


## Contract

Datoviz v0.4 should use four distinct sizes:

| Name | Meaning |
|---|---|
| `logical_size` | Scene/application size in logical pixels. Figure and panel layout use this size. |
| `native_size` | Backend/window-system content size in backend coordinates. |
| `surface_size` | Presentable swapchain/framebuffer size in physical pixels. |
| `render_size` | Internal render-target size in physical pixels. This may exceed surface size when supersampling. |

Scale factors:

| Name | Meaning |
|---|---|
| `device_scale` | `surface_size / logical_size`, per axis. |
| `render_scale` | `render_size / surface_size`. This is supersampling, not layout scale. |
| `user_scale` | Accessibility/style scale. It never changes figure or panel layout size. |
| `native_to_logical` | Conversion from backend input coordinates to Datoviz logical coordinates. |

Required formulas:

```text
surface_size = round(logical_size * device_scale)
render_size  = round(surface_size * render_scale)
style_px     = logical_style_px * device_scale * render_scale * user_scale
```

`render_scale == 1` keeps `surface_size == render_size`. Offscreen views may treat
`surface_size` as the output image size unless a future API exposes a separate resolved output.


## API Changes

Break ambiguous pre-RC width/height APIs where necessary. Do not preserve compatibility if it keeps
the size unit unclear.

Add shared explicit types:

```c
typedef struct DvzExtent
{
    uint32_t width;
    uint32_t height;
} DvzExtent;

typedef struct DvzScaleXY
{
    float x;
    float y;
} DvzScaleXY;
```

`DvzScale` is already the retained scene scale object, so the window metrics type uses
`DvzScaleXY`.

Add size-space vocabulary:

```c
typedef enum DvzSizeSpace
{
    DVZ_SIZE_LOGICAL,
    DVZ_SIZE_NATIVE,
    DVZ_SIZE_SURFACE,
    DVZ_SIZE_RENDER,
} DvzSizeSpace;
```

Add high-DPI policies:

```c
typedef enum DvzHiDpiPolicy
{
    DVZ_HIDPI_AUTO = 0,
    DVZ_HIDPI_DISABLED,
    DVZ_HIDPI_FRAMEBUFFER,
    DVZ_HIDPI_NATIVE_WINDOW,
    DVZ_HIDPI_EXTERNAL,
} DvzHiDpiPolicy;
```

Add explicit window metrics:

```c
typedef struct DvzWindowMetrics
{
    DvzExtent logical_size;
    DvzExtent native_size;
    DvzExtent surface_size;
    DvzExtent render_size;

    DvzScaleXY content_scale;
    DvzScaleXY framebuffer_scale;
    DvzScaleXY device_scale;
    DvzScaleXY native_to_logical;

    DvzHiDpiPolicy active_hidpi_policy;
    uint64_t generation;
} DvzWindowMetrics;
```

Expose view queries by space:

```c
DvzExtent dvz_view_size(const DvzView* view, DvzSizeSpace space);
DvzScaleXY dvz_view_device_scale_xy(const DvzView* view);
float dvz_view_render_scale(const DvzView* view);
float dvz_view_user_scale(const DvzView* view);
```

Existing scalar helpers may remain only as compatibility conveniences if their documentation says
how non-uniform scale is reduced.


## GLFW Strategy

`dvz_view()` must pass the resolved descriptor to GLFW creation, not just logical width/height:

```text
_view_desc_resolve_logical()
_view_create_glfw(app, figure, &resolved_desc)
metrics = dvz_window_metrics(window)
_view_apply_metrics(view, &resolved_desc, &metrics)
```

Split descriptor resolution:

| Function | Responsibility |
|---|---|
| `_view_desc_resolve_logical()` | Resolve requested logical size and caller-provided scale preferences before a platform window exists. |
| `_view_desc_resolve_from_window_metrics()` | Resolve surface/render/device/native state after backend metrics are known. |

GLFW `DVZ_HIDPI_AUTO` chooses one active policy.

| Policy | Detection | Creation / resize behavior |
|---|---|---|
| `DVZ_HIDPI_FRAMEBUFFER` | `framebuffer_size / window_size ~= content_scale` | Use logical window size. Surface is the larger framebuffer. |
| `DVZ_HIDPI_NATIVE_WINDOW` | `framebuffer_size / window_size ~= 1` and content scale > 1 | Enlarge native window to `round(logical_size * device_scale)`. Preserve logical size separately. |
| `DVZ_HIDPI_DISABLED` | Scale disabled or scale resolves to 1 | Logical, native, and surface sizes match. |
| `DVZ_HIDPI_EXTERNAL` | Host supplies metrics | Reject incomplete or ambiguous metrics. Do not guess. |

For X11/Windows-style native-window scaling, prefer hidden creation:

```text
1. Create hidden window with initial size.
2. Query content scale, framebuffer size, and native window size.
3. Resolve active policy and device scale.
4. If native-window scaling is active, call glfwSetWindowSize(logical * device_scale).
5. Requery all metrics.
6. Show the window.
```

`GLFW_SCALE_TO_MONITOR` may be used, but Datoviz must still preserve requested logical size. If GLFW
returns an enlarged window size, Datoviz derives `logical_size = native_size / device_scale`, not
`logical_size = native_size`.

Every resize, framebuffer-size, and content-scale callback must requery full metrics. Do not trust a
single callback argument as the complete state.


## Input Strategy

Scene and controller APIs receive logical coordinates only.

Store in `DvzWindowMetrics`:

```text
native_to_logical.x = logical_size.width  / native_size.width
native_to_logical.y = logical_size.height / native_size.height
```

GLFW pointer conversion:

```text
logical_x = native_x * native_to_logical.x
logical_y = native_y * native_to_logical.y
```

Rules:

1. Mouse position and drag deltas convert through `native_to_logical`.
2. Scroll wheel offsets remain abstract scroll units unless Datoviz adds pixel-scroll deltas.
3. Resize events emit logical, native, surface, render, device scale, and reason.
4. Picking converts logical input to render-target coordinates using `render_size / logical_size`.
5. Logical input origin remains top-left with y down; GPU clip space remains y up.


## Render And Style Scaling

Use one internal transform for render lowering:

```c
typedef struct DvzViewTransform
{
    DvzExtent logical_size;
    DvzExtent surface_size;
    DvzExtent render_size;

    DvzScaleXY logical_to_surface;
    DvzScaleXY logical_to_render;
    DvzScaleXY style_to_render;
} DvzViewTransform;
```

Quantity rules:

| Quantity | Authored in | Scaling |
|---|---|---|
| Figure/view size | logical px | Device/render scale affect only physical targets. |
| Panel layout and explicit reserves | logical px | Layout is not user-scaled. Lower to physical viewport/scissor with `logical_to_render`. |
| Point size, line width, text size, text offset | logical px | Lower with `style_to_render = logical_to_render * user_scale`. |
| Auto reserves derived from text/ticks | logical px | Derive after style/user scaling so larger UI text does not clip. |
| Data/world/image sample coordinates | data/world/sample space | Never scaled by user scale. |

`render_scale` must not change OS window size. It changes internal render size only.


## Text Anchoring

Retained text anchors use layout-box coordinates:

| Anchor | Meaning |
|---|---|
| `(0, 0)` | top-left |
| `(0.5, 0.5)` | center |
| `(0.5, 1)` | bottom-center |
| `(1, 1)` | bottom-right |

For a text layout box with width `w`, height `h`, and anchor position `P`:

| Anchor | Expected box relative to `P` |
|---|---|
| `(0, 0)` | `min=P`, `max=P+(w,h)` |
| `(0.5, 0.5)` | `min=P-(w/2,h/2)`, `max=P+(w/2,h/2)` |
| `(0.5, 1)` | `min=P+(-w/2,-h)`, `max=P+(w/2,0)` |
| `(1, 1)` | `min=P-(w,h)`, `max=P` |

The current WGSL glyph shader applies y-down screen offsets as negative clip-space y. The GLSL
glyph shader currently differs. Add tests first, then make both shader paths use the same y-down
screen-space convention.


## Commit Plan

Use six checkpoint commits for the Datoviz implementation:

1. **Add size/scale metrics and diagnostics.**
   No behavior change. Include a probe that prints requested logical, native, surface, render,
   device scale, and active policy.
2. **Refactor view/window sizing around explicit spaces.**
   Split descriptor resolution and stop using ambiguous width/height at the app/window boundary.
3. **Implement GLFW high-DPI policies.**
   Add `AUTO`, `DISABLED`, `FRAMEBUFFER`, and `NATIVE_WINDOW`.
4. **Normalize resize and input coordinates.**
   Requery full metrics on callbacks and convert native pointer coordinates to logical coordinates.
5. **Audit render/style scaling.**
   Ensure viewport/scissor, style payloads, text, guides, and colorbars use the correct scale.
6. **Fix text anchoring and glyph y-axis consistency.**
   Add anchor-bounds tests and reconcile GLSL/WGSL.


## Test Plan

Pure CI tests:

1. Round-size helper with fractional scales and invalid inputs.
2. View-desc logical/surface/render resolution.
3. Synthetic window metrics normalization for framebuffer-scaling, native-window-scaling, and
   current broken X11 metrics.
4. Input coordinate conversion from native to logical.
5. Render/style scaling matrix for device, render, and user scale.
6. Text anchor bounds for bitmap and MSDF paths.
7. Shader convention test proving positive screen y maps to negative clip y.

Conditional live GLFW tests:

1. Skip unless `DVZ_TEST_LIVE_GLFW=1`.
2. Skip if no display, GLFW init fails, GPU creation fails, or the window manager clamps size.
3. With `DVZ_DISPLAY_SCALE=1.5`, request `300 x 200` logical:
   - logical size must remain `300 x 200`,
   - surface/native size must be about `450 x 300` in native-window mode,
   - surface size must be about `450 x 300` in framebuffer mode,
   - scale tolerance `±0.02`, physical size tolerance `±2 px`.

Manual acceptance probe:

```text
datoviz_hidpi_probe --logical 900x650 --render-scale 1 --user-scale 1
```

Expected on the observed X11 setup:

```text
logical_size:      900 x 650
device_scale:      ~1.4549
surface_size:      ~1309 x 946
native_size:       ~1309 x 946
input center:      ~450 x 325 logical
```


## Failure Modes

| Failure | Symptom | Detection |
|---|---|---|
| Double scaling | Physical size is `logical * scale * scale`. | Assert `surface/logical ~= device_scale`. |
| Logical overwritten by native | Figure becomes `1310 x 946` after create. | Synthetic X11 metrics and live logical-size assertion. |
| Render scale changes OS window | `render_scale=2` doubles the native window. | Compare native/surface at render scale 1 and 2. |
| Input not converted | Center click reports physical center. | Synthetic and live input tests. |
| Resize feedback loop | Size oscillates while moving monitors. | Metrics generation, idempotent comparison, callback coalescing. |
| User scale changes layout | Panels or explicit reserves move under user scale. | Explicit reserve tests. |
| Text paths use different bounds | Bitmap/MSDF anchors differ. | Anchor-bounds tests with the same string. |
| GLSL/WGSL y-axis divergence | Text appears above in one backend and below in another. | Shader-sign and GPU smoke tests. |
