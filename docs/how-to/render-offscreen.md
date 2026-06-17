# Render Offscreen and Capture

Render without opening a window, then save a PNG.

## Task Workflow

Use the normal scene, figure, panel, and visual setup. At the view step, create an offscreen view
instead of a GLFW view, render one frame into its framebuffer, then capture the last rendered frame.

Use this path for exact-size native screenshots, CI smoke tests, documentation images, and batch
renders where a visible window would be fragile or unnecessary.

## Minimal Call Sequence

```c
DvzApp* app = dvz_app(scene);
DvzView* view = dvz_view_offscreen(app, figure, width, height);

uint32_t framebuffer_width = 0;
uint32_t framebuffer_height = 0;
dvz_view_framebuffer_size(view, &framebuffer_width, &framebuffer_height);
if (framebuffer_width != width || framebuffer_height != height)
    return -1;

if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
    return -1;

if (dvz_view_capture_png(view, "output.png") != 0)
    return -1;

dvz_app_destroy(app);
```

Create the scene, figure, panel, and visuals before `dvz_view_offscreen()`. Capture only after a
submitted frame; `dvz_view_capture_png()` writes the previous rendered framebuffer.

## Static Captures

For static scenes, one call to `dvz_view_render_once()` is enough. This is the path used by
`examples/c/features/offscreen_capture.c`: build the retained scene, create an offscreen view,
verify the framebuffer dimensions, render once, and write the PNG.

```sh
just example-c features/offscreen_capture
./build/examples/c/features/offscreen_capture
```

The example writes `offscreen_capture.png` next to the executable and reports the exact pixel size.

## Multi-Frame Captures

For animated or incremental captures, update retained scene data before each render and capture
after each successful frame.

```c
for (uint32_t frame = 0; frame < frame_count; frame++)
{
    update_scene_for_frame(scene, frame);

    if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        break;

    char path[256] = {0};
    dvz_snprintf(path, sizeof(path), "frames/frame_%04u.png", frame);
    dvz_view_capture_png(view, path);
}
```

For long sequences, prefer the video export path instead of writing and assembling many PNGs by
hand.


## Important Details

Offscreen rendering is native-only in the current example set. It is the preferred path for CI,
image comparison tests, batch rendering, and documentation screenshots.

`dvz_view_offscreen(app, figure, width, height)` uses framebuffer pixels. If the output becomes a
test artifact, keep dimensions, data, camera/controller state, random seeds, and color-scale ranges
deterministic.

PNG capture is an sRGB RGBA8 screenshot/export path. It is not a scientific linear-float readback;
use explicit data/readback paths when the output is numeric evidence rather than a visual snapshot.

An offscreen view still needs a native GPU/runtime context. It avoids opening a user-facing window,
but it can still fail on machines without a usable Vulkan backend or required device capabilities.

## Common Mistakes

- Capturing before running a frame.
- Requesting a very large framebuffer without checking GPU limits.
- Assuming offscreen rendering means CPU-only rendering.
- Letting documentation or test screenshots depend on wall-clock animation state.
- Using offscreen capture as a substitute for WebGPU browser screenshots; browser examples use a
  separate route.

## See Also

- [Save screenshots](capture-an-image.md)
- [Export videos](video-export.md)
- [Debug rendering output](debug-rendering.md)

??? example "Related examples"

    - [Offscreen Capture](../examples/gallery/features/feature_offscreen_capture.md) - Source: `examples/c/features/offscreen_capture.c`
    - [Basic Scene](../examples/gallery/features/feature_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
