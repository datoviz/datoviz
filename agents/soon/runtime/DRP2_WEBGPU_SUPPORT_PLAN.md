# DRP2 WebGPU Support Plan

> **Execution Status**
> - **Status:** `PICKUP PLAN`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track DRP2/WebGPU runtime support work for the experimental browser path.


## Durable Contract

The shared WebGPU/WASM integration contract lives in
[../../../spec/scene/integration/WEBGPU_WASM.md](../../../spec/scene/integration/WEBGPU_WASM.md).

DRP2 command, conformance, capability, lifecycle, and fixture rules remain under
[../../../spec/drp2/](../../../spec/drp2/).

This file tracks DRP2 WebGPU runtime pickup order only. Scene techniques, frame graph policy, and
visual-family expansion remain out of scope except where they expose DRP2 command requirements.


## Current Position

The browser WebGPU path is a useful proof-of-concept DRP2 replay harness, not a first-class
renderer module. It lives under `examples/webgpu/`, executes the committed positive DRP2 fixture
manifest, and supports the narrow command subset needed by current browser streams and fixtures.

For v0.4, the target is a clear, tested subset with explicit unsupported-feature diagnostics, not
native Vulkan feature parity.


## Stage 1: Active Command Parity

Implement active DRP2 commands currently missing from the WebGPU runner:

1. `SetViewport`;
2. `SetScissor`;
3. `SetBlendConstant`;
4. `SetStencilReference`;
5. `CopyBufferToBuffer`.

Exit criteria:

1. every active command in `spec/drp2/COMMANDS.md` has executable WebGPU behavior or a deliberate
   diagnostic/reply-only no-op;
2. `just webgpu-fixture-preflight` stays green;
3. the browser fixture dashboard has no unsupported active-command entries.


## Stage 2: Remove Proof-Of-Concept Shortcuts

Replace demo conveniences with explicit DRP2/runtime behavior.

Current shortcuts to remove or confine to demo-only paths:

1. `texture_id: 0` as an implicit canvas target alias;
2. `"canvas"` dimensions and target formats in strict fixture paths;
3. fallback for missing pipeline vertex-buffer metadata;
4. fallback for missing pipeline color-target metadata;
5. binding unaligned buffer offsets at zero;
6. demo-local knowledge of scene-emitted uniform buffer ids.

Exit criteria:

1. strict fixture execution no longer relies on fallback pipeline or canvas metadata inference;
2. demo-only compatibility is isolated and documented, or removed.


## Stage 3: Lifecycle And Validation Parity

Make WebGPU runtime behavior match the native DRP2 semantic model:

1. implement real object destruction;
2. reject use-after-destroy;
3. reject destruction of objects still referenced by recorded or submitted work;
4. track command encoder, render pass, compute pass, and queue-submit state;
5. map validation failures to stable DRP2 error classes where practical;
6. add browser/preflight coverage for negative fixtures.


## Stage 4: Attachment And Format Coverage

Broaden render-pass and pipeline behavior where active DRP2 already permits it:

1. support multiple color attachments;
2. validate render-pipeline color targets against render-pass attachment formats;
3. expand texture/render-target format coverage according to active fixtures and scene streams;
4. tighten depth/stencil behavior;
5. implement blend state and color write-mask parity across color targets;
6. confirm load/store behavior for color and depth/stencil attachments.


## Stage 5: Compute, Readback, Capabilities, And Automation

After the render subset is stable:

1. add compute fixtures with multiple bind groups and storage buffers;
2. harden asynchronous queue submit and readback reporting;
3. report shader, texture, bind-group, alignment, and copy-row capabilities;
4. negotiate renderer features through DRP2 hello/hello-reply flow;
5. add a headless browser fixture runner;
6. produce parity reports comparing native DRP2 validation with browser execution.


## Deferred DRP2 Features

Keep these deferred until Stages 1-5 are stable and a concrete use case requires them:

1. `CreatePipelineLayout`;
2. `DestroyPipelineLayout`;
3. `ResourceBarrier`;
4. indirect draw and dispatch commands.

Promotion must update `spec/drp2/COMMANDS.md`, schemas, native semantic validation, WebGPU
execution, fixture coverage, and lifecycle rules together.


## Recommended Immediate Order

1. Implement `SetViewport` and `SetScissor`.
2. Implement `CopyBufferToBuffer`.
3. Add real destroy/lifetime behavior.
4. Add multiple color attachments and color-target validation.
5. Keep `CreatePipelineLayout` deferred during this work.
