# Explanation

These pages explain the Datoviz v0.4 mental model: what you build, how scenes become images, where
runtime boundaries sit, and which parts are deliberately outside the v0.4 release surface.

Use explanation pages when you need design context. Use how-to pages for task recipes and reference
pages for exact API names, status labels, and constraints.

## Reading Order

Start with the positioning pages:

1. [What is Datoviz?](../start/what-is-datoviz.md)
2. [Choose your layer](../start/choose-your-layer.md)
3. [Datoviz, GSP, and VisPy2](gsp-vispy2-boundary.md)

Then read the system model:

1. [Architecture](architecture.md)
2. [Scene to runtime boundary](scene-to-runtime-boundary.md)
3. [WebGPU subset](../reference/webgpu-subset.md)

Then read the scene concepts:

1. [Scene building blocks](figure-panel-visual-model.md)
2. [Coordinate systems](coordinate-systems.md)
3. [Interaction model](interaction-model.md)
4. [Queries, picking, and probing](query-pick-probe-model.md)

Finally read the frame and resource model:

1. [Frame lifecycle](frame-lifecycle.md)
2. [Retained resources](retained-resources.md)
3. [Invalidation and caching](invalidation-and-caching.md)
4. [GPU resource ownership](gpu-resource-ownership.md)
5. [Profile rendering performance](../how-to/profile-performance.md)
