# Render offscreen

Render a scene without opening a visible window.

!!! info "At a glance"

    **Status:** Supported native rendering path · **Languages:** Python NumPy facade and C
    **Prerequisites:** A prepared scene and figure plus a usable native GPU runtime
    **Result:** An exact-size RGBA8 framebuffer, optionally captured as PNG or NumPy data

## Task workflow

Use the normal scene, figure, panel, and visual setup. At the view step, create an offscreen view
instead of a visible window view, then render one frame or a bounded sequence of frames.

Use this path for exact-size native rendering, automated checks, documentation images, and batch
renders where a visible window would be fragile or unnecessary. To save the rendered frame as a PNG,
see [Save screenshots](screenshots.md).

![Four colored points rendered to an offscreen framebuffer](../assets/gallery/v0.4/runtime/runtime_offscreen_capture.webp)

## Core offscreen fragment

This fragment assumes the scene, figure, panel, and visuals already exist. It shows the offscreen
view step and uses a cleanup path so failed size checks or render calls still destroy the app.

=== "Python"

    ```python
    import datoviz as dvz

    app = dvz.dvz_app(scene)
    if not app:
        raise RuntimeError("dvz_app() failed")
    try:
        view = dvz.dvz_view_offscreen(app, figure, width, height)
        if not view:
            raise RuntimeError("dvz_view_offscreen() failed")
        if dvz.dvz_view_render_once(view) != dvz.DVZ_CANVAS_FRAME_READY:
            raise RuntimeError("dvz_view_render_once() failed")

        rgba = dvz.dvz_view_capture_rgba(view)
        assert rgba.shape == (height, width, 4)
        assert rgba.dtype.name == "uint8"
    finally:
        dvz.dvz_app_destroy(app)
    ```

=== "C"

    ```c
    DvzApp* app = dvz_app(scene);
    int rc = -1;
    if (app == NULL)
        return rc;
    DvzView* view = dvz_view_offscreen(app, figure, width, height);
    if (view == NULL)
        goto cleanup;

    uint32_t framebuffer_width = 0;
    uint32_t framebuffer_height = 0;
    dvz_view_framebuffer_size(view, &framebuffer_width, &framebuffer_height);
    if (framebuffer_width != width || framebuffer_height != height)
        goto cleanup;

    if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        goto cleanup;

    rc = 0;

    cleanup:
    dvz_app_destroy(app);
    return rc;
    ```

Create the scene, figure, panel, and visuals before `dvz_view_offscreen()`. Render at least one
frame before reading pixels or saving a screenshot from the view.

The Python array returned above is top-row-first RGBA8 data. The C excerpt only proves rendering;
use `dvz_view_capture_png()` or the canvas capture functions to consume its last frame.

## Static offscreen frames

For static scenes, one call to `dvz_view_render_once()` is enough. This is the path used by
`examples/c/runtime/offscreen_capture.c`: build the retained scene, create an offscreen view,
verify the framebuffer dimensions, render once, and write the PNG.

Prerequisite: build the source checkout from the repository root. A successful run writes the
reported exact-size PNG; use the source below as the complete, checked example.

```sh
just example-c runtime/offscreen_capture
./build/examples/c/runtime/offscreen_capture
```

The example writes `offscreen_capture.png` next to the executable and reports the exact pixel size.

## Multi-frame rendering

For animated or incremental output, update retained scene data before each render. Save screenshots
or video only after each successful frame. This function-body excerpt assumes that the translation
unit includes `<stdio.h>` for standard `snprintf()`.

```c
for (uint32_t frame = 0; frame < frame_count; frame++)
{
    update_scene_for_frame(scene, frame);

    if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        break;

    char path[256] = {0};
    int written = snprintf(path, sizeof(path), "frames/frame_%04u.png", frame);
    if (written < 0 || (size_t)written >= sizeof(path))
        break;
    if (dvz_view_capture_png(view, path) != 0)
        break;
}
```

For long sequences, prefer the video export path instead of writing and assembling many PNGs by
hand.


## Important details

Offscreen rendering is a supported native feature. In the generated WebGPU example matrix its
examples are classified as `native-only` because this view API is not a browser route; that label
describes browser portability, not the native API's release status. Offscreen rendering is the
preferred path for CI, image comparison tests, batch rendering, and documentation screenshots.

`dvz_view_offscreen(app, figure, width, height)` uses framebuffer pixels. Python
`dvz_view_capture_rgba(view)` returns an array shaped `(height, width, 4)` with top-row-first RGBA8
screenshot pixels. If the output becomes a test artifact, keep dimensions, data, camera/controller
state, random seeds, and color-scale ranges deterministic.

PNG capture is an sRGB RGBA8 screenshot/export path. It is not a scientific linear-float readback;
use explicit data/readback paths when the output is numeric evidence rather than a visual snapshot.

An offscreen view still needs a usable native graphics runtime. It avoids opening a user-facing
window, but it can still fail on machines without the required GPU/device capabilities.

## Common mistakes

- Reading pixels or capturing before running a frame.
- Requesting a very large framebuffer without checking GPU limits.
- Assuming offscreen rendering means CPU-only rendering.
- Letting documentation or test screenshots depend on wall-clock animation state.
- Using offscreen rendering as a substitute for WebGPU browser screenshots; browser examples use a
  separate route.

## See also

- [Save screenshots](screenshots.md)
- [Export videos](video-export.md)
- [Debug rendering output](debug-rendering.md)

??? example "Complete and related examples"

    - Canonical complete example: [Offscreen Capture](../examples/gallery/runtime/runtime_offscreen_capture.md) - Source: `examples/c/runtime/offscreen_capture.c`
    - [Basic Scene](../examples/gallery/features/features_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
