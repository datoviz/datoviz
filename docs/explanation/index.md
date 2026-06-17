# Explanation

These pages explain the Datoviz v0.4 mental model: what the engine owns, how retained scene state
turns into GPU work, where runtime boundaries sit, and which parts are deliberately outside the
v0.4 release surface.

Use explanation pages when you need design context. Use how-to pages for task recipes and reference
pages for exact API names, status labels, and constraints.

## Reading Order

Start with the positioning pages:

1. [Why Datoviz?](why-datoviz.md)
2. [Datoviz, GSP, and VisPy2](gsp-vispy2-boundary.md)

Then read the system model:

1. [Architecture](architecture.md)
2. [Scene to runtime boundary](scene-to-runtime-boundary.md)
3. [Portability and WebGPU](portability-webgpu.md)

Then read the scene concepts:

1. [Scene model](scene-model.md)
2. [Figures, panels, and visuals](figure-panel-visual-model.md)
3. [Coordinate systems](coordinate-systems.md)
4. [Interaction model](interaction-model.md)
5. [Queries, picking, and probing](query-pick-probe-model.md)

Finally read the frame and resource model:

1. [Frame lifecycle](frame-lifecycle.md)
2. [Retained resources](retained-resources.md)
3. [Invalidation and caching](invalidation-and-caching.md)
4. [GPU resource ownership](gpu-resource-ownership.md)
5. [Performance model](performance-model.md)
