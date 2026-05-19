# DRP2 WebGPU Support Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track DRP2/WebGPU runtime pickup order for the experimental browser path.


## Current State

Durable browser integration rules live in
[`../../../spec/scene/integration/WEBGPU_WASM.md`](../../../spec/scene/integration/WEBGPU_WASM.md).
DRP2 command, conformance, capability, lifecycle, error, and fixture rules remain under
[`../../../spec/drp2/`](../../../spec/drp2/).

Use this file only for WebGPU runtime execution sequencing. Do not duplicate scene/WASM
architecture, DRP2 command semantics, or fixture contracts here.

The browser WebGPU path is still a narrow proof-of-concept replay harness under `examples/webgpu/`.
For v0.4, the target is a tested subset with explicit unsupported-feature diagnostics, not native
Vulkan feature parity.


## Remaining WebGPU Runtime Work

Recommended follow-up commits:

1. Implement active command parity for `SetViewport`, `SetScissor`, `SetBlendConstant`,
   `SetStencilReference`, and `CopyBufferToBuffer`.
2. Replace strict fixture shortcuts such as implicit canvas aliases, missing pipeline metadata
   fallback, unaligned-offset fallback, and demo-local scene uniform assumptions.
3. Add real destroy/lifetime behavior and reject use-after-destroy or destruction while referenced
   by recorded/submitted work.
4. Track command encoder, render pass, compute pass, and queue-submit state closely enough to map
   validation failures to stable DRP2 diagnostics.
5. Broaden attachment and format coverage: multiple color attachments, color-target validation,
   depth/stencil behavior, blend state, color write masks, and load/store behavior.
6. Add compute, readback, capabilities, and automation only after the render subset is stable.
7. Keep `CreatePipelineLayout`, `DestroyPipelineLayout`, `ResourceBarrier`, and indirect commands
   deferred until a concrete use case promotes them across DRP2 specs, schemas, native validation,
   WebGPU execution, fixtures, and lifecycle rules together.


## Immediate Order

1. Implement `SetViewport` and `SetScissor`.
2. Implement `CopyBufferToBuffer`.
3. Add real destroy/lifetime behavior.
4. Add multiple color attachments and color-target validation.
5. Keep `CreatePipelineLayout` deferred during this work.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation changes in this lane, use the narrowest relevant DRP2/WebGPU validation:

```text
just webgpu-fixture-preflight
```

Add browser fixture dashboard checks and native DRP2 semantic validation when a change affects
command behavior, lifecycle rules, capability reporting, or fixture contracts.
