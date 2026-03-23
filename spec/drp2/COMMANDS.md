# DRP2 Commands

This document defines the authoritative prose contract for the active DRP2 command surface.

It does two things:

1. names the command set currently in scope,
2. defines the intended meaning of each command field.

If an active JSON schema under `schema/` disagrees with this document, this document wins until the
schema is updated.


## Scope

The active DRP2 command set is intentionally small but complete enough for:

1. render-first workloads,
2. compute-assisted workloads,
3. explicit object destruction,
4. deterministic validation,
5. native and browser runtime parity.


## Active Command Surface

### Session And Diagnostics

1. `HelloRenderer`
2. `RendererHelloReply`
3. `Error`


### Resource Lifecycle

1. `CreateBuffer`
2. `DestroyBuffer`
3. `WriteBuffer`
4. `CreateTexture`
5. `DestroyTexture`
6. `WriteTexture`


### Encoder And Pass Lifecycle

1. `BeginCommandEncoder`
2. `FinishCommandEncoder`
3. `BeginRenderPass`
4. `EndRenderPass`
5. `BeginComputePass`
6. `EndComputePass`


### Recording Commands

1. `SetPipeline`
2. `SetBindGroup`
3. `SetViewport`
4. `SetScissor`
5. `SetBlendConstant`
6. `SetStencilReference`
7. `Draw`
8. `DrawIndexed`
9. `DispatchWorkgroups`


### Copy And Submission

1. `CopyBufferToBuffer`
2. `CopyBufferToTexture`
3. `CopyTextureToBuffer`
4. `QueueSubmit`


## Deferred And Non-Authoritative Commands

The following command names exist in the schema tree as deferred design material only. They are not part
of the active DRP2 contract and must not be treated as authoritative until promoted into this document.

1. `CreateBindGroup`
2. `DestroyBindGroup`
3. `CreateBindGroupLayout`
4. `DestroyBindGroupLayout`
5. `CreatePipelineLayout`
6. `DestroyPipelineLayout`
7. `CreateShaderModule`
8. `DestroyShaderModule`
9. `CreateSampler`
10. `DestroySampler`
11. `CreateTextureView`
12. `DestroyTextureView`
13. `CreateRenderPipeline`
14. `DestroyRenderPipeline`
15. `CreateComputePipeline`
16. `DestroyComputePipeline`
17. `ResourceBarrier`
18. `DispatchWorkgroupsIndirect`
19. `DrawIndirect`
20. `DrawIndexedIndirect`


## Common Field Semantics

Unless a command says otherwise:

- `cmd`: required discriminator naming the command kind.
- `id`: client-assigned identifier for a newly created object.
- `<object>_id`: identifier of an existing object of that kind.
- `label`: optional human-readable debug name with no protocol effect.
- `offset`: byte offset into a buffer.
- `size`: byte count for transfers, or transfer extent when the field is structurally typed as an extent.
- `encoder_id`: identifier of an open command encoder.
- `pass_id`: identifier of an open render or compute pass recorder.
- `pipeline_id`: identifier of a previously created pipeline object.
- `bind_group_id`: identifier of a previously created bind group object.
- `slot`: zero-based bind-group slot index.
- `submission_id`: optional host-visible identifier for queue-tracking or correlation.

General rules:

1. ids are chosen by the client and must be unique within the live namespace of their object kind,
2. referencing an unknown or already-destroyed id is a validation error,
3. creation commands define object state but do not themselves submit GPU work,
4. encoder and pass commands are ordered and stateful,
5. omitted optional fields do not imply support for backend-specific defaults unless the command says so.


## Session And Diagnostics

### `HelloRenderer`

Starts protocol negotiation for a fresh connection.

Required fields:

- `cmd`: must be `HelloRenderer`.
- `version`: protocol version requested by the client.

Optional fields:

- `client_name`: human-readable client implementation name.
- `requested_features`: list or bitset of protocol features the client wants enabled.

Semantics:

1. this should be the first client-to-renderer message on a new session,
2. it negotiates protocol compatibility rather than GPU resources,
3. unsupported versions or features may be rejected by `RendererHelloReply` or followed by `Error`.


### `RendererHelloReply`

Returns the renderer-side result of handshake negotiation.

Required fields:

- `cmd`: must be `RendererHelloReply`.
- `version`: protocol version accepted by the renderer.
- `status`: handshake outcome.

Optional fields:

- `renderer_name`: human-readable renderer implementation name.
- `enabled_features`: list or bitset of features accepted by the renderer.

Semantics:

1. a successful reply completes protocol negotiation,
2. a failed reply means later commands are invalid until the client reconnects or renegotiates.


### `Error`

Reports a protocol, validation, or execution failure.

Required fields:

- `cmd`: must be `Error`.
- `code`: stable machine-readable error code.
- `message`: human-readable explanation.

Optional fields:

- `failed_cmd`: command discriminator for the command that failed.
- `failed_id`: object or submission identifier associated with the failure.

Semantics:

1. `Error` is renderer-to-client diagnostic output, not a client request,
2. whether the session remains usable depends on the error class, not on the existence of the message alone.


## Resource Lifecycle

### `CreateBuffer`

Creates a logical GPU buffer object.

Required fields:

- `cmd`: must be `CreateBuffer`.
- `id`: identifier assigned to the new buffer.
- `size`: total buffer capacity in bytes.
- `usage`: allowed buffer usages.

Optional fields:

- `label`: debug label.

Semantics:

1. `size` is the allocation size of the whole buffer, not an upload length,
2. later write, copy, bind, or draw commands must stay within the declared size and usage constraints.


### `DestroyBuffer`

Destroys a previously created buffer.

Required fields:

- `cmd`: must be `DestroyBuffer`.
- `buffer_id`: identifier of the buffer to destroy.

Semantics:

1. no later command may reference the buffer after destruction,
2. destroying an unknown or already-destroyed buffer is a validation error.


### `WriteBuffer`

Uploads client-provided bytes into a buffer.

Required fields:

- `cmd`: must be `WriteBuffer`.
- `buffer_id`: destination buffer.
- `offset`: first byte written in the destination buffer.
- `size`: number of bytes copied from the payload.
- `data`: raw byte payload to upload.

Semantics:

1. `offset + size` must fit in the destination buffer,
2. `size` must match the payload length,
3. this is host-to-buffer upload, not buffer creation or mapping.


### `CreateTexture`

Creates a logical GPU texture object.

Required fields:

- `cmd`: must be `CreateTexture`.
- `id`: identifier assigned to the new texture.
- `dimension`: texture dimensionality token.
- `width`: width in texels.
- `height`: height in texels.
- `depth`: depth or array-layer count, depending on dimension.
- `format`: texture format token.
- `usage`: allowed texture usages.
- `mip_level_count`: number of mip levels allocated.
- `sample_count`: sample count.

Optional fields:

- `label`: debug label.

Semantics:

1. the tuple `(dimension, width, height, depth)` defines the logical texture extent,
2. `usage`, `format`, `mip_level_count`, and `sample_count` constrain later copies and attachment use.


### `DestroyTexture`

Destroys a previously created texture.

Required fields:

- `cmd`: must be `DestroyTexture`.
- `texture_id`: identifier of the texture to destroy.

Semantics:

1. no later command may reference the texture after destruction.


### `WriteTexture`

Uploads client-provided bytes into a texture subresource.

Required fields:

- `cmd`: must be `WriteTexture`.
- `texture_id`: destination texture.
- `mip_level`: destination mip level.
- `origin`: destination texel origin within the mip level.
- `size`: written extent in texels.
- `bytes_per_row`: source row stride in bytes.
- `rows_per_image`: source image stride in rows.
- `data`: raw byte payload to upload.

Semantics:

1. `origin` and `size` select the destination box within the chosen subresource,
2. `bytes_per_row` and `rows_per_image` describe the source payload layout, not the texture itself,
3. the written region must fit inside the destination subresource.


## Encoder And Pass Lifecycle

### `BeginCommandEncoder`

Opens a command encoder that will collect GPU work for later submission.

Required fields:

- `cmd`: must be `BeginCommandEncoder`.
- `id`: identifier assigned to the new encoder.

Optional fields:

- `label`: debug label.

Semantics:

1. the encoder starts in an open state,
2. pass, copy, and recording commands attach to this encoder until it is finished.


### `FinishCommandEncoder`

Closes an encoder and materializes a submit-ready command buffer.

Required fields:

- `cmd`: must be `FinishCommandEncoder`.
- `encoder_id`: encoder being closed.
- `command_buffer_id`: identifier assigned to the finished command buffer.

Semantics:

1. the encoder must be open and must not contain an unclosed pass,
2. no later command may target the encoder after it is finished,
3. the resulting command buffer becomes eligible for queue submission.


### `BeginRenderPass`

Begins a render pass inside an open command encoder.

Required fields:

- `cmd`: must be `BeginRenderPass`.
- `id`: identifier assigned to the new render-pass recorder.
- `encoder_id`: parent encoder.
- `color_attachments`: ordered list of color attachment descriptors.

Optional fields:

- `depth_stencil_attachment`: depth/stencil attachment descriptor.
- `label`: debug label.

Attachment descriptor semantics:

- `view_id` or equivalent attachment handle: image/view used as the attachment target.
- `load_op`: whether the attachment content is loaded or cleared at pass start.
- `store_op`: whether the final attachment content is stored after the pass.
- `clear_value`: clear color or clear depth/stencil value used when `load_op` is clear.

Pass semantics:

1. the attachment list fixes the framebuffer state for the duration of the pass,
2. later render-state and draw commands target this pass until `EndRenderPass`.


### `EndRenderPass`

Ends the current render pass.

Required fields:

- `cmd`: must be `EndRenderPass`.
- `pass_id`: render pass being closed.

Semantics:

1. the pass must be the currently open render pass for its encoder,
2. no later render-state or draw command may target the pass after it ends.


### `BeginComputePass`

Begins a compute pass inside an open command encoder.

Required fields:

- `cmd`: must be `BeginComputePass`.
- `id`: identifier assigned to the new compute-pass recorder.
- `encoder_id`: parent encoder.

Optional fields:

- `label`: debug label.

Semantics:

1. later pipeline, bind-group, and dispatch commands target this pass until `EndComputePass`.


### `EndComputePass`

Ends the current compute pass.

Required fields:

- `cmd`: must be `EndComputePass`.
- `pass_id`: compute pass being closed.

Semantics:

1. the pass must be the currently open compute pass for its encoder.


## Recording Commands

### `SetPipeline`

Binds the pipeline used by subsequent draw or dispatch commands in the current pass.

Required fields:

- `cmd`: must be `SetPipeline`.
- `pass_id`: target render or compute pass.
- `pipeline_id`: pipeline to bind.

Semantics:

1. the pipeline type must match the pass type,
2. rebinding replaces the previously bound pipeline for later commands in the same pass.


### `SetBindGroup`

Binds a bind group for subsequent pipeline execution.

Required fields:

- `cmd`: must be `SetBindGroup`.
- `pass_id`: target render or compute pass.
- `slot`: bind-group slot index.
- `bind_group_id`: bind group to bind.

Optional fields:

- `dynamic_offsets`: dynamic buffer offsets applied to dynamic bindings.

Semantics:

1. `slot` is interpreted against the currently bound pipeline layout,
2. if present, `dynamic_offsets` are consumed in the binding order defined by the bind group layout.


### `SetViewport`

Sets the viewport transform used by later draw commands in the current render pass.

Required fields:

- `cmd`: must be `SetViewport`.
- `pass_id`: target render pass.
- `x`: viewport origin x in framebuffer coordinates.
- `y`: viewport origin y in framebuffer coordinates.
- `width`: viewport width.
- `height`: viewport height.
- `min_depth`: minimum depth range value.
- `max_depth`: maximum depth range value.

Semantics:

1. this affects rasterization for subsequent draws only,
2. it has no effect in compute passes.


### `SetScissor`

Sets the scissor rectangle used by later draw commands in the current render pass.

Required fields:

- `cmd`: must be `SetScissor`.
- `pass_id`: target render pass.
- `x`: left edge in framebuffer coordinates.
- `y`: top edge in framebuffer coordinates.
- `width`: rectangle width.
- `height`: rectangle height.

Semantics:

1. fragments outside the rectangle are discarded for subsequent draws.


### `SetBlendConstant`

Sets the constant blend color used by blend states that reference it.

Required fields:

- `cmd`: must be `SetBlendConstant`.
- `pass_id`: target render pass.
- `color`: four-component blend constant.

Semantics:

1. this matters only for pipelines whose blend state uses the constant blend color.


### `SetStencilReference`

Sets the stencil reference value used by later draw commands.

Required fields:

- `cmd`: must be `SetStencilReference`.
- `pass_id`: target render pass.
- `reference`: stencil reference integer.

Semantics:

1. this matters only for pipelines with stencil testing enabled.


### `Draw`

Encodes a non-indexed draw call.

Required fields:

- `cmd`: must be `Draw`.
- `pass_id`: target render pass.
- `vertex_count`: number of vertices per instance.
- `instance_count`: number of instances.
- `first_vertex`: starting vertex.
- `first_instance`: starting instance.

Semantics:

1. all counts apply to the currently bound render pipeline and resources,
2. clients should serialize counts explicitly rather than relying on implicit defaults.


### `DrawIndexed`

Encodes an indexed draw call.

Required fields:

- `cmd`: must be `DrawIndexed`.
- `pass_id`: target render pass.
- `index_count`: number of indices per instance.
- `instance_count`: number of instances.
- `first_index`: starting index.
- `base_vertex`: signed vertex offset added to each fetched index.
- `first_instance`: starting instance.

Semantics:

1. indexed draws additionally depend on the currently bound index-buffer state.


### `DispatchWorkgroups`

Encodes a compute dispatch.

Required fields:

- `cmd`: must be `DispatchWorkgroups`.
- `pass_id`: target compute pass.
- `x`: workgroup count in the x dimension.
- `y`: workgroup count in the y dimension.
- `z`: workgroup count in the z dimension.

Semantics:

1. these are workgroup counts, not thread counts,
2. clients should serialize all dimensions explicitly.


## Copy And Submission

### `CopyBufferToBuffer`

Encodes a buffer-to-buffer copy.

Required fields:

- `cmd`: must be `CopyBufferToBuffer`.
- `encoder_id`: target command encoder.
- `src_buffer_id`: source buffer.
- `src_offset`: source byte offset.
- `dst_buffer_id`: destination buffer.
- `dst_offset`: destination byte offset.
- `size`: copied byte count.

Semantics:

1. both source and destination ranges must fit in their respective buffers,
2. copy compatibility is constrained by the usages declared at buffer creation time.


### `CopyBufferToTexture`

Encodes a buffer-to-texture copy.

Required fields:

- `cmd`: must be `CopyBufferToTexture`.
- `encoder_id`: target command encoder.
- `src_buffer_id`: source buffer.
- `src_offset`: first source byte.
- `bytes_per_row`: source row stride in bytes.
- `rows_per_image`: source image stride in rows.
- `dst_texture_id`: destination texture.
- `dst_mip_level`: destination mip level.
- `dst_origin`: destination texel origin.
- `size`: copied extent in texels.

Semantics:

1. the layout fields describe how texel data is packed in the source buffer,
2. the destination box must fit inside the selected texture subresource.


### `CopyTextureToBuffer`

Encodes a texture-to-buffer copy.

Required fields:

- `cmd`: must be `CopyTextureToBuffer`.
- `encoder_id`: target command encoder.
- `src_texture_id`: source texture.
- `src_mip_level`: source mip level.
- `src_origin`: source texel origin.
- `size`: copied extent in texels.
- `dst_buffer_id`: destination buffer.
- `dst_offset`: first destination byte.
- `bytes_per_row`: destination row stride in bytes.
- `rows_per_image`: destination image stride in rows.

Semantics:

1. the source box must fit inside the selected texture subresource,
2. the layout fields describe how texel data is packed in the destination buffer.


### `QueueSubmit`

Submits one or more finished command buffers to the execution queue.

Required fields:

- `cmd`: must be `QueueSubmit`.
- `command_buffer_ids`: ordered list of finished command buffers to submit.

Optional fields:

- `submission_id`: host-visible identifier for tracking or correlation.

Semantics:

1. command buffers execute in listed order within the submission,
2. every referenced command buffer must already have been produced by `FinishCommandEncoder`,
3. submission transfers work from recorded state to queue execution.
