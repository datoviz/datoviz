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


### Resource Group Lifecycle

1. `CreateBindGroup`
2. `DestroyBindGroup`
3. `CreateBindGroupLayout`
4. `DestroyBindGroupLayout`


### Pipeline Lifecycle

1. `CreateRenderPipeline`
2. `DestroyRenderPipeline`
3. `CreateComputePipeline`
4. `DestroyComputePipeline`


### Encoder And Pass Lifecycle

1. `BeginCommandEncoder`
2. `FinishCommandEncoder`
3. `BeginRenderPass`
4. `EndRenderPass`
5. `BeginComputePass`
6. `EndComputePass`


### Recording Commands

1. `SetPipeline`
2. `SetVertexBuffer`
3. `SetIndexBuffer`
4. `SetBindGroup`
5. `SetViewport`
6. `SetScissor`
7. `SetBlendConstant`
8. `SetStencilReference`
9. `Draw`
10. `DrawIndexed`
11. `DispatchWorkgroups`


### Copy And Submission

1. `CopyBufferToBuffer`
2. `CopyBufferToTexture`
3. `CopyTextureToBuffer`
4. `QueueSubmit`


## Deferred And Non-Authoritative Commands

The following command names exist in the schema tree as deferred design material only. They are not part
of the active DRP2 contract and must not be treated as authoritative until promoted into this document.

1. `CreatePipelineLayout`
2. `DestroyPipelineLayout`
3. `CreateShaderModule`
4. `DestroyShaderModule`
5. `CreateSampler`
6. `DestroySampler`
7. `CreateTextureView`
8. `DestroyTextureView`
9. `ResourceBarrier`
10. `DispatchWorkgroupsIndirect`
11. `DrawIndirect`
12. `DrawIndexedIndirect`


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
- `render_pipeline_id`: identifier of an existing render pipeline.
- `compute_pipeline_id`: identifier of an existing compute pipeline.
- `bind_group_id`: identifier of a previously created bind group object.
- `slot`: zero-based bind-group slot index.
- `index_format`: index element format token for indexed draws.
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


## Resource Group Lifecycle

### `CreateBindGroup`

Creates a lightweight bind-group object that captures references to existing resources.

Required fields:

- `cmd`: must be `CreateBindGroup`.
- `id`: identifier assigned to the new bind group.
- `bind_group_layout_id`: layout that defines the expected entry set.
- `entries`: ordered list of resource-binding entries.

Each entry requires:

- `binding`: numeric binding index within the bind group.
- `binding_type`: one of `uniform_buffer`, `storage_buffer`, `sampled_texture`, or
  `storage_texture`.
- `resource_kind`: one of `buffer` or `texture`.
- `resource_id`: identifier of the referenced resource object.

Optional per-entry fields:

- `offset`: first visible byte for buffer-backed bindings.
- `size`: visible byte count for buffer-backed bindings.

Optional top-level fields:

- `label`: debug label.

Semantics:

1. active DRP2 `2.0` treats bind groups as lightweight protocol objects rather than full layout-aware
   descriptor sets,
2. `bind_group_layout_id` must reference a live bind-group layout object,
3. each provided entry must match the declared layout entry for the same `binding` index,
4. `binding_type` must be compatible with both `resource_kind` and the referenced resource's declared
   usage bits,
5. each referenced resource must already exist and remain live for the lifetime required by any
   recorded work that captures the bind group,
6. buffer-backed entry ranges must fit within the referenced buffer when `offset` and `size` are
   provided.


### `DestroyBindGroup`

Destroys a previously created bind group.

Required fields:

- `cmd`: must be `DestroyBindGroup`.
- `bind_group_id`: identifier of the bind group to destroy.

Semantics:

1. no later command may bind the bind group after destruction,
2. destroying a bind group still referenced by recorded work is invalid.


### `CreateBindGroupLayout`

Creates a bind-group layout object that defines the expected binding indices and binding types.

Required fields:

- `cmd`: must be `CreateBindGroupLayout`.
- `id`: identifier assigned to the new bind-group layout.
- `entries`: ordered list of layout entries.

Each layout entry requires:

- `binding`: numeric binding index within the bind group.
- `binding_type`: one of `uniform_buffer`, `storage_buffer`, `sampled_texture`, or
  `storage_texture`.

Optional fields:

- `label`: debug label.

Semantics:

1. active DRP2 `2.0` bind-group layouts are intentionally minimal and validate binding number plus
   binding type only,
2. binding indices within one layout must be unique,
3. bind groups created from the layout must provide exactly the declared binding set.


### `DestroyBindGroupLayout`

Destroys a previously created bind-group layout.

Required fields:

- `cmd`: must be `DestroyBindGroupLayout`.
- `bind_group_layout_id`: identifier of the bind-group layout to destroy.

Semantics:

1. no later bind group or pipeline may reference the layout after destruction,
2. destroying a bind-group layout still referenced by live bind groups, pipelines, or recorded work
   is invalid.


## Pipeline Lifecycle

### `CreateRenderPipeline`

Creates a render pipeline object with validation-relevant vertex-input requirements.

Required fields:

- `cmd`: must be `CreateRenderPipeline`.
- `id`: identifier assigned to the new render pipeline.
- `vertex_buffer_slots`: number of contiguous vertex-buffer slots required by later draw commands.

Optional fields:

- `bind_group_layout_ids`: ordered list of bind-group layouts expected by slot.
- `label`: debug label.

Semantics:

1. active DRP2 `2.0` treats render pipelines as lightweight protocol objects rather than full shader
   or layout graphs,
2. `vertex_buffer_slots` defines the required slot range `[0, vertex_buffer_slots)` for later draws
   using this pipeline,
3. if present, `bind_group_layout_ids[slot]` defines the bind-group layout expected by
   `SetBindGroup(slot, ...)`.


### `DestroyRenderPipeline`

Destroys a previously created render pipeline.

Required fields:

- `cmd`: must be `DestroyRenderPipeline`.
- `render_pipeline_id`: identifier of the render pipeline to destroy.

Semantics:

1. no later command may bind the pipeline after destruction,
2. destroying a pipeline still referenced by recorded work is invalid.


### `CreateComputePipeline`

Creates a compute pipeline object.

Required fields:

- `cmd`: must be `CreateComputePipeline`.
- `id`: identifier assigned to the new compute pipeline.

Optional fields:

- `bind_group_layout_ids`: ordered list of bind-group layouts expected by slot.
- `label`: debug label.

Semantics:

1. active DRP2 `2.0` treats compute pipelines as lightweight protocol objects rather than full
   shader or layout graphs,
2. if present, `bind_group_layout_ids[slot]` defines the bind-group layout expected by
   `SetBindGroup(slot, ...)`.


### `DestroyComputePipeline`

Destroys a previously created compute pipeline.

Required fields:

- `cmd`: must be `DestroyComputePipeline`.
- `compute_pipeline_id`: identifier of the compute pipeline to destroy.

Semantics:

1. no later command may bind the pipeline after destruction,
2. destroying a pipeline still referenced by recorded work is invalid.


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

- `texture_id`: live texture used as the attachment target in active DRP2 `2.0`.
- `resolve_target_texture_id`: optional live texture used as the resolve target in active DRP2 `2.0`.
- `load_op`: whether the attachment content is loaded or cleared at pass start.
- `store_op`: whether the final attachment content is stored after the pass.
- `clear_value`: clear color or clear depth/stencil value used when `load_op` is clear.

Active `2.0` note:

1. render-pass attachments reference textures directly,
2. texture views are deferred and non-authoritative in active `2.0`,
3. runtimes may derive backend-native attachment views from the referenced texture plus implicit full-subresource selection.

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

1. `pipeline_id` must reference a live pipeline object,
2. the pipeline type must match the pass type,
3. rebinding replaces the previously bound pipeline for later commands in the same pass.


### `SetVertexBuffer`

Binds a vertex buffer for later draw commands in the current render pass.

Required fields:

- `cmd`: must be `SetVertexBuffer`.
- `pass_id`: target render pass.
- `slot`: zero-based vertex-buffer slot index.
- `buffer_id`: bound vertex buffer.
- `offset`: first byte visible through the binding.

Optional fields:

- `size`: size in bytes visible through the binding.

Semantics:

1. this command is valid only in an open render pass,
2. `buffer_id` must reference a buffer whose declared usage includes `VERTEX`,
3. the binding satisfies the slot requirement for later draws that use the currently bound render
   pipeline.


### `SetIndexBuffer`

Binds the index buffer for later indexed draw commands in the current render pass.

Required fields:

- `cmd`: must be `SetIndexBuffer`.
- `pass_id`: target render pass.
- `buffer_id`: bound index buffer.
- `index_format`: index element format.
- `offset`: first byte visible through the binding.

Optional fields:

- `size`: size in bytes visible through the binding.

Semantics:

1. this command is valid only in an open render pass,
2. `buffer_id` must reference a buffer whose declared usage includes `INDEX`,
3. `DrawIndexed` requires index-buffer state to have been bound earlier in the same open render
   pass.


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

1. `bind_group_id` must reference a live bind-group object,
2. a pipeline must already be bound in the same open pass before the bind group is set,
3. if the bound pipeline declares `bind_group_layout_ids`, the entry at `slot` must exist and match
   the bind group's layout id,
4. `slot` is interpreted against the currently bound pipeline layout,
5. if present, `dynamic_offsets` are consumed in the binding order defined by the bind group layout.


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
2. a render pipeline must already have been bound in the same open render pass,
3. every vertex-buffer slot required by the bound render pipeline must already have been bound in
   the same open render pass,
4. clients should serialize counts explicitly rather than relying on implicit defaults.


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

1. a render pipeline must already have been bound in the same open render pass,
2. every vertex-buffer slot required by the bound render pipeline must already have been bound in
   the same open render pass,
3. indexed draws additionally require an index buffer to have been bound earlier in the same open
   render pass.


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
2. a compute pipeline must already have been bound in the same open compute pass,
3. clients should serialize all dimensions explicitly.


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
3. each referenced command buffer must not already have been submitted earlier in the same stream,
4. submission transfers work from recorded state to queue execution.
