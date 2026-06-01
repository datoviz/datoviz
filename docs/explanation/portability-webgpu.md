# Portability And WebGPU

Datoviz treats WebGPU as portability pressure on the scene and DRP2 contracts, not as a second
renderer architecture.

The active native path is:

```text
scene frame plans -> DRP2 command streams -> vklite runtime -> canvas/stream/app
```

The experimental browser path keeps the same upper layers:

```text
C/WASM scene state -> scene frame plan -> WGSL DRP2 stream -> browser WebGPU runtime -> canvas
```

The scene layer owns visual semantics, controller math, lowering decisions, shader selection,
resource ids, and diagnostics. The browser runtime executes DRP2 commands. It must not learn
Datoviz visual families or mutate scene-owned buffers through browser-only shortcuts.

## Why The Subset Is Small

The v0.4 browser goal is an honest experimental subset, not native Vulkan parity. A small subset is
useful because it checks the hard boundaries:

- public DRP2 commands stay backend-agnostic;
- WGSL emission stays available for portable scene paths;
- unsupported features fail explicitly;
- browser execution replays the same DRP2-shaped command streams used by native planning tests;
- JavaScript handles browser concerns such as adapter selection, canvas configuration, DPR-aware
  resize, input event translation, and status reporting.

This keeps portability work from forking scene semantics.

## Current Browser Shape

The first scene subset proves point, primitive, RGBA8 image, basic mesh, panzoom, and a basic 3D
mesh with arcball through the generic WASM scene ABI. The browser pages call the same handle-based
`dvz_wasm_api_*` object API and feed emitted WGSL DRP2 JSON into the browser WebGPU runtime.

The pure browser WebGPU runner has broader DRP2 fixture coverage than the live WASM demos. It
validates resource lifetimes, capability failures, dynamic offsets, render and compute passes,
texture copies, readback-style fixtures, and negative semantic fixtures. That runner is the
conformance pressure; the live WASM pages are scene-integration proof.

## Diagnostics

Diagnostics are part of the portability contract. Unsupported commands, unsupported shader formats,
unsupported texture formats, unsupported sample counts, invalid ids, lifecycle errors, and malformed
command order should surface as deterministic DRP2-level failures.

The WASM ABI exposes scene diagnostics separately from browser WebGPU errors. A failed scene emit
should return a non-zero status and make diagnostics available through
`dvz_wasm_api_diagnostic_count()` and `dvz_wasm_api_diagnostic()`. A successful emit should leave
the report empty.

## Validation Strategy

Validation is layered:

- C scene tests check portable WGSL frame-plan emission;
- fixture preflight checks committed DRP2 streams and negative fixtures without a browser;
- the browser runner smoke uses a fake WebGPU device for command-path parity;
- the fixture dashboard executes the committed subset in a real browser WebGPU runtime;
- the WASM scene pages prove browser input -> C scene/controller state -> DRP2 emission -> WebGPU
  replay.

Run:

```bash
just webgpu-fixture-preflight
just webgpu-runner-smoke
just wasm-scene-smoke
```

For browser evidence, serve the repository and inspect the fixture dashboard plus the unified WASM
examples host:

```bash
python3 -m http.server 8765
```

```text
http://localhost:8765/examples/webgpu/fixtures.html
http://localhost:8765/examples/webgpu/examples.html?demo=wasm-2d
http://localhost:8765/examples/webgpu/examples.html?demo=wasm-3d
```

The release bar is clear scope, repeatable validation, and explicit unsupported-feature behavior.
Broader visual parity can follow after those properties are stable.
