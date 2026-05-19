# Scene/WASM/WebGPU Port Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track the staged route for running the active scene code in the browser through
>   portable scene emission, WASM, WGSL DRP2 streams, and WebGPU execution.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/integration/WEBGPU_WASM.md`](../../../spec/scene/integration/WEBGPU_WASM.md)
2. [`../../../spec/scene/api/WASM_PORTABILITY.md`](../../../spec/scene/api/WASM_PORTABILITY.md)
3. [`../../../spec/drp2/`](../../../spec/drp2/)

Use this file only for implementation order, risk, and validation. Do not duplicate the browser
architecture, public scene API portability rules, DRP2 command contracts, or transport policy here.

The goal remains a documented, narrow scene/DRP2/WGSL/WebGPU subset that can run in a browser
without porting `vk`, `vklite`, `canvas`, `window`, `stream`, `video`, or native `app`.


## Remaining Port Work

Recommended follow-up commits:

1. Add a portable native build target for scene plus DRP2 stream/semantic code with Vulkan,
   vklite, canvas, and native app code disabled.
2. Fix any public header leakage exposed by that target, especially native runtime, Vulkan, canvas,
   stream-frame, window, or platform types.
3. Add an Emscripten build profile for the portable scene/DRP2 subset.
4. Export a tiny C/WASM API for creating/destroying a scene, figure, panel, and point visual, then
   emitting one WGSL DRP2 frame stream.
5. Feed the scene-emitted point stream into the existing WebGPU runner without demo-specific stream
   rewriting.
6. Add primitive and image scenes after the point milestone is stable.
7. Add incremental uniform-buffer updates for pan/zoom and resize before attempting large visual
   payloads.
8. Replace JSON/base64 hot-path transport with direct payload spans or a compact binary command
   path once the debugging path is proven.
9. Add browser app examples only after the runtime can preserve resources across repeated frame
   submissions.
10. Broaden parity to readback, multi-panel, linked controllers, mesh/path/sphere/text/colorbar,
   and graph-backed techniques only after the core path is usable and efficient.


## First Milestone

1. Build a WASM module containing portable scene and DRP2 stream emission.
2. From browser JavaScript, create one scene, one figure, one panel, and one point visual.
3. Request WGSL scene emission with an external browser canvas color target.
4. Submit the emitted DRP2 stream to the WebGPU runtime.
5. Render points using the portable WebGPU lowering selected by scene emission.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation slices, use progressively broader validation:

```text
portable native build without Vulkan/canvas
DRP2 semantic tests that do not require vklite
scene emission tests requesting WGSL without vklite execution
Emscripten compile
Node or browser smoke for create/destroy and stream emission
just webgpu-fixture-preflight
```

Add browser point/primitive/image canvas smokes, repeated-frame resource-count checks, and
native/browser stream comparisons as the runtime moves beyond the first milestone.
