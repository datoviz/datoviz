# Performance Model

Datoviz performance is built around batching, retained resources, and predictable frame work.

## Batch by Visual

The preferred scene shape is a small number of retained visuals, each containing many homogeneous
items. A point cloud should usually be one point visual with a large `position` array, not thousands
of point visuals. A trajectory set should usually be one path or segment visual with many items, not
one visual per curve or segment.

This matters because each visual carries CPU-side retained state, validation, command generation,
resource tracking, and draw setup. The GPU is efficient when it receives large batches of similar
work. It is much less efficient when a scene is fragmented into many tiny visuals with one or a few
items each.

Split visuals only when there is a real rendering or lifecycle boundary:

- a different visual family;
- a different material, shader, technique, or render state;
- a different panel attachment or transform;
- a different lifetime or visibility policy;
- a substantially different update cadence.

Style differences that can be expressed as attributes should normally remain inside one visual. For
example, point color, size, item state, and position should be arrays on one point visual whenever
possible.

## Prefer Retained Updates

Retained updates are the second major cost boundary. Updating the contents of an existing attribute
or texture is usually cheaper than changing its shape. Recreating visuals every frame defeats the
retained model: it forces validation, resource planning, uploads, and draw setup to repeat when a
bounded update would have been enough.

## Main Cost Factors

The main performance factors are:

- visual count and draw count;
- item count per visual;
- uploaded bytes per frame;
- resource shape churn, such as changing item counts or texture dimensions;
- framebuffer size, sample count, depth, blending, and capture/readback cost;
- controller and callback work on the CPU;
- backend path, especially native Vulkan versus experimental browser WebGPU.

## Readback and Capture

Readback and capture should be treated as synchronization-heavy work. A screenshot, pixel probe, or
buffer readback may force the runtime to make GPU results visible to the CPU or browser. Keep those
requests explicit and bounded, especially in interaction loops.

## Backend Baselines

Browser WebGPU adds portability overhead and asynchronous behavior. The public WebGPU subset is
valuable for validation and browser demos, but it is not the performance baseline for native
Datoviz. Native Vulkan proof and browser proof should be profiled separately.

## Profiling Approach

When profiling, separate CPU data generation, attribute upload volume, visual count, draw count,
framebuffer size, readback, and backend. If performance is poor, inspect visual count and resource
churn before tuning lower layers.

See also:

- [Retained resources](retained-resources.md)
- [Invalidation and caching](invalidation-and-caching.md)
- [Profile rendering performance](../how-to/profile-performance.md)
