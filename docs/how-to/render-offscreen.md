# Render Offscreen and Capture

Render without opening a window, then save a PNG.

## Task Workflow

Use the normal scene, figure, panel, and visual setup. At the view step, create an offscreen view
instead of a GLFW view, render one frame, and capture the framebuffer.

## Minimal Call Sequence

```c
DvzApp* app = dvz_app(scene);
DvzView* view = dvz_view_offscreen(app, figure, width, height);
dvz_app_run(app, 1);
dvz_view_capture_png(view, "output.png");
dvz_app_destroy(app);
```

Use one frame for static scenes. For animated captures, update the scene each frame before saving
successive images or use the video-export path.


## Important Details

Offscreen rendering is native-only in the current example set. It is the preferred path for CI,
image comparison tests, batch rendering, and documentation screenshots.

## Common Mistakes

- Capturing before running a frame.
- Requesting a very large framebuffer without checking GPU limits.
- Using offscreen capture as a substitute for WebGPU browser screenshots; browser examples use a
  separate route.

## See Also

- [Save screenshots](capture-an-image.md)
- [Export videos](video-export.md)
- [Debug rendering output](debug-rendering.md)

??? example "Related examples"

    - [Offscreen Capture](../examples/gallery/features/feature_offscreen_capture.md) - Source: `examples/c/features/offscreen_capture.c`
    - [Basic Scene](../examples/gallery/features/feature_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
