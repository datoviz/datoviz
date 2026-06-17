# Profile Rendering Performance

Measure the cost of scene updates and rendering.

## Task Workflow

Use the smallest scene that reproduces the performance issue. Separate CPU data generation, visual
attribute upload, command submission, and GPU rendering when measuring.

First check batching. Datoviz is fastest when a scene has a small number of visuals and a large
number of items per visual. Excessive visual count increases CPU bookkeeping, command generation,
state changes, and draw overhead before the GPU can help.

## Minimal Call Sequence

```c
dvz_app_run(app, frame_count);
```

For rendering-only checks, prefer offscreen or fixed-frame native examples.


## Important Details

Datoviz performance depends on upload volume, primitive count, draw count, framebuffer size, and
backend. Measure one variable at a time.

When optimizing, prefer increasing item count inside existing visuals over increasing visual count.
Split visuals only for different visual families, material or technique paths, panels, transforms,
lifetimes, or update rates.

## Common Mistakes

- Timing random data generation and calling it rendering cost.
- Recreating visuals in the hot path.
- Benchmarking many one-item visuals instead of a batched visual with many items.
- Comparing native and browser paths without matching resolution and feature set.

## See Also

- [Update visual data](update-visual-data.md)
- [Render offscreen and capture](render-offscreen.md)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Related examples"

    - Gallery: [Visual Data Update](../examples/gallery/features/feature_update_visual_data.md)
    - Source: `examples/c/features/update_visual_data.c`
    - Gallery: [Compute Buffer Animation](../examples/gallery/features/feature_compute_buffer_animation.md)
    - Source: `examples/c/features/compute_buffer_animation.c`
    - Gallery: [Point Cloud](../examples/gallery/showcases/point_cloud.md)
    - Source: `examples/c/showcases/point_cloud.c`
