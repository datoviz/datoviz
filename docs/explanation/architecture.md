# Architecture

The active Datoviz v0.4 path is:

```text
scene frame plans -> DRP2 command streams -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```

The scene layer owns user-facing visualization state: figures, panels, visuals, controllers,
adornments, sampled fields, scales, diagnostics, and frame planning. It should remain high-level
logic. It derives rendering work, but it does not own swapchains, windows, Vulkan queues, or command
buffer submission.

DRP2 is the runtime-facing command stream. It is the boundary between scene semantics and backend
execution. A scene frame plan lowers to setup, update, and frame packets that describe resources,
uploads, pipelines, render passes, compute passes, copies, draws, barriers, and readback-shaped
work. DRP2 exists so native execution, WebGPU fixture execution, replay, and validation can apply
pressure to the same contract.

The native runtime path uses `vklite`, `canvas`, and `stream` to turn DRP2-shaped work into Vulkan
resource ownership and frame execution. The `app` layer is intentionally small: it owns views,
offscreen targets, GLFW windows, presentation scheduling, and capture plumbing. It is not a second
scene system.

The browser path keeps the upper layers shared. C/WASM scene state emits frame artifacts and split
DRP2 packets; browser JavaScript selects adapters, configures canvases, translates input, and
executes the supported command subset through WebGPU. JavaScript should host the runtime, not
reimplement Datoviz visual semantics.

This architecture has one main rule: do not create parallel presentation, renderer, frame-stream,
or Vulkan-wrapper paths when extending v0.4. New work should either strengthen the scene contract,
extend DRP2/runtime support, or remain explicitly outside the release surface.

See also:

- [Scene model](scene-model.md)
- [Frame lifecycle](frame-lifecycle.md)
- [Portability and WebGPU](portability-webgpu.md)
