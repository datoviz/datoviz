# Profile Rendering Performance

Measure the cost of scene updates and rendering.

## Task Workflow

Use the smallest scene that reproduces the performance issue. Separate CPU data generation, visual
attribute upload, command submission, and GPU rendering when measuring.

## Minimal Call Sequence

```c
/* Run a bounded frame count for repeatable native profiling. */
dvz_app_run(app, frame_count);
```

For rendering-only checks, prefer offscreen or fixed-frame native examples.

## Canonical Examples

- Gallery: [Visual Data Update](../examples/gallery/features/feature_update_visual_data.md)
- Source: `examples/c/features/update_visual_data.c`
- Gallery: [Compute Buffer Animation](../examples/gallery/features/feature_compute_buffer_animation.md)
- Source: `examples/c/features/compute_buffer_animation.c`
- Gallery: [Point Cloud](../examples/gallery/showcases/point_cloud.md)
- Source: `examples/c/showcases/point_cloud.c`

## Important Details

Datoviz performance depends on upload volume, primitive count, draw count, framebuffer size, and
backend. Measure one variable at a time.

## Common Mistakes

- Timing random data generation and calling it rendering cost.
- Recreating visuals in the hot path.
- Comparing native and browser paths without matching resolution and feature set.

## See Also

- [Update visual data](update-visual-data.md)
- [Render offscreen and capture](render-offscreen.md)
- [Diagnose build and platform issues](diagnose-platform.md)
