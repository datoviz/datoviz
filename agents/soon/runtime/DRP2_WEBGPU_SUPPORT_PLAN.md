# DRP2 WebGPU Support Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-29`
> - **Purpose:** track DRP2/WebGPU runtime pickup order for the experimental browser path.


## Current State

Durable browser integration rules live in
[`../../../spec/scene/integration/WEBGPU_WASM.md`](../../../spec/scene/integration/WEBGPU_WASM.md).
DRP2 command, conformance, capability, lifecycle, error, and fixture rules remain under
[`../../../spec/drp2/`](../../../spec/drp2/).

Use this file only for WebGPU runtime execution sequencing. Do not duplicate scene/WASM
architecture, DRP2 command semantics, or fixture contracts here.

The browser WebGPU path is a narrow experimental replay harness under `examples/webgpu/`. For v0.4,
the target is a tested subset with explicit unsupported-feature diagnostics, not native Vulkan
feature parity.

Current automated evidence:

1. `just webgpu-fixture-preflight` passes the committed strict subset: `39/39`.
2. `just webgpu-runner-smoke` passes the browserless runner smoke:
   `37` positive fixtures, `2` WebGPU streams, `81` semantic negative fixtures, and retained
   repeated-frame resource checks for point, primitive, texture-sampling, and depth-attachment
   streams.
3. The strict subset includes point, primitive, and image scene-emitted WGSL fixtures, multiple color
   attachments, depth attachments, copy commands, compute dispatch, readback, dynamic buffer
   updates, and destroy/lifetime validation.
4. The browser fixture dashboard passed `120/120` rows on `2026-05-28` after the
   repeated-runtime-frame smoke slice (`183812f27`).
5. The browser fixture dashboard passed fixture compatibility `120/120` and retained runtime
   stress `4/4` on `2026-05-29` after `292e82899`.
6. The browser fixture dashboard passed fixture compatibility `120/120` and retained runtime
   stress `7/7`, including demo-session pan/zoom, resize, and stream-reload paths, on
   `2026-05-29` after `a1c0d7306`.


## Remaining WebGPU Runtime Work

Completed since the initial follow-up note:

1. Active command parity exists for `SetViewport`, `SetScissor`, `SetBlendConstant`,
   `SetStencilReference`, and `CopyBufferToBuffer`.
2. Strict fixture paths require explicit bind-group layout visibility/access metadata and explicit
   render-pipeline vertex/color-target metadata.
3. Destroy commands validate use-after-destroy and recorded/submitted-work dependencies before
   tombstoning or destroying browser objects.
4. The runner tracks command encoder, render pass, compute pass, queue-submit state, and DRP2
   diagnostic context for the committed negative fixture parity set.
5. Multiple color attachments, color-target validation, depth/stencil attachment matching, blend
   state, color write masks, and load/store operation checks are covered by WebGPU streams or
   preflight/smoke tests.
6. Compute dispatch, readback fixtures, and browserless automation are now part of the committed
   subset.
7. Real-browser retained-runtime stress rows now cover repeated frames for point, primitive,
   texture-sampling, and depth-attachment streams.
8. Main-demo session stress rows now cover pan/zoom uniform updates, resize-triggered reload, and
   stream reload on the same helper used by the demo page.

Remaining follow-up commits:

1. Replace remaining standalone-demo compatibility shortcuts where practical: implicit canvas
   aliases, unaligned-offset fallback, and demo-local scene uniform id assumptions. Scene
   interaction must be WASM/scene-owned: browser input should become scene/controller calls, and
   the scene layer should emit DRP2 update commands for the WebGPU runtime to execute directly.
   Do not promote browser-side direct uniform mutation into the app architecture.
2. Add a DRP2-aligned capability snapshot for the browser runtime and use it to produce explicit
   unsupported-feature diagnostics before execution.
3. Keep `CreatePipelineLayout`, `DestroyPipelineLayout`, `ResourceBarrier`, and indirect commands
   deferred until a concrete use case promotes them across DRP2 specs, schemas, native validation,
   WebGPU execution, fixtures, and lifecycle rules together.


## Immediate Order

1. Add browser capability reporting and unsupported-feature diagnostics.
2. Start the portable scene/WASM emission milestone from
   [`SCENE_WASM_WEBGPU_PORT_PLAN.md`](SCENE_WASM_WEBGPU_PORT_PLAN.md).
3. Keep `CreatePipelineLayout` deferred during this work.


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
