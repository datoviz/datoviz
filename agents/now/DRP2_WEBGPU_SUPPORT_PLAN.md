# DRP2 WebGPU Support Plan

> Status: pickup plan.
> Created: 2026-05-16.
> Scope: DRP2/WebGPU runtime support only. Scene techniques, frame graph policy, and visual-family
> expansion are intentionally out of scope except where they expose DRP2 command requirements.

## Current Position

The browser WebGPU path is currently a useful proof-of-concept DRP2 replay harness, not a first-class
Datoviz renderer module. It lives under `examples/webgpu/`, executes the committed positive DRP2
fixture manifest, and supports the narrow command subset needed by the current browser streams and
fixtures.

Active DRP2 does not require explicit `CreatePipelineLayout` support. Pipeline-layout information is
carried directly on `CreateRenderPipeline.bind_group_layout_ids` and
`CreateComputePipeline.bind_group_layout_ids`. Backends synthesize their native pipeline-layout
objects from those ordered bind-group layouts at pipeline creation time. Keep
`CreatePipelineLayout`, `DestroyPipelineLayout`, `ResourceBarrier`, and indirect draw/dispatch
commands deferred until a real use case promotes them into the active DRP2 contract.

## Non-Goals

1. Do not fork scene semantics for WebGPU.
2. Do not make WBOIT, EDL, G-buffer, SSAO, or other scene techniques WebGPU-specific.
3. Do not promote deferred DRP2 schema commands just to mirror Vulkan or WebGPU object models.
4. Do not add browser-side GLSL-to-WGSL translation. Scene-owned streams should already carry WGSL
   when targeting WebGPU.

## Stage 1 - Active Command Parity

Implement the active DRP2 commands currently missing from the WebGPU runner:

1. `SetViewport`.
2. `SetScissor`.
3. `SetBlendConstant`.
4. `SetStencilReference`.
5. `CopyBufferToBuffer`.

Validation goals:

1. Add positive fixtures or browser fixture coverage for each command.
2. Add negative cases where command placement is invalid, for example dynamic state outside a render
   pass or copies inside a pass.
3. Keep deferred commands unsupported with explicit diagnostics.

Exit criteria:

1. Every active command listed in `spec/drp2/COMMANDS.md` has either executable WebGPU behavior or a
   deliberate no-op only when the command is diagnostic/reply-only.
2. `just webgpu-fixture-preflight` stays green.
3. The browser fixture dashboard has no unsupported active-command entries.

## Stage 2 - Remove PoC Shortcuts

Replace demo conveniences with explicit DRP2/runtime behavior.

Current shortcuts to remove or confine to demo-only paths:

1. `texture_id: 0` as an implicit canvas target alias.
2. `"canvas"` texture dimensions and pipeline target formats in strict fixture paths.
3. Compatibility fallback for missing `CreateRenderPipeline.vertex_buffers`.
4. Compatibility fallback for missing `CreateRenderPipeline.color_targets`.
5. Binding unaligned buffer offsets at zero.
6. Demo-local knowledge of scene-emitted uniform buffer ids for interactive pan/zoom.

Preferred direction:

1. Define an explicit runtime target binding rule for browser canvas targets.
2. Require portable producers and strict fixtures to emit explicit vertex and color-target metadata.
3. Either adapt WebGPU buffer-offset alignment correctly or reject unsupported offsets with a clear
   capability/validation error.
4. Move interactive uniform target metadata into portable stream metadata rather than hard-coded
   browser demo ids.

Exit criteria:

1. Strict fixture execution no longer relies on fallback pipeline or canvas metadata inference.
2. Demo-only compatibility remains isolated and documented, or is removed.

## Stage 3 - Lifecycle And Validation Parity

Make WebGPU runtime behavior match the native DRP2 semantic model rather than only replaying happy
paths.

Work items:

1. Implement real object destruction for buffers, textures, texture views, samplers, bind-group
   layouts, bind groups, shader modules, render pipelines, and compute pipelines.
2. Reject use-after-destroy.
3. Reject destruction of objects still referenced by recorded or submitted work.
4. Track command encoder, render pass, compute pass, and queue-submit state with the same broad
   invariants as native semantic validation.
5. Map validation failures to stable DRP2 error codes/classes where practical.
6. Add browser/preflight coverage for negative fixtures, not only positive fixture replay.

Exit criteria:

1. Destroy commands are no longer accepted as generic no-op lifecycle markers.
2. Negative fixture behavior is deterministic and aligned with native DRP2 validation where the
   browser can express the same condition.
3. Error reporting is specific enough for fixture dashboards and CI logs to identify the failing
   command and reason.

## Stage 4 - Attachment And Format Coverage

Broaden render-pass and pipeline behavior where active DRP2 already permits it.

Work items:

1. Support multiple color attachments.
2. Validate render-pipeline `color_targets` against render-pass attachment formats.
3. Expand texture and render-target format coverage according to active fixtures and scene-emitted
   streams.
4. Tighten depth/stencil behavior beyond the current narrow depth path.
5. Implement blend state and color write-mask parity across all color targets.
6. Confirm load/store behavior for color and depth/stencil attachments.

Exit criteria:

1. Multi-target DRP2 render-pass fixtures execute in WebGPU.
2. Format mismatches produce deterministic validation errors.
3. Browser and native fixture results agree for attachment-count, blend, write-mask, and depth
   behavior in the covered subset.

## Stage 5 - Compute And Readback Hardening

The browser runner already has a narrow compute/readback path. This stage makes it renderer-grade.

Work items:

1. Add more compute pipeline fixtures, including multiple bind groups and storage buffers.
2. Cover storage texture or storage-buffer edge cases as active DRP2 requires them.
3. Harden asynchronous `QueueSubmit` and readback reporting.
4. Add deterministic readback validation for selected fixtures.
5. Define how device loss, WebGPU validation errors, and asynchronous mapping failures are reported
   through DRP2 diagnostics.

Exit criteria:

1. Compute-assisted fixtures run without demo-only assumptions.
2. Readback fixtures report stable results and useful failure details.
3. Async WebGPU failures surface as DRP2-level errors instead of generic JavaScript exceptions.

## Stage 6 - Capability Model

Add a WebGPU capability snapshot aligned with DRP2 negotiation and validation.

Work items:

1. Report supported shader formats, expected to be WGSL only for the browser path.
2. Report supported texture formats, usages, sample counts, and limits.
3. Report bind-group, binding, dynamic-offset, alignment, and buffer-copy-row constraints.
4. Negotiate renderer features through `HelloRenderer` and `RendererHelloReply`.
5. Use the capability snapshot to reject unsupported streams before partial execution where
   possible.

Exit criteria:

1. A WebGPU runtime can explain why a valid DRP2 stream is unsupported on the current browser/device.
2. Capability failures are distinguishable from malformed-stream validation failures.
3. Fixture dashboards can classify pass, fail, and unsupported cases consistently.

## Stage 7 - Automation

Move WebGPU validation out of manual browser checks.

Work items:

1. Add a headless browser fixture runner.
2. Run positive fixtures in CI-capable browser environments.
3. Add negative fixture coverage once lifecycle and validation parity are implemented.
4. Add selected image/hash or readback checks where deterministic output is practical.
5. Produce a parity report comparing native DRP2 semantic validation with browser WebGPU execution.

Exit criteria:

1. WebGPU fixture status is available from a command-line recipe.
2. CI can catch active-command regressions without manual dashboard use.
3. Manual `examples/webgpu/fixtures.html` remains useful for debugging but is not the only
   validation path.

## Stage 8 - Deferred DRP2 Features

Only revisit these after Stages 1-7 are stable and a concrete use case requires them:

1. `CreatePipelineLayout`.
2. `DestroyPipelineLayout`.
3. `ResourceBarrier`.
4. `DrawIndirect`.
5. `DrawIndexedIndirect`.
6. `DispatchWorkgroupsIndirect`.

Decision rule:

1. Promote a deferred command only when it reduces real duplication, enables a needed renderer
   feature, or represents semantics that cannot be cleanly expressed through the active command
   surface.
2. Promotion must update `spec/drp2/COMMANDS.md`, schemas, native semantic validation, WebGPU
   execution, fixture coverage, and lifecycle rules together.

## Recommended Immediate Order

For the next implementation pass, prioritize:

1. `SetViewport` and `SetScissor`, because they are ordinary active render-pass commands and are
   important for multi-panel and subpass-shaped replay.
2. `CopyBufferToBuffer`, because it is a straightforward active copy command and closes an obvious
   command-parity gap.
3. Real destroy/lifetime behavior, because it is the largest difference between fixture replay and a
   renderer-grade DRP2 backend.
4. Multiple color attachments and color-target validation, because they are the most likely DRP2
   requirements exposed by graph-backed multi-pass techniques.

Keep `CreatePipelineLayout` deferred during this work.
