# vklite

`vklite` is Datoviz's native Vulkan execution layer. It wraps the Vulkan objects and operations
needed by the v0.4 runtime without becoming a second scene system.

Use this page when you are working below DRP2, canvas, or app code. Most application users should
start with scene, visual, app, and how-to pages instead.

## Role in the Stack

The active native path is:

```text
scene frame plan -> DRP2 command stream -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```

The scene layer decides what should be drawn. DRP2 carries that decision as backend-neutral
commands. `vklite` maps those commands to Vulkan resources, synchronization, command buffers,
render passes, compute passes, draws, copies, and readback-shaped work.

## What vklite Owns

`vklite` owns or coordinates native GPU objects:

- buffers, images, samplers, descriptors, shaders, graphics pipelines, and compute pipelines;
- command recording and submission helpers;
- Vulkan synchronization objects and resource barriers;
- swapchain and surface helpers used by higher runtime layers;
- rendering utilities used by offscreen and presentation paths.

The public umbrella header is `include/datoviz/vklite.h`, which collects the focused headers under
`include/datoviz/vklite/`.

## What vklite Does Not Own

`vklite` should not own scene semantics, visual-family rules, panel layout, controller behavior,
selection policy, or user-facing query semantics. Those belong above the runtime boundary.

It should also not take ownership of borrowed platform or interop handles unless the API contract
explicitly grants that authority. Swapchain images, host surfaces, external memory, and borrowed
command resources need clear lifetime rules.

## Working on vklite

Prefer changes that strengthen the DRP2/runtime contract rather than adding scene-shaped shortcuts.
If a feature needs new execution behavior, add the smallest Vulkan mapping that matches the
existing DRP2 command model, then cover lifetime, synchronization, and failure diagnostics.

For graphics ownership changes, make begin/end/reset/submit/transition authority explicit. A
borrowed handle should not be destroyed, reset, submitted, or transitioned through an implicit
shortcut.

## Validation

Use focused tests while iterating:

```sh
just test vklite
just test drp2
git diff --check
```

Add Vulkan validation or a bounded offscreen/GLFW smoke when the change touches command buffers,
render targets, swapchains, queues, synchronization, or resource lifetimes.

See also:

- [Scene to runtime boundary](../explanation/scene-to-runtime-boundary.md)
- [GPU resource ownership](../explanation/gpu-resource-ownership.md)
- [DRP2](../reference/drp2/index.md)
- [Canvas and stream API](canvas.md)
