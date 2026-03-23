# DRP2 Lifetimes And State

This document defines the authoritative lifetime, scope, and recorder-state rules for the active
DRP2 `2.0` command surface.

If command prose in [COMMANDS.md](./COMMANDS.md) and this document appear to disagree, treat
`COMMANDS.md` as the field-level source of truth and this document as the source of state and
lifetime invariants.


## Scope

This document applies only to the active DRP2 `2.0` commands:

1. session and diagnostics,
2. buffer and texture lifecycle,
3. command encoder lifecycle,
4. render and compute pass lifecycle,
5. render and compute recording commands,
6. copy commands,
7. queue submission.

Deferred commands listed in `schema/DEFERRED.md` are not covered by these invariants and must not be
assumed to inherit them unchanged.


## Object Identity

General id rules:

1. ids are chosen by the client,
2. ids are typed by the command that creates or defines their scope,
3. a live id must refer to exactly one object kind,
4. referencing an unknown id is invalid,
5. referencing a destroyed id is invalid,
6. reusing an id after destruction is invalid in DRP2 `2.0`,
7. reusing an id for a different object kind is invalid even if the earlier object is no longer live.

Validation consequences:

1. unknown ids should fail with `DRP2_ERR_INVALID_ID`,
2. duplicate or reused ids should fail with `DRP2_ERR_DUPLICATE_ID`,
3. correct id existence with wrong object kind should fail with `DRP2_ERR_WRONG_OBJECT_TYPE`.


## Persistent Versus Scoped Objects

Persistent objects remain live until explicit destruction:

1. buffer
2. texture
3. command buffer

Scoped objects exist only between their begin and end commands:

1. command encoder
2. render pass
3. compute pass

Diagnostic messages are not persistent objects:

1. `HelloRenderer` and `RendererHelloReply` negotiate session compatibility,
2. `Error` reports a failure but does not create a reusable object.


## Session State

Session states:

1. `unnegotiated`
2. `ready`
3. `failed`

Rules:

1. a fresh connection starts in `unnegotiated`,
2. `HelloRenderer` is valid only in `unnegotiated`,
3. a successful `RendererHelloReply` transitions the session to `ready`,
4. a failed handshake transitions the session to `failed`,
5. active resource and recording commands are valid only in `ready`,
6. once the session is `failed`, the stream is not required to recover within the same connection.


## Buffer Lifetime

Rules:

1. `CreateBuffer` makes `id` live as a buffer immediately after semantic validation succeeds,
2. `WriteBuffer` requires a live buffer id,
3. `DestroyBuffer` ends the buffer lifetime immediately after semantic validation succeeds,
4. no later command may reference the destroyed buffer,
5. a buffer may be referenced by copy commands only while live,
6. a buffer may be referenced by `QueueSubmit` only indirectly through previously finished command buffers.


## Texture Lifetime

Rules:

1. `CreateTexture` makes `id` live as a texture immediately after semantic validation succeeds,
2. `WriteTexture` requires a live texture id,
3. `DestroyTexture` ends the texture lifetime immediately after semantic validation succeeds,
4. no later command may reference the destroyed texture,
5. a texture may be referenced by copy commands only while live,
6. texture subresources do not have independent protocol lifetimes in the active `2.0` surface.


## Command Buffer Lifetime

Rules:

1. `FinishCommandEncoder` creates a new command buffer object identified by `command_buffer_id`,
2. a command buffer becomes live only if its parent encoder finishes successfully,
3. `QueueSubmit` requires every referenced command buffer to be live and finished,
4. command buffers are immutable once created,
5. DRP2 `2.0` has no explicit `DestroyCommandBuffer`,
6. runtimes may reclaim backend-native command-buffer resources after submission or stream teardown,
   but that is not a protocol-visible state change.


## Command Encoder State Machine

Encoder states:

1. `open`
2. `finished`

Rules:

1. `BeginCommandEncoder` creates an encoder in `open`,
2. copy commands are valid only while the target encoder is `open`,
3. `BeginRenderPass` and `BeginComputePass` are valid only while the target encoder is `open`,
4. `FinishCommandEncoder` is valid only while the target encoder is `open`,
5. `FinishCommandEncoder` transitions the encoder to `finished`,
6. no later command may target a `finished` encoder,
7. an encoder with an open child pass cannot be finished,
8. an encoder cannot contain more than one open child pass at a time.


## Pass State Machine

Render-pass states:

1. `open`
2. `ended`

Compute-pass states:

1. `open`
2. `ended`

Rules:

1. `BeginRenderPass` creates a render pass in `open`,
2. `BeginComputePass` creates a compute pass in `open`,
3. `EndRenderPass` transitions the referenced render pass to `ended`,
4. `EndComputePass` transitions the referenced compute pass to `ended`,
5. no later command may target an ended pass,
6. a pass must belong to exactly one encoder,
7. a second pass may not begin in an encoder while the first is still open,
8. a pass must be the currently open pass of its encoder when it is ended.


## Command Validity By Scope

Commands valid without encoder or pass scope:

1. `HelloRenderer`
2. `RendererHelloReply`
3. `Error`
4. `CreateBuffer`
5. `DestroyBuffer`
6. `WriteBuffer`
7. `CreateTexture`
8. `DestroyTexture`
9. `WriteTexture`
10. `BeginCommandEncoder`
11. `QueueSubmit`

Commands valid in an open encoder but outside any pass:

1. `BeginRenderPass`
2. `BeginComputePass`
3. `FinishCommandEncoder`
4. `CopyBufferToBuffer`
5. `CopyBufferToTexture`
6. `CopyTextureToBuffer`

Commands valid only in an open render pass:

1. `SetPipeline` with a render pipeline
2. `SetBindGroup`
3. `SetViewport`
4. `SetScissor`
5. `SetBlendConstant`
6. `SetStencilReference`
7. `Draw`
8. `DrawIndexed`
9. `EndRenderPass`

Commands valid only in an open compute pass:

1. `SetPipeline` with a compute pipeline
2. `SetBindGroup`
3. `DispatchWorkgroups`
4. `EndComputePass`

Additional rules:

1. draw commands are invalid outside an open render pass,
2. dispatch commands are invalid outside an open compute pass,
3. copy commands are invalid inside any pass,
4. `SetViewport`, `SetScissor`, `SetBlendConstant`, and `SetStencilReference` are invalid in compute
   passes,
5. `EndRenderPass` is invalid for a compute pass id,
6. `EndComputePass` is invalid for a render pass id.


## Pipeline And Pass Compatibility

Rules:

1. `SetPipeline` requires a live pipeline id,
2. the bound pipeline kind must match the target pass kind,
3. a draw command requires a render pipeline to have been bound earlier in the same open render pass,
4. a dispatch command requires a compute pipeline to have been bound earlier in the same open
   compute pass,
5. rebinding a pipeline replaces the previously bound pipeline for subsequent commands in the same
   pass,
6. `SetBindGroup` is interpreted against the currently bound pipeline layout,
7. validation may reject `SetBindGroup` immediately if no pipeline is currently bound and the runtime
   cannot validate the slot against a known layout.


## Submission And Destruction Safety

The active `2.0` contract distinguishes between recording-time references and already-submitted work.

Rules:

1. an object referenced by an open encoder or open pass cannot be destroyed,
2. a command buffer referenced by a `QueueSubmit` is consumed as immutable recorded work,
3. destroying a resource that is referenced by a finished but not yet submitted command buffer is
   invalid,
4. destroying a resource that is referenced by already submitted work is invalid unless the runtime
   explicitly defines completion tracking beyond the active `2.0` contract,
5. because active `2.0` has no fence or completion primitive, clients should conservatively treat
   submitted work as still using its referenced resources for the remainder of the stream,
6. destroying an already-destroyed object is invalid,
7. destroying an object of the wrong kind is invalid.

Validation consequences:

1. state violations should usually fail with `DRP2_ERR_INVALID_STATE`,
2. active-use destruction should usually fail with `DRP2_ERR_USAGE` or `DRP2_ERR_INVALID_STATE`,
3. pass-kind mismatches should fail with `DRP2_ERR_PASS_MISMATCH`.


## Range And Layout Invariants

These rules supplement the per-command field semantics in `COMMANDS.md`.

1. `WriteBuffer` requires `offset + size` to fit inside the target buffer,
2. `CopyBufferToBuffer` requires both source and destination ranges to fit inside their buffers,
3. `CopyBufferToTexture` requires the destination box to fit inside the selected texture subresource,
4. `CopyTextureToBuffer` requires the source box to fit inside the selected texture subresource,
5. `WriteTexture` requires the written box to fit inside the selected texture subresource,
6. `bytes_per_row` and `rows_per_image` describe payload layout and do not change object lifetime or
   resource shape,
7. schema-valid commands may still fail semantic validation if ids, ranges, usage, or pass state are
   wrong.


## Invalid Sequence Examples

The following sequences are invalid in active DRP2 `2.0`.

### Draw Outside A Render Pass

1. `BeginCommandEncoder`
2. `Draw`

Reason:

`Draw` requires an open render pass.


### Finish Encoder With Open Pass

1. `BeginCommandEncoder`
2. `BeginRenderPass`
3. `FinishCommandEncoder`

Reason:

an encoder cannot finish while a child pass is still open.


### Copy Inside A Pass

1. `BeginCommandEncoder`
2. `BeginComputePass`
3. `CopyBufferToBuffer`

Reason:

copy commands are valid only at encoder scope, not inside a render or compute pass.


### Destroy Resource Still Referenced By Recorded Work

1. `CreateBuffer`
2. `BeginCommandEncoder`
3. `CopyBufferToBuffer`
4. `FinishCommandEncoder`
5. `DestroyBuffer`

Reason:

the finished command buffer still refers to the buffer, so destruction is invalid before that work is
considered no longer in use by the protocol.


## Validation Consequences

Lifetime and state violations should fail during semantic validation whenever possible.

They should not be deferred to backend execution unless the violation depends on backend-only facts
that are not visible at the DRP2 contract layer.
