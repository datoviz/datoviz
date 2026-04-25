# DRP2 Schema

This directory contains machine-readable schema material for DRP2.


## Authority

The authoritative protocol meaning still lives first in the prose spec.

Active source-of-truth documents:

- `../COMMANDS.md`: active command set plus per-command field semantics.
- `../ERRORS.md`: validation and error model.
- `../LIFETIMES.md`: lifetime and encoder/pass state rules.
- `../VERSIONING.md`: compatibility and contract-evolution rules.

If schema and markdown diverge, the markdown contract wins until the schema is updated.


## Active Versus Deferred Schema Material

Only the active files listed below are authoritative for the current DRP2 review surface.
Everything else in this directory is draft material and must be treated as non-authoritative unless and
until it is promoted into the active list.

Authoritative files:

- `drp_command.json`
- `common/*.json`
- `commands/HelloRenderer.json`
- `commands/RendererHelloReply.json`
- `commands/Error.json`
- `commands/CreateBuffer.json`
- `commands/DestroyBuffer.json`
- `commands/WriteBuffer.json`
- `commands/CreateTexture.json`
- `commands/DestroyTexture.json`
- `commands/WriteTexture.json`
- `commands/CreateBindGroup.json`
- `commands/DestroyBindGroup.json`
- `commands/CreateBindGroupLayout.json`
- `commands/DestroyBindGroupLayout.json`
- `commands/CreateShaderModule.json`
- `commands/DestroyShaderModule.json`
- `commands/CreateRenderPipeline.json`
- `commands/DestroyRenderPipeline.json`
- `commands/CreateComputePipeline.json`
- `commands/DestroyComputePipeline.json`
- `commands/BeginCommandEncoder.json`
- `commands/FinishCommandEncoder.json`
- `commands/BeginRenderPass.json`
- `commands/EndRenderPass.json`
- `commands/BeginComputePass.json`
- `commands/EndComputePass.json`
- `commands/SetPipeline.json`
- `commands/SetVertexBuffer.json`
- `commands/SetIndexBuffer.json`
- `commands/SetBindGroup.json`
- `commands/SetViewport.json`
- `commands/SetScissor.json`
- `commands/SetBlendConstant.json`
- `commands/SetStencilReference.json`
- `commands/Draw.json`
- `commands/DrawIndexed.json`
- `commands/DispatchWorkgroups.json`
- `commands/CopyBufferToBuffer.json`
- `commands/CopyBufferToTexture.json`
- `commands/CopyTextureToBuffer.json`
- `commands/QueueSubmit.json`
- `commands/QueueSubmitReply.json`
- `commands/CreateSampler.json`
- `commands/DestroySampler.json`
- `commands/CreateTextureView.json`
- `commands/DestroyTextureView.json`

Deferred, non-authoritative files:

- `commands/CreatePipelineLayout.json`
- `commands/DestroyPipelineLayout.json`
- `commands/ResourceBarrier.json`
- `commands/DispatchWorkgroupsIndirect.json`
- `commands/DrawIndirect.json`
- `commands/DrawIndexedIndirect.json`

See `DEFERRED.md` for the explicit deferred inventory.


## Maintenance Rules

When changing the active protocol:

1. update `../COMMANDS.md` first,
2. update the relevant `commands/*.json` files to match,
3. update `drp_command.json` so its root union matches the active command set,
4. move future-facing draft schemas back into the deferred list until their field semantics are frozen in prose.
