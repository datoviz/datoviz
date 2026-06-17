# Portability and WebGPU

Datoviz treats WebGPU as portability pressure on the scene and DRP2 contracts, not as a second
renderer architecture.

## Shared Architecture

The active native path is:

```text
scene frame plans -> DRP2 command streams -> vklite runtime -> canvas/stream/app
```

The experimental browser path keeps the same upper layers:

```text
C/WASM scene state -> scene frame plan -> DRP2 packets -> browser WebGPU runtime -> canvas
```

The scene layer owns visual semantics, controller math, lowering decisions, shader selection,
resource ids, and diagnostics. The browser runtime executes DRP2 commands. It must not learn
Datoviz visual families or mutate scene-owned buffers through browser-only shortcuts.

## Why the Subset Is Small

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

The current browser shape has two layers. The public live examples route promoted scene examples
through the generic WASM scene ABI and split DRP2 setup, update, and frame packets. The pure
browser WebGPU runner executes committed DRP2 fixtures and semantic negative cases without needing
the full scene host.

The promoted live subset is broader than the original point-and-panzoom proof and changes as RC
evidence lands. It currently includes core retained visual families, composed layout/adornment
routes, several controller routes, selected query/readback routes, and compute-to-render examples.
Keep the exact list in [WebGPU subset](../reference/webgpu-subset.md), not on this explanation
page.

JSON emission remains a debug and fixture-export view. The browser render path consumes frame
artifact packet spans, not JSON.

## Vulkan/WebGPU Parity Boundary

Parity for v0.4 means that the portable DRP2 subset is shared and diagnosed consistently. It does
not mean that every native scene, app, or visual feature renders in the browser.

| Capability area | Native Vulkan path | Browser WebGPU/WASM path |
| --- | --- | --- |
| Scene semantics | supported for declared v0.4 scene/app surface | shared for the experimental WASM scene subset |
| Runtime transport | DRP2 streams into vklite/canvas/stream/app | split binary DRP2 setup/update/frame packets into WebGPU |
| Shader input | native runtime may use internal GLSL/SPIR-V/WGSL paths | WGSL only |
| Visual families | broader retained v0.4 visual surface | promoted retained scene subset; see WebGPU subset reference |
| Interaction | native controllers through app/window paths | promoted controller examples through WASM ABI input |
| DRP2 command execution | native runtime under active hardening | committed positive fixture slice, WebGPU attachment streams, and semantic negative parity |
| Compute | experimental compute-to-render lane | fixture-level and promoted live-route proof for the declared subset |
| Query/readback | native promoted query/readback slices | asynchronous browser query/readback for promoted routes only |
| App/window/video/GUI | native-only v0.4 ownership | unsupported in WASM |
| Diagnostics | scene, DRP2, and runtime diagnostics | scene ABI diagnostics plus WebGPU runner capability/lifecycle diagnostics |

## Diagnostics

Diagnostics are part of the portability contract. Unsupported commands, unsupported shader formats,
unsupported texture formats, unsupported sample counts, invalid ids, lifecycle errors, and malformed
command order should surface as deterministic DRP2-level failures.

The WASM ABI exposes scene diagnostics separately from browser WebGPU errors. A failed scene emit
should return a non-zero status and expose diagnostics through the documented WASM diagnostic
accessors. A successful emit should leave the report empty.

## Validation Strategy

Validation is layered rather than exhaustive on every route:

- C scene tests check portable WGSL frame-plan emission;
- fixture preflight checks committed DRP2 streams and negative fixtures without a browser;
- the browser runner smoke uses a fake WebGPU device for command-path parity;
- the fixture dashboard executes the committed subset in a real browser WebGPU runtime;
- the WASM scene pages prove browser input -> C scene/controller state -> DRP2 emission -> WebGPU
  replay.

The release bar is clear scope, repeatable validation, and explicit unsupported-feature behavior.
Broader visual parity can follow after those properties are stable.

See also:

- [WebGPU subset](../reference/webgpu-subset.md)
- [Compute and graphics](../reference/compute-graphics.md)
- [Deploy WebGPU examples to the browser](../how-to/deploy-to-web.md)
