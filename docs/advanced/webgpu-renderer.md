# WebGPU Renderer

The WebGPU renderer is Datoviz's experimental browser executor for the portable DRP2 subset. It is
not a port of `vklite`, `canvas`, `window`, `stream`, `video`, or native `app` into WebAssembly.

Use this page when working on browser runtime execution. For user-facing support status, use the
WebGPU subset reference and gallery pages.

## Role in the Stack

The browser path is:

```text
C/WASM scene state -> scene frame artifact -> split DRP2 packets ->
browser WebGPU runtime -> HTML canvas
```

The WASM scene layer owns visual semantics, controller math, resource ids, shader selection,
diagnostics, and frame artifact emission. The browser renderer owns WebGPU adapter/device/canvas
setup and executes the supported DRP2 packet subset.

## Runtime Responsibilities

The browser runtime manages WebGPU objects:

- buffers, textures, texture views, samplers, bind groups, and layouts;
- WGSL shader modules and render/compute pipelines;
- command encoders, render passes, compute passes, copies, and readback requests;
- retained runtime resource tables across repeated frames;
- resize, canvas configuration, browser capability reporting, and diagnostics.

The runtime should execute packets directly. It must not mutate scene-owned buffers through
hard-coded ids, visual-family assumptions, or browser-only shortcuts.

## Transport Model

JSON remains a debug and fixture-export view. The browser render path consumes split binary DRP2
setup, update, and frame packets plus payload arenas owned by the current scene frame artifact.

JavaScript must copy or decode borrowed WASM spans before releasing the artifact or emitting the
next frame. Long-lived browser GPU resources should be keyed by stable DRP2 ids and refreshed by
setup/update packets rather than rebuilt from scratch every frame.

## Parity Boundary

WebGPU parity means shared scene and DRP2 semantics for the promoted browser subset. It does not
mean full native Vulkan feature parity.

Unsupported commands, shader formats, texture formats, sample counts, invalid ids, lifecycle
errors, and malformed stream state should fail with explicit diagnostics. New browser features need
matching scene emission, DRP2 validation or fixture coverage, WebGPU execution, capability
reporting, and documentation updates.

## Validation

Use browserless checks first:

```sh
just webgpu-fixture-preflight
just webgpu-runner-smoke
just wasm-scene-smoke
git diff --check
```

Use the browser smoke when Chrome/Chromium and WebGPU are available:

```sh
just webgpu-browser-smoke
```

See also:

- [Portability and WebGPU](../explanation/portability-webgpu.md)
- [WebGPU subset](../reference/webgpu-subset.md)
- [Deploy WebGPU examples to the browser](../how-to/deploy-to-web.md)
- [DRP2](../reference/drp2/index.md)
