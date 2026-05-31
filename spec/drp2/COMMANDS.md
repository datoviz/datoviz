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
7. `CreateSampler`
8. `DestroySampler`
9. `CreateTextureView`
10. `DestroyTextureView`


### Resource Group Lifecycle

1. `CreateBindGroup`
2. `DestroyBindGroup`
3. `CreateBindGroupLayout`
4. `DestroyBindGroupLayout`


### Pipeline Lifecycle

1. `CreateShaderModule`
2. `DestroyShaderModule`
3. `CreateRenderPipeline`
4. `DestroyRenderPipeline`
5. `CreateComputePipeline`
6. `DestroyComputePipeline`


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
12. `ResourceBarrier`


### Copy And Submission

1. `CopyBufferToBuffer`
2. `CopyBufferToTexture`
3. `CopyTextureToBuffer`
4. `CopyTextureToTexture`
5. `QueueSubmit`
6. `QueueSubmitReply`


## Deferred And Non-Authoritative Commands

The following command names exist in the schema tree as deferred design material only. They are not part
of the active DRP2 contract and must not be treated as authoritative until promoted into this document.

1. `CreatePipelineLayout`
2. `DestroyPipelineLayout`
3. `DispatchWorkgroupsIndirect`
4. `DrawIndirect`
5. `DrawIndexedIndirect`


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
- `submission_id`: host-visible identifier for queue-tracking or correlation; required on `QueueSubmit` when `readbacks` is non-empty.
- `shader_module_id`: identifier of an existing shader module.
- `vertex_shader_module_id`: identifier of an existing vertex-stage shader module.
- `fragment_shader_module_id`: identifier of an existing fragment-stage shader module.
- `compute_shader_module_id`: identifier of an existing compute-stage shader module.
- `entry_point`: shader entry-point name inside the chosen module.
- `format`: shader-source or shader-binary format token.
- `required_features`: explicit runtime features required by a shader module.

General rules:

1. ids are chosen by the client and must be unique within the live namespace of their object kind,
2. referencing an unknown or already-destroyed id is a validation error,
3. creation commands define object state but do not themselves submit GPU work,
4. resource, bind-group, shader, pipeline, upload, and copy commands must not be recorded inside a
   render or compute pass scope,
5. encoder and pass commands are ordered and stateful,
6. omitted optional fields do not imply support for backend-specific defaults unless the command says so.


## Session And Diagnostics

### `HelloRenderer`

Starts protocol negotiation for a fresh connection.

Required fields:

- `cmd`: must be `HelloRenderer`.
- `version`: protocol version object requested by the client.

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
- `version`: protocol version object accepted by the renderer.
- `status`: handshake outcome, currently `ok` or `error`.

Optional fields:

- `renderer_name`: human-readable renderer implementation name.
- `enabled_features`: list or bitset of features accepted by the renderer.

Semantics:

1. `status = ok` completes protocol negotiation,
2. `status = error` means later commands are invalid until the client reconnects or renegotiates.


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
2. `Error` is valid only after `HelloRenderer` has established the stream,
3. `Error` alone does not change session readiness; a `ready` session remains usable unless another
   command already transitioned it to `failed`,
4. `Error` may still appear after a failed handshake or other terminal validation failure as
   additional diagnostic output.


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

The C stream API carries `format` as a `VkFormat` value. Convenience helpers that do not take an
explicit format emit `rgba8unorm`; WBOIT-style intermediate targets should use the explicit
format-and-usage helper so RGBA16F accumulation and R16F revealage textures survive serialization and
runtime creation.


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
3. `mip_level` must select an allocated destination subresource,
4. `bytes_per_row` and `rows_per_image` must be consistent with the transfer extent,
5. the written region must fit inside the destination subresource.


### `CreateSampler`

Creates a sampler object that controls how textures are sampled inside shaders.

Required fields:

- `cmd`: must be `CreateSampler`.
- `id`: client-assigned sampler id.

Optional fields:

- `mag_filter`: magnification filter; one of `nearest`, `linear`. Defaults to `nearest`.
- `min_filter`: minification filter; one of `nearest`, `linear`. Defaults to `nearest`.
- `mipmap_filter`: mip-level interpolation filter; one of `nearest`, `linear`. Defaults to `nearest`.
- `address_mode_u`: horizontal wrap mode; one of `clamp-to-edge`, `repeat`, `mirror-repeat`. Defaults to `clamp-to-edge`.
- `address_mode_v`: vertical wrap mode; same tokens. Defaults to `clamp-to-edge`.
- `address_mode_w`: depth wrap mode (3D textures); same tokens. Defaults to `clamp-to-edge`.
- `lod_min_clamp`: minimum level-of-detail value, must be ≥ 0. Defaults to 0.
- `lod_max_clamp`: maximum level-of-detail value, must be ≥ 0. Defaults to backend maximum.
- `compare`: comparison function for shadow/depth samplers; one of `never`, `less`, `equal`,
  `less-equal`, `greater`, `not-equal`, `greater-equal`, `always`. Omit for non-comparison samplers.
- `max_anisotropy`: maximum anisotropy clamping value, integer ≥ 1. Defaults to 1 (isotropic).
- `label`: optional debug name.

Semantics:

1. a sampler describes sampling parameters, it does not own or reference any texture,
2. the sampler is usable as a bind-group resource entry with `binding_type: "sampler"`,
3. omitting optional fields inherits implementation defaults aligned with WebGPU defaults,
4. `compare` is mutually exclusive with filtering modes in most backends — set it only for
   depth-comparison samplers.


### `DestroySampler`

Ends the lifetime of a sampler object.

Required fields:

- `cmd`: must be `DestroySampler`.
- `id`: id of an existing live sampler.

Semantics:

1. the sampler must be live at the time of destruction,
2. any bind group that references this sampler must already be destroyed before `DestroySampler` is
   valid,
3. no later command may reference the destroyed sampler.


### `CreateTextureView`

Creates a view of an existing texture with a specified subresource range, format, and dimension.

Required fields:

- `cmd`: must be `CreateTextureView`.
- `id`: client-assigned identifier for the new texture view.
- `texture_id`: identifier of the live parent texture.
- `format`: texture format for this view; must be compatible with the parent texture format.
- `dimension`: view dimensionality token.
- `aspect`: which aspect of the texture is visible through this view; one of `all`, `depth-only`,
  `stencil-only`.

Optional fields:

- `mip_range`: object with `base` (integer ≥ 0) and `count` (integer ≥ 1) specifying the visible
  mip-level range. Defaults to the full mip range of the parent texture.
- `layer_range`: object with `base` (integer ≥ 0) and `count` (integer ≥ 1) specifying the visible
  array-layer range. Defaults to the full layer range of the parent texture.
- `label`: optional debug name.

Semantics:

1. the parent texture must be live at the time of view creation,
2. `mip_range.base + mip_range.count` must not exceed the parent texture's `mip_level_count`,
3. `layer_range.base + layer_range.count` must not exceed the parent texture's depth (for array and
   cube dimensions),
4. the texture view is a derived protocol object; it does not own the underlying texture storage,
5. the view can be used as a bind-group resource entry with `resource_kind: "texture_view"` and
   `binding_type` of `sampled_texture` or `storage_texture`,
6. the required usage check for the bind-group entry applies to the parent texture, not the view.


### `DestroyTextureView`

Ends the lifetime of a texture view object.

Required fields:

- `cmd`: must be `DestroyTextureView`.
- `id`: identifier of the existing live texture view.

Semantics:

1. the texture view must be live at the time of destruction,
2. any bind group that references this view must already be destroyed before `DestroyTextureView` is
   valid,
3. no later command may reference the destroyed texture view,
4. destroying the view does not affect the parent texture.


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
- `binding_type`: one of `uniform_buffer`, `storage_buffer`, `sampled_texture`, `storage_texture`,
  or `sampler`.
- `resource_kind`: one of `buffer`, `texture`, `texture_view`, or `sampler`.
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
6. if the referenced bind-group layout marks a buffer binding with `has_dynamic_offset = true`, the
   corresponding bind-group entry must provide both `offset` and `size`,
7. buffer-backed entry ranges must fit within the referenced buffer when `offset` and `size` are
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

- `has_dynamic_offset`: whether a buffer binding consumes one entry from `SetBindGroup.dynamic_offsets`.
- `visibility`: shader stages that may access the binding. If omitted, defaults are:
  - sampled textures and samplers: `FRAGMENT`,
  - uniform buffers: `VERTEX`, `FRAGMENT`, and `COMPUTE`,
  - storage buffers and storage textures: `FRAGMENT` and `COMPUTE`.
- `access`: storage binding access mode, either `read` or `read_write`. It applies only to
  `storage_buffer` and `storage_texture` entries and defaults to `read_write`.
- `label`: debug label.

Semantics:

1. active DRP2 `2.0` bind-group layouts are intentionally minimal and validate binding number plus
   binding type only,
2. binding indices within one layout must be unique,
3. `has_dynamic_offset` may be set only on `uniform_buffer` or `storage_buffer` bindings,
4. `visibility` must include every shader stage that statically accesses the binding and may not
   include stages where the backend rejects the declared binding class,
5. storage bindings declared with `access = read_write` must not be used from shader stages or
   language declarations that only allow read-only storage,
6. bind groups created from the layout must provide exactly the declared binding set.


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

### `CreateShaderModule`

Creates a logical shader-module object.

Required fields:

- `cmd`: must be `CreateShaderModule`.
- `id`: identifier assigned to the new shader module.
- `stage`: shader stage implemented by the module.
- `format`: shader format token.
- `entry_point`: non-empty entry-point name.
- `code`: shader payload encoded as UTF-8 source text or base64 binary, depending on `format`.

Optional fields:

- `required_features`: explicit feature requirements such as `fp64`.
- `builtin_family`: portable identity for a built-in shader family emitted by Datoviz.
- `builtin_variant`: portable identity for a built-in shader variant emitted by Datoviz.
- `builtin_version`: version for the built-in shader identity.
- `label`: debug label.

Semantics:

1. active DRP2 `2.0` accepts WGSL in the portable core contract,
2. GLSL and SPIR-V remain capability-gated ingestion paths rather than always-available contract
   paths,
3. the module stage is fixed at creation time and later pipeline commands must use it consistently,
4. `entry_point` names the callable entry in the provided module payload,
5. `required_features` declares execution requirements that must be checked during capability
   validation before backend execution.


### `DestroyShaderModule`

Destroys a previously created shader module.

Required fields:

- `cmd`: must be `DestroyShaderModule`.
- `shader_module_id`: identifier of the shader module to destroy.

Semantics:

1. no later pipeline may reference the shader module after destruction,
2. destroying a shader module still referenced by a live pipeline or by recorded/submitted work is
   invalid.

### `CreateRenderPipeline`

Creates a render pipeline object with explicit shader attachments, fixed-function state, and
validation-relevant vertex-input requirements.

Required fields:

- `cmd`: must be `CreateRenderPipeline`.
- `id`: identifier assigned to the new render pipeline.
- `vertex_shader_module_id`: vertex-stage shader module used by the pipeline.
- `fragment_shader_module_id`: fragment-stage shader module used by the pipeline.
- `vertex_buffer_slots`: number of contiguous vertex-buffer slots required by later draw commands.

Optional fields:

- `bind_group_layout_ids`: ordered list of bind-group layouts expected by slot.
- `builtin_pipeline`: portable identity for a built-in pipeline emitted by Datoviz.
- `builtin_version`: version for the built-in pipeline identity.
- `vertex_buffers`: ordered list of vertex buffer layout descriptors, one per slot in
  `[0, vertex_buffer_slots)`. If present, its length must equal `vertex_buffer_slots`. If absent,
  vertex input layout is unspecified and the pipeline is expected to use vertex pulling via storage
  buffers or builtins. Each entry requires:
  - `array_stride`: byte distance between successive elements in the buffer.
  - `attributes`: list of vertex attributes sourced from this buffer. Each attribute requires:
    - `format`: vertex attribute format token (e.g. `float32x3`, `unorm8x4`).
    - `offset`: byte offset of the attribute within one stride.
    - `shader_location`: `@location(n)` index in the vertex shader.
  - `step_mode`: `vertex` (advance per vertex) or `instance` (advance per instance). Defaults to
    `vertex`.
- `topology`: primitive assembly mode; one of `point-list`, `line-list`, `line-strip`,
  `triangle-list`, `triangle-strip`. Defaults to `triangle-list`.
- `cull_mode`: face-culling mode; one of `none`, `front`, `back`. Defaults to `none`.
- `front_face`: winding order considered front-facing; one of `ccw`, `cw`. Defaults to `ccw`.
- `sample_count`: MSAA sample count; integer ≥ 1. Defaults to `1`.
- `depth_stencil`: depth and stencil state object. Omit for render passes without a depth/stencil
  attachment.
  - `format` (required): texture format of the depth/stencil attachment.
  - `depth_write_enabled`: whether depth values are written. Defaults to `false`.
  - `depth_compare`: comparison function for depth testing. Defaults to `always`.
  - `stencil_front`: stencil state for front faces; object with optional `compare`,
    `fail_op`, `depth_fail_op`, `pass_op`. All default to `always` / `keep`.
  - `stencil_back`: stencil state for back faces; same shape as `stencil_front`.
- `color_targets`: per-attachment color write and blend state; array of objects.
  - `format` (required per target): texture format of the color attachment.
  - `blend`: blend state object with required `color` and `alpha` sub-objects, each with
    required `src_factor`, `dst_factor`, and `operation`. Omit for no blending.
  - `write_mask`: array of channels to write; each element one of `red`, `green`, `blue`, `alpha`.
    Empty array means no writes. Defaults to all four channels.
- `label`: debug label.

Semantics:

1. active DRP2 `2.0` render pipelines bind one explicit vertex shader and one explicit fragment
   shader,
2. `vertex_shader_module_id` must reference a live shader module whose declared stage is `VERTEX`,
3. `fragment_shader_module_id` must reference a live shader module whose declared stage is
   `FRAGMENT`,
4. `vertex_buffer_slots` defines the required slot range `[0, vertex_buffer_slots)` for later draws
   using this pipeline,
5. if present, `bind_group_layout_ids[slot]` defines the bind-group layout expected by
   `SetBindGroup(slot, ...)`,
6. if `vertex_buffers` is present, its length must equal `vertex_buffer_slots`; if absent, vertex
   input layout is unspecified and vertex pulling is assumed. Portable producers should provide
   `vertex_buffers` whenever the vertex shader declares user vertex-input locations,
7. omitting `topology` is equivalent to `triangle-list`,
8. omitting `depth_stencil` means no depth or stencil testing is performed; the pipeline must not
   be used in a render pass whose `depth_stencil_attachment` requires depth writes or comparison,
9. omitting `color_targets` means the pipeline targets one color attachment with no blending and
   full write mask. The default format is backend-selected from the render target: browser WebGPU
   uses the configured canvas format for canvas targets and `rgba8unorm` for ordinary color
   textures unless a target format is known from the referenced attachment,
10. if `color_targets` is present, its length should match the number of color attachments declared
    in the render pass.


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
- `compute_shader_module_id`: compute-stage shader module used by the pipeline.

Optional fields:

- `bind_group_layout_ids`: ordered list of bind-group layouts expected by slot.
- `label`: debug label.

Semantics:

1. active DRP2 `2.0` compute pipelines bind one explicit compute shader module,
2. `compute_shader_module_id` must reference a live shader module whose declared stage is
   `COMPUTE`,
3. if present, `bind_group_layout_ids[slot]` defines the bind-group layout expected by
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
  Depth/stencil attachments may use `0` for an implementation-owned transient depth attachment.
- `resolve_target_texture_id`: optional live texture used as the resolve target in active DRP2 `2.0`.
- `load_op`: whether the attachment content is loaded or cleared at pass start.
- `store_op`: whether the final attachment content is stored after the pass.
- `clear_value`: clear color used when a color attachment `load_op` is clear.
- `depth_load_op`: whether depth content is loaded or cleared at pass start.
- `depth_store_op`: whether final depth content is stored after the pass.
- `depth_clear_value`: clear depth value used when `depth_load_op` is clear.

Active `2.0` note:

1. render-pass attachments reference textures directly via `texture_id`,
2. runtimes may derive backend-native attachment views from the referenced texture using implicit
   full-subresource selection,
3. texture view objects (`CreateTextureView`) are used for bind-group bindings, not render-pass
   attachments.

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

1. the pass must be the currently open compute pass for its encoder,
2. no later bind-group or dispatch command may target the pass after it ends.


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
3. rebinding replaces the previously bound pipeline for later commands in the same pass,
4. later `SetBindGroup`, `Draw`, `DrawIndexed`, or `DispatchWorkgroups` validation uses the most
   recently bound pipeline in that pass,
5. runtimes may discard previously cached bind-group compatibility state when a new pipeline is
   bound.


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
5. if the bind-group layout declares dynamic buffer bindings, `dynamic_offsets` must be present,
6. `dynamic_offsets` must contain exactly one offset for each layout entry whose
   `has_dynamic_offset` is true,
7. `dynamic_offsets` are consumed in the layout entry order declared by `CreateBindGroupLayout`,
8. each consumed dynamic offset is added to the corresponding bind-group entry's base `offset`
   before validating the referenced buffer range,
9. each final dynamic buffer offset must satisfy the backend's binding-offset alignment requirement.
   Backends may materialize an aligned temporary binding when the stream is otherwise valid and the
   visible range is representable,
10. if the bind-group layout declares no dynamic buffer bindings, `dynamic_offsets` must be omitted or
   empty,
11. if a new pipeline is rebound earlier in the same pass, `SetBindGroup` is interpreted against the
    newly bound pipeline rather than any earlier pipeline.


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


### `ResourceBarrier`

Records an ordered buffer resource-state marker inside an open command encoder.

Required fields:

- `cmd`: must be `ResourceBarrier`.
- `encoder_id`: target command encoder.
- `buffer_id`: buffer whose writes must be made visible to a later consumer.
- `src_stage`: producer stage. Active `2.0` supports `COMPUTE`.
- `src_access`: producer access. Active `2.0` supports `STORAGE_WRITE`.
- `dst_stage`: consumer stage. Active `2.0` supports `VERTEX_INPUT` and `COPY`.
- `dst_access`: consumer access. Active `2.0` supports `VERTEX_READ` with `VERTEX_INPUT` and
  `COPY_READ` with `COPY`.

Optional fields:

- `offset`: byte offset into `buffer_id`, default `0`.
- `size`: byte count. `0` or omission means the remainder of the buffer.

Semantics:

1. this command is valid only inside an open command encoder and outside render/compute pass scopes,
2. `buffer_id` must reference a live buffer whose usage includes `STORAGE`,
3. `COMPUTE` / `STORAGE_WRITE` -> `VERTEX_INPUT` / `VERTEX_READ` additionally requires `VERTEX`
   usage on the buffer,
4. `COMPUTE` / `STORAGE_WRITE` -> `COPY` / `COPY_READ` additionally requires `COPY_SRC` usage on
   the buffer,
5. native Vulkan runtimes map the marker to an explicit buffer memory barrier,
6. WebGPU runtimes may treat the marker as a validated no-op when pass order already provides the
   required visibility.


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
2. `src_mip_level` must select an allocated source subresource,
3. `bytes_per_row` and `rows_per_image` must be consistent with the copied extent,
4. the layout fields describe how texel data is packed in the destination buffer,
5. backends whose copy command requires stricter row-pitch alignment may use an aligned temporary
   copy buffer and compact the requested rows into the destination layout described by
   `bytes_per_row`.


### `CopyTextureToTexture`

Encodes a texture-to-texture copy.

Required fields:

- `cmd`: must be `CopyTextureToTexture`.
- `encoder_id`: target command encoder.
- `src_texture_id`: source texture.
- `src_mip_level`: source mip level.
- `src_origin`: source texel origin.
- `dst_texture_id`: destination texture.
- `dst_mip_level`: destination mip level.
- `dst_origin`: destination texel origin.
- `size`: copied extent in texels.

Semantics:

1. `src_mip_level` and `dst_mip_level` must each select an allocated subresource,
2. the source box `(src_origin, size)` must fit inside the source subresource,
3. the destination box `(dst_origin, size)` must fit inside the destination subresource,
4. the source texture must have `COPY_SRC` usage and the destination must have `COPY_DST` usage,
5. source and destination textures may be the same object only if the subresource ranges do not
   overlap,
6. this command must not be issued inside an open render or compute pass.


### `QueueSubmit`

Submits one or more finished command buffers to the execution queue.

Required fields:

- `cmd`: must be `QueueSubmit`.
- `command_buffer_ids`: ordered list of finished command buffers to submit.

Optional fields:

- `submission_id`: host-visible identifier for tracking or correlation; required when `readbacks` is non-empty.
- `readbacks`: list of buffer ranges the runtime must copy back to the producer after submitted work completes.

Each `readbacks` entry:

- `buffer_id`: live buffer to read back; must have `MAP_READ` usage.
- `offset`: byte offset into the buffer.
- `size`: byte count to read back; `offset + size` must not exceed the buffer size.

Semantics:

1. command buffers execute in listed order within the submission,
2. every referenced command buffer must already have been produced by `FinishCommandEncoder`,
3. each referenced command buffer must not already have been submitted earlier in the same stream,
4. each command buffer id may appear at most once within a single `QueueSubmit`,
5. submission transfers work from recorded state to queue execution,
6. when `readbacks` is non-empty, the runtime must send a `QueueSubmitReply` after all submitted work affecting the requested buffers has completed,
7. readback ranges are read after GPU work is complete; producers must not assume the data is available before the reply arrives.


### `QueueSubmitReply`

Returns buffer readback data to the producer after a submission with readbacks completes.

Required fields:

- `cmd`: must be `QueueSubmitReply`.
- `submission_id`: must match the `submission_id` of the corresponding `QueueSubmit`.
- `readbacks`: list of readback results, in the same order as the `readbacks` in the originating `QueueSubmit`.

Each `readbacks` entry:

- `buffer_id`: the buffer id from the original readback request.
- `offset`: the byte offset from the original readback request.
- `size`: the byte count from the original readback request.
- `data`: base64-encoded bytes read from the buffer range.

Semantics:

1. `QueueSubmitReply` is a runtime-to-producer message, analogous to `RendererHelloReply`,
2. the runtime must deliver replies in the same order as their corresponding submissions,
3. the `readbacks` list must mirror the original request exactly: same buffer ids, offsets, and sizes in the same order,
4. each `data` field must contain exactly `size` bytes encoded as base64,
5. a `QueueSubmit` with no `readbacks` field or an empty `readbacks` list does not produce a reply.
