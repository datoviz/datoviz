# Save Screenshots

Capture a rendered figure to an image file.

Use screenshot capture for documentation images, smoke-test artifacts, and user-facing raster
exports. It captures the rendered framebuffer, not a scientific linear-float data buffer.

!!! info "At a glance"

    - **Status:** Supported native app/offscreen capture.
    - **Languages:** Python and C.
    - **Prerequisites:** A valid view with at least one successfully rendered frame and a writable output path.
    - **Result:** A top-row-first sRGB RGBA8 frame written as PNG or available through the Python array facade.

## Task workflow

Use offscreen rendering for deterministic screenshots. For interactive workflows, render at least
one frame before capture so the framebuffer contains the current scene.

If you only need to render without opening a window, start with [Render offscreen](render-offscreen.md).
This page focuses on writing the rendered frame to an image file.

## Core capture fragment

This fragment assumes the scene, figure, panel, and visuals already exist. It renders one offscreen
frame, writes a PNG, and keeps one cleanup path for success and failure.

The block is a C function-body excerpt. For Python array capture and complete `try/finally` cleanup,
use [Render offscreen](render-offscreen.md#core-offscreen-fragment).

```c
DvzApp* app = dvz_app(scene);
int rc = -1;
if (app == NULL)
    return rc;
DvzView* view = dvz_view_offscreen(app, figure, width, height);
if (view == NULL)
    goto cleanup;

if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
    goto cleanup;
if (dvz_view_capture_png(view, "output.png") != 0)
    goto cleanup;

rc = 0;

cleanup:
dvz_app_destroy(app);
return rc;
```

For one-shot offscreen capture, prefer `dvz_view_render_once()` over an open-ended app loop. The
canonical example also verifies that the offscreen framebuffer size matches the requested output
size before writing the PNG.


## Capture size

Offscreen capture is the deterministic path: request the exact framebuffer size you want in the
PNG, then render and capture that view.

```c
uint32_t framebuffer_width = 0;
uint32_t framebuffer_height = 0;
dvz_view_framebuffer_size(view, &framebuffer_width, &framebuffer_height);
```

For native GLFW views, remember that the framebuffer size can differ from the logical window size
on high-DPI displays. Use offscreen views for exact-pixel tests and generated documentation assets.


## Capture modes

Use `dvz_view_capture_png()` when you want to save the last rendered frame immediately.

Use `dvz_view_capture_start()` and `dvz_view_capture_stop()` when the same run may also produce
DVZR recordings or video output, or when capture is controlled from `DvzAppCaptureConfig`:

This function-body excerpt assumes a valid `view` and returns from its enclosing function on error:

```c
DvzAppCaptureConfig capture = dvz_app_capture_config();
capture.flags = DVZ_APP_CAPTURE_PNG;
capture.directory = "captures";
capture.basename = "frame";

if (dvz_view_capture_start(view, &capture) != 0)
    return -1;
if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
{
    (void)dvz_view_capture_stop(view);
    return -1;
}
if (dvz_view_capture_stop(view) != 0)
    return -1;
```

`dvz_view_capture_stop()` writes the PNG from the last rendered frame for this capture mode.


## Important details

Use fixed dimensions and deterministic data when screenshots become tests or documentation assets.

PNG capture writes sRGB RGBA8 screenshot pixels. Do not use this path as a linear scientific image
readback or as a replacement for sampled-field data export.

Always check capture return codes in examples and tests. A missing display, unavailable GPU, invalid
output path, or failed readback should fail loudly.

The view owns the captured framebuffer; `dvz_view_capture_png()` does not transfer view ownership.
Finish capture before destroying the app. Python `dvz_view_capture_rgba()` returns a caller-owned
NumPy array copy that remains usable after app teardown.

## Common mistakes

- Capturing before a frame is rendered.
- Saving relative to an unexpected working directory.
- Treating generated screenshots as hand-written source assets.
- Assuming GLFW window dimensions are the same as framebuffer pixels on high-DPI displays.
- Using PNG screenshots when the application needs linear floating-point data.
- Ignoring a failed offscreen view or capture call.

## See also

- [Render offscreen](render-offscreen.md)
- [Export videos](video-export.md)
- [Debug rendering output](debug-rendering.md)

??? example "Complete and related examples"

    - Canonical complete example: [Offscreen Capture](../examples/gallery/runtime/runtime_offscreen_capture.md) - Source: `examples/c/runtime/offscreen_capture.c`
    - [Basic Scene](../examples/gallery/features/features_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
