# Profile Rendering Performance

Measure the cost of scene updates and rendering.

## Task Workflow

Use the smallest scene that reproduces the performance issue. Build the scene once, keep the
framebuffer size fixed, warm up a few frames, then time a fixed number of frames.

Separate CPU data generation, visual attribute upload, command submission, GPU rendering, and
readback or capture when measuring. Change one variable at a time.

First check batching. Datoviz is fastest when a scene has a small number of visuals and a large
number of items per visual. Excessive visual count increases CPU bookkeeping, command generation,
state changes, and draw overhead before the GPU can help.

Before collecting numbers, record:

- backend and platform;
- framebuffer width and height;
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

Datoviz performance depends on upload volume, primitive count, draw count, framebuffer size, and
backend. Measure one variable at a time.

When optimizing, prefer increasing item count inside existing visuals over increasing visual count.
Split visuals only for different visual families, material or technique paths, panels, transforms,
lifetimes, or update rates.

Use retained updates instead of recreating visuals in the hot path. Updating an existing attribute is
usually cheaper than changing item count, texture dimensions, visual family, material path, or other
resource shape.

Readback and capture are synchronization-heavy. Disable screenshots, pixel probes, queries, and
buffer downloads unless those operations are the target of the measurement.

Browser WebGPU is useful for browser validation and diagnostics, but it is not the native
performance baseline. Compare native and browser results only after matching framebuffer size,
feature set, item count, and readback behavior.

## What To Measure

| Symptom | Likely cause | First check |
| --- | --- | --- |
| Slow first frame. | Resource creation or initial upload. | Compare the first frame with steady-state frames after warm-up. |
| Slow every frame. | Draw count, uploads, callbacks, or framebuffer cost. | Disable updates and callbacks, then reduce framebuffer size. |
| Slow only during animation. | Per-frame data upload or CPU callback work. | Time the update callback separately from `dvz_view_render_once()`. |
| Slow only with screenshots or probes. | GPU-to-CPU synchronization. | Profile again with capture, query, and readback disabled. |
| Native is fast but browser is slow. | WebGPU subset, async behavior, or readback overhead. | Match resolution, features, and readback before comparing. |
| Many tiny objects are slow. | Visual fragmentation. | Batch items into fewer retained visuals. |

## Batching Pattern

Prefer one retained visual with dense arrays:

```c
DvzVisual* points = dvz_point(scene, 0);
dvz_visual_set_data(points, "position", positions, point_count);
dvz_visual_set_data(points, "color", colors, point_count);
dvz_visual_set_data(points, "diameter", diameters, point_count);
```

Avoid creating one visual per item:

```c
for (uint32_t i = 0; i < point_count; i++)
{
    DvzVisual* point = dvz_point(scene, 0);
    dvz_visual_set_data(point, "position", &positions[i], 1);
}
```

Split visuals only for real rendering or lifecycle boundaries: a different visual family, material
or technique path, panel, transform, lifetime, visibility policy, or update cadence.

## Common Mistakes

- Timing random data generation and calling it rendering cost.
- Recreating visuals in the hot path.
- Benchmarking many one-item visuals instead of a batched visual with many items.
- Changing item counts every frame when a fixed-size attribute update would work.
- Capturing screenshots or querying pixels inside a render benchmark.
- Comparing native and browser paths without matching resolution and feature set.

## Report Template

```text
Backend:
Platform/GPU:
Framebuffer size:
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
- [Render offscreen and capture](render-offscreen.md)
- [Performance model](../explanation/performance-model.md)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Related examples"

    - [Visual Data Update](../examples/gallery/features/feature_update_visual_data.md) - Source: `examples/c/features/update_visual_data.c`
    - [Compute Buffer Animation](../examples/gallery/features/feature_compute_buffer_animation.md) - Source: `examples/c/features/compute_buffer_animation.c`
    - [Point Cloud](../examples/gallery/showcases/point_cloud.md) - Source: `examples/c/showcases/point_cloud.c`
