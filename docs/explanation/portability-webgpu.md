# Portability And WebGPU

Datoviz treats WebGPU as portability pressure on the scene and DRP2 contracts, not as a second
renderer architecture.

The active native path is:

```text
scene frame plans -> DRP2 command streams -> vklite runtime -> canvas/stream/app
```

The experimental browser path keeps the same upper layers:

```text
C/WASM scene state -> scene frame plan -> WGSL DRP2 packets -> browser WebGPU runtime -> canvas
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

The first scene subset proves point, pixel, marker, basic segment, basic path, primitive, RGBA8
image, basic/textured mesh, basic sphere, panzoom, and a 3D sphere + textured mesh scene with arcball
through the generic WASM scene ABI. The browser pages call the same handle-based `dvz_wasm_api_*` object API and feed emitted
split DRP2 setup, update, and frame packets into the browser WebGPU runtime. JSON emission remains a
debug and fixture-export view.

The pure browser WebGPU runner has broader DRP2 fixture coverage than the live WASM demos. It
validates resource lifetimes, capability failures, dynamic offsets, render and compute passes,
texture copies, readback-style fixtures, and negative semantic fixtures. That runner is the
conformance pressure; the live WASM pages are scene-integration proof.

## Vulkan/WebGPU Parity Boundary

Parity for v0.4 means that the portable DRP2 subset is shared and diagnosed consistently. It does
not mean that every native scene, app, or visual feature renders in the browser.

| Capability area | Native Vulkan path | Browser WebGPU/WASM path |
| --- | --- | --- |
| Scene semantics | supported for declared v0.4 scene/app surface | shared for the experimental WASM scene subset |
| Runtime transport | DRP2 streams into vklite/canvas/stream/app | split binary DRP2 setup/update/frame packets into WebGPU |
| Shader input | native runtime may use internal GLSL/SPIR-V/WGSL paths | WGSL only |
| Visual families | broader retained v0.4 visual surface | point, pixel, basic marker, basic segment, basic path, primitive, RGBA8 image, basic/textured mesh, and basic sphere demos |
| Interaction | native controllers through app/window paths | panzoom and one arcball scene through WASM ABI input |
| DRP2 command execution | native runtime under active hardening | committed positive fixture slice, WebGPU attachment streams, and semantic negative parity |
| Compute | experimental compute-to-render lane | fixture-level compute and ResourceBarrier validation; gallery-level parity still experimental |
| Query/readback | native point/image first slices under active validation | DRP2 fixture/readback pressure only; WASM scene query parity deferred |
| App/window/video/GUI | native-only v0.4 ownership | unsupported in WASM |
| Diagnostics | scene, DRP2, and runtime diagnostics | scene ABI diagnostics plus WebGPU runner capability/lifecycle diagnostics |

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
just webgpu-browser-smoke
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
