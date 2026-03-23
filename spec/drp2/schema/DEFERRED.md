# Deferred DRP2 Schemas

This file lists schema files that are present in-tree but are currently deferred and non-authoritative.

Use `../COMMANDS.md` and `README.md` in this directory as the source of truth for the active protocol
surface under review.

Deferred command schema files:

- `commands/CreateBindGroup.json`
- `commands/DestroyBindGroup.json`
- `commands/CreateBindGroupLayout.json`
- `commands/DestroyBindGroupLayout.json`
- `commands/CreatePipelineLayout.json`
- `commands/DestroyPipelineLayout.json`
- `commands/CreateShaderModule.json`
- `commands/DestroyShaderModule.json`
- `commands/CreateSampler.json`
- `commands/DestroySampler.json`
- `commands/CreateTextureView.json`
- `commands/DestroyTextureView.json`
- `commands/CreateRenderPipeline.json`
- `commands/DestroyRenderPipeline.json`
- `commands/CreateComputePipeline.json`
- `commands/DestroyComputePipeline.json`
- `commands/ResourceBarrier.json`
- `commands/DispatchWorkgroupsIndirect.json`
- `commands/DrawIndirect.json`
- `commands/DrawIndexedIndirect.json`

Deferred means:

1. the filename may reserve a future command name,
2. the JSON shape may be incomplete, stale, or incompatible with the active prose contract,
3. reviewers must not treat these files as compatibility commitments,
4. implementations must not rely on them until they are promoted into the active list.
