# Profile Rendering Performance

Measure how much time Datoviz spends updating and drawing a scene.


## Task Workflow

Use the smallest scene that reproduces the performance issue. Build the scene once, keep the output
size fixed, warm up a few frames, then time a fixed number of frames.

Separate data generation, data upload, drawing, and screenshot or query work when measuring. Change
one variable at a time.

First check batching. Datoviz is fastest when a scene has a small number of visuals and many items
inside each visual. For example, use one point visual with many points instead of one visual per
point.

Before collecting numbers, record:

- platform and graphics path;
- output width and height;
- frame count and warm-up frame count;
- visual count and item count per visual;
- uploaded bytes per frame;
- whether screenshots, probes, queries, or buffer readback are enabled.


## Minimal Call Sequence

```c
const uint32_t warmup_frames = 8;
const uint32_t timed_frames = 240;

for (uint32_t frame = 0; frame < warmup_frames; frame++)
{
    if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        return -1;
}

const uint64_t t0 = dvz_time_monotonic_ns();
for (uint32_t frame = 0; frame < timed_frames; frame++)
{
    update_scene_for_frame(scene, frame);

    if (dvz_view_render_once(view) != DVZ_CANVAS_FRAME_READY)
        return -1;
}
const uint64_t t1 = dvz_time_monotonic_ns();

const double ms_per_frame = (double)(t1 - t0) / (double)timed_frames / 1e6;
const double fps = 1000.0 / ms_per_frame;
```

Create the scene, figure, panel, visuals, view, controllers, and callbacks before the warm-up loop.
Keep random data generation outside the timed loop unless data generation is the thing being
measured.

For rendering-only checks, prefer offscreen or fixed-frame native examples. Avoid timing an
unbounded interactive event loop unless the interaction path is the performance issue.


## Important Details

Datoviz performance depends on how much data changes, how many items are drawn, how many visuals the
scene has, output size, and whether screenshots or queries are enabled. Measure one variable at a
time.

When optimizing, prefer increasing item count inside existing visuals over increasing visual count.
Split visuals only for different visual types, style paths, panels, transforms, lifetimes, or update
rates.

Update existing visuals instead of recreating them while the scene is running. Updating existing
positions, colors, sizes, image pixels, or mesh data is usually cheaper than deleting and rebuilding
the visual.

Screenshots, pixel probes, queries, and downloads can force the program to wait for drawing to
finish. Disable them unless those operations are the target of the measurement.

Browser WebGPU is useful for browser testing, but it is not the desktop performance baseline.
Compare desktop and browser results only after matching output size, feature set, item count, and
screenshot or query behavior.


## What To Measure

| Symptom | Likely cause | First check |
| --- | --- | --- |
| Slow first frame. | Resource creation or initial upload. | Compare the first frame with steady-state frames after warm-up. |
| Slow every frame. | Draw count, uploads, callbacks, or output-size cost. | Disable updates and callbacks, then reduce output size. |
| Slow only during animation. | Per-frame data upload or CPU callback work. | Time the update callback separately from `dvz_view_render_once()`. |
| Slow only with screenshots or probes. | Waiting for drawing results. | Profile again with capture, query, and readback disabled. |
| Desktop is fast but browser is slow. | Browser subset, async behavior, or readback overhead. | Match resolution, features, and readback before comparing. |
| Many tiny objects are slow. | Too many separate visuals. | Batch items into fewer visuals. |


## Batching Pattern

Prefer one visual with dense arrays:

```c
DvzVisual* points = dvz_point(scene, 0);
dvz_visual_set_data(points, "position", positions, point_count);
dvz_visual_set_data(points, "color", colors, point_count);
dvz_visual_set_data(points, "diameter_px", diameters, point_count);
```

Avoid creating one visual per item:

```c
for (uint32_t i = 0; i < point_count; i++)
{
    DvzVisual* point = dvz_point(scene, 0);
    dvz_visual_set_data(point, "position", &positions[i], 1);
}
```

Split visuals only for real differences: a different visual type, style, panel, transform, lifetime,
visibility policy, or update rate.


## Common Mistakes

- Timing random data generation and calling it rendering cost.
- Recreating visuals in the hot path.
- Benchmarking many one-item visuals instead of one visual with many items.
- Changing item counts every frame when a fixed-size attribute update would work.
- Capturing screenshots or querying pixels inside a render benchmark.
- Comparing desktop and browser paths without matching resolution and feature set.


## Report Template

```text
Graphics path:
Platform/GPU:
Output size:
Frame count:
Warm-up frame count:
Visual count:
Item count per visual:
Uploaded bytes per frame:
Capture/query/readback enabled:
Average ms/frame:
FPS:
Reproducer:
```


## See Also

- [Update visual data](update-visual-data.md)
- [Render offscreen](render-offscreen.md)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Related examples"

    - [Visual Data Update](../examples/gallery/features/feature_update_visual_data.md) - Source: `examples/c/features/update_visual_data.c`
    - [Compute Buffer Animation](../examples/gallery/features/feature_compute_buffer_animation.md) - Source: `examples/c/features/compute_buffer_animation.c`
    - [Point Cloud](../examples/gallery/showcases/point_cloud.md) - Source: `examples/c/showcases/point_cloud.c`
