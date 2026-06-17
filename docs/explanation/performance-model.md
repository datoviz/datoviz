# Performance Model

Datoviz performance is built around batching.

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

When profiling, separate CPU data generation, attribute upload volume, visual count, draw count,
framebuffer size, and backend. If performance is poor, inspect visual count before tuning lower
layers.
