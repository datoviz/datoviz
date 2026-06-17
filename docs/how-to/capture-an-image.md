# Save Screenshots

Capture a rendered figure to an image file.

## Task Workflow

Use offscreen rendering for deterministic screenshots. For interactive workflows, render at least
one frame before capture so the framebuffer contains the current scene.

## Minimal Call Sequence

```c
DvzView* view = dvz_view_offscreen(app, figure, width, height);
dvz_app_run(app, 1);
dvz_view_capture_png(view, "output.png");
```

## Canonical Examples

- Gallery: [Offscreen Capture](../examples/gallery/features/feature_offscreen_capture.md)
- Source: `examples/c/features/offscreen_capture.c`
- Gallery: [Basic Scene](../examples/gallery/features/feature_basic_scene.md)
- Source: `examples/c/features/basic_scene.c`

## Important Details

Use fixed dimensions and deterministic data when screenshots become tests or documentation assets.

## Common Mistakes

- Capturing before a frame is rendered.
- Saving relative to an unexpected working directory.
- Treating generated screenshots as source files unless the docs pipeline expects them.

## See Also

- [Render offscreen and capture](render-offscreen.md)
- [Export videos](video-export.md)
- [Debug rendering output](debug-rendering.md)
