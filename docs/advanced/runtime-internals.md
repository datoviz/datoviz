# Runtime Internals

Datoviz lowers retained scene state into DRP2 packets, then executes those packets through native
Vulkan or browser WebGPU runtimes.

Most application code should use scene, visual, app, and how-to pages. Use this page when changing
runtime execution, backend portability, recording/replay, or host integration.

## Stack

```text
scene state -> frame artifact -> DRP2 packets -> runtime -> output
```

| Layer | Owns | Does not own |
| --- | --- | --- |
| Scene | Figures, panels, visuals, controllers, scales, diagnostics, frame planning. | Backend handles, command buffers, swapchains, browser GPU objects. |
| DRP2 | Backend-neutral commands, validation, fixtures, packets, recording, capabilities. | Scene semantics, visual-family policy, user interaction behavior. |
| `vklite` | Native Vulkan resources, pipelines, synchronization, command recording, submission. | Panel layout, visual meaning, selection/query policy. |
| Canvas/stream | Swapchain/offscreen frames, capture pixels, sink fan-out, presentation timing. | Rendering semantics or scene-derived draw decisions. |
| WebGPU runtime | Browser adapter/device/canvas setup and execution of the supported DRP2 packet subset. | Native app/window/video parity or browser-only scene shortcuts. |

## Runtime Rules

- Scene semantics stay above DRP2.
- Runtime code executes packets; it does not infer visual-family meaning from ids or buffers.
- Borrowed Vulkan, platform, interop, and WASM spans need explicit lifetime and ownership rules.
- JSON is for fixtures, debug export, and DVZR recordings; hot paths use typed packets and payload
  arenas.
- Unsupported commands, formats, sample counts, ids, and lifecycle states must fail with explicit
  diagnostics.
- Capture/export pixels are tightly packed sRGB RGBA8 unless an API states otherwise.

## Where To Go

| Task | Page |
| --- | --- |
| Change command-stream semantics | [DRP2 command streams](drp2-command-streams.md) |
| Record or replay rendered frames | [Record/replay diagnostics](../how-to/record-replay.md) |
| Check browser support | [WebGPU subset](../reference/webgpu-subset.md) |
| Understand WebGPU architecture pressure | [Portability and WebGPU](../explanation/portability-webgpu.md) |
| Debug lower-level C symbols | [Runtime and utilities C API](../reference/c-api/runtime.md) |
| Add backend-facing commands | [Adding a DRP2 command](../contributors/adding-a-drp2-command.md) |

## Validation

Use the narrowest check that exercises the changed layer:

```sh
just test drp2
just test vklite
just test canvas
just test stream
just webgpu-fixture-preflight
just webgpu-runner-smoke
just wasm-scene-smoke
git diff --check
```

Add Vulkan validation, bounded GLFW/offscreen smoke, or `just webgpu-browser-smoke` when the change
touches swapchains, render targets, command buffers, synchronization, browser canvas setup, or
external handles.
