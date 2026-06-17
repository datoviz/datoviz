# Scene to Runtime Boundary

Datoviz keeps scene semantics above backend execution. The scene describes what should be rendered;
the runtime executes already-planned work.

The short version is:

```text
scene semantics -> frame artifact -> DRP2 packets -> runtime execution
```

## What the Scene Owns

The scene owns user-facing state: figures, panels, visuals, controllers, sampled fields, scales,
adornments, diagnostics, and frame planning. It validates that state, adapts it to declared runtime
capabilities, and emits frame artifacts.

Scene code should talk in terms of semantic objects and logical resources. It should not depend on
Vulkan handles, swapchain internals, command-buffer recording, queue ownership, or browser WebGPU
objects.

## What DRP2 Carries

DRP2 is the command-stream boundary between planning and execution. A frame artifact lowers scene
state into setup, update, and frame packets: resource creation, uploads, pipeline state, render or
compute passes, draw and dispatch commands, barriers, copies, and readback-shaped requests.

DRP2 should be deterministic enough for validation, replay, WebGPU fixture pressure, and native
runtime execution. It is not a second scene model.

## What the Runtime Owns

The runtime owns backend resources and execution. Native paths map DRP2-shaped work onto
`vklite`, `canvas`, `stream`, and optional `app` presentation. Browser paths consume the supported
DRP2 packet subset through the experimental WebGPU runtime.

The runtime may cache backend objects, reuse allocations, schedule submissions, and report
execution diagnostics. It should not infer new scene semantics from cached state or hidden backend
shortcuts.

## Why the Boundary Matters

This boundary keeps v0.4 extensible. New visual semantics belong in the scene contract. New
execution capabilities belong in DRP2 and the runtime. Native embedding, offscreen capture, replay,
and WebGPU portability all depend on the same split.

When debugging, ask which side failed: scene validation and frame planning, DRP2 emission, runtime
resource execution, or presentation/readback delivery.

See also:

- [Architecture](architecture.md)
- [Frame lifecycle](frame-lifecycle.md)
- [GPU resource ownership](gpu-resource-ownership.md)
