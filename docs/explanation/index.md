# Concepts and architecture

These pages explain the Datoviz v0.4 model behind the public API: which objects hold scientific
state, how interaction changes that state, how one frame is planned, and where GPU execution begins.
They are useful when a recipe works but you need to understand *why*, or when you are preparing to
change scene or runtime code.

For exact API names and support claims, use the [Reference](../reference/index.md). For a task
recipe, use the [How-To guides](../how-to/index.md). Lower runtime and protocol details are
**advanced/unstable** even when the scene feature using them is supported.


## Choose a reading path

| Goal | Read in this order | Prerequisite |
| --- | --- | --- |
| Understand ordinary scenes | [Scene building blocks](figure-panel-visual-model.md) → [Coordinate systems](coordinate-systems.md) → [Interaction model](interaction-model.md) | The [Quickstart](../start/quickstart.md) |
| Understand scene planning | [Architecture](architecture.md) → [Scene to runtime boundary](scene-to-runtime-boundary.md) → [Frame lifecycle](frame-lifecycle.md) | Familiarity with retained scenes and one rendered example |
| Work on incremental updates | [Retained resources](retained-resources.md) → [Invalidation and caching](invalidation-and-caching.md) → [GPU resource ownership](gpu-resource-ownership.md) | C, resource lifetimes, and the frame lifecycle |
| Work on queries | [Interaction model](interaction-model.md) → [Queries, picking, and probing](query-pick-probe-model.md) | Panels, coordinates, and asynchronous result handling |
| Choose the correct project layer | [Datoviz, GSP, and VisPy2](gsp-vispy2-boundary.md) | None |


## Layer map

```text
user data and intent
        |
        v
scene objects and retained state       supported user surface, by feature
        |
        v
FramePlan and frame artifact            scene-owned internal boundary
        |
        v
DRP2 packets                            advanced/unstable protocol
        |
        v
native Vulkan or browser WebGPU         backend execution
```

The durable design contracts live in `spec/scene/` and `spec/drp2/` in the repository. Installed
headers and generated reference pages remain authoritative for public names that already exist;
specs may also describe future pressure and must not be read as feature-status claims. Start with
the [Scene spec authority](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/scene/AUTHORITY.md)
before changing a cross-layer contract.
