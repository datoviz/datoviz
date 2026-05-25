# Draw Resource Validation Plan

## Status

- Status: implemented
- Updated on: 2026-05-25
- Purpose: harden the scene -> DRP2 handoff against mismatches between draw counts and bound GPU
  resource contents.


## Problem

The retained scene model currently keeps logical visual counts, resource byte capacity, and emitted
DRP2 draw commands in related but separate places. Dense visual updates validate per-item attribute
counts, and external scene-buffer bindings validate their declared range, but the final draw emission
path can still bind resource ids and emit `Draw` or `DrawIndexed` without a single final contract
that proves every bound resource covers the draw range.

This means a false dirty state, stale derived visual cache, external buffer registration mismatch, or
resource recreation bug can become a valid-looking command stream that draws past the intended
logical vertex data. Vulkan validation will not necessarily catch this class because the buffer can be
large enough while the logical payload is stale, short, or from a different visual state.


## Goals

1. Reject scene-generated command streams before DRP2 emission when draw counts exceed any required
   bound vertex, instance, or index resource.
2. Keep logical item count separate from byte capacity in frame-plan/emitter resource state.
3. Make diagnostics deterministic: mismatches should produce scene or DRP2 validation errors, not
   visual corruption.
4. Preserve the current DRP2 runtime boundary; do not add Vulkan-specific escape hatches to scene.


## Phased Plan

### Phase 1: Scene-Side Guardrail

Add validation near visual descriptor resolution or render emission, before `SetVertexBuffer` and
`Draw` commands are appended.

Required checks:

1. for every per-vertex buffer in a `DvzSceneVisualDesc`, require
   `first_vertex + vertex_count <= logical_item_count`,
2. when only byte size is available, conservatively derive `logical_item_count = byte_size / stride`
   and reject zero or incomplete stride divisions,
3. for indexed draws, require `index_count <= logical_index_count`,
4. for instanced draws, require every per-instance binding to cover `instance_count`,
5. emit a diagnostic that names the visual kind, resource role, draw count, logical count, and
   resource id.

Expected scope: `src/scene/visual_desc.c`, `src/scene/runtime_render_emit.c`, focused scene tests.


### Phase 2: Logical Resource Counts

Extend frame-plan/emitter resource metadata so resources carry logical item counts alongside byte
capacity.

Rules:

1. uploads created from retained visual attributes set `logical_item_count` from the visual attr,
2. derived family caches set logical counts from generated vertex/index cache counts,
3. external buffers set logical counts from the scene binding contract, not from runtime buffer size,
4. buffer capacity may grow without changing logical count,
5. partial uploads update bytes but do not change logical count unless the owning retained attr was
   resized through a full update.

Expected scope: frame-plan upload metadata, runtime emitter resource table, visual upload helpers,
descriptor resolution tests.


### Phase 3: DRP2 Semantic Validation

Teach DRP2 validation enough vertex-input state to reject malformed command streams even when they
do not originate in scene.

Required state:

1. render pipeline binding strides, step modes, and attribute-to-binding mapping,
2. currently bound vertex buffers and byte offsets per render pass,
3. currently bound index buffer, index format, and byte offset,
4. buffer sizes from `CreateBuffer` or external runtime registration.

Validation rules:

1. `Draw` requires all required vertex bindings to cover `first_vertex + vertex_count`,
2. instance-step bindings must cover `first_instance + instance_count`,
3. `DrawIndexed` requires the index buffer to cover the requested index range,
4. validation failure must prevent backend execution.

Expected scope: DRP2 semantic state, pipeline creation validation, set-buffer validation, draw
validation, DRP2 tests.


### Phase 4: Draw Packet Refactor

Once the guardrails are in place, consolidate scene render emission around a single validated draw
packet.

The packet should contain:

1. visual identity and visual family,
2. pipeline id and draw contract,
3. ordered vertex buffers with roles, strides, step modes, and logical counts,
4. optional index buffer with format and logical count,
5. `first_vertex`, `vertex_count`, `first_instance`, `instance_count`, and `index_count`,
6. bind groups and pass-local clip state.

The packet becomes the only path that can emit draw commands from scene. Validation happens before
lowering to DRP2 commands, and tests assert both accepted packets and rejected mismatches.

Implemented on 2026-05-25:

1. retained scene visual draws now prepare `SceneDrawPacket` instances before lowering to DRP2,
2. WGSL/fallback scene visual draws use the same packet initializer and lowering helper,
3. fullscreen/effect helper triangles remain outside the visual packet path,
4. marker and indexed primitive tests assert packet-sensitive vertex/index binding behavior.


## Recommended Order

Implement Phase 1 first, then add the logical count pieces from Phase 2 where the guardrail needs
more precise data. Phase 3 should follow as a DRP2 hardening pass. Phase 4 is valuable cleanup, but
it should not block the first guardrail because the smaller validation layer already prevents the
class of visible corruption that motivated this plan.


## Acceptance Tests

1. A point visual with a stale or undersized position resource is rejected before `Draw`.
2. A primitive visual whose metadata vertex count exceeds the position buffer logical count is
   rejected with a diagnostic.
3. A partial update after a full allocation preserves logical count and emits only the dirty byte
   range.
4. A resource capacity growth does not increase draw count unless retained visual metadata changes.
5. DRP2 semantic validation rejects a hand-authored stream whose `Draw` exceeds bound vertex buffer
   capacity.
6. Existing axis, text, image, segment/path, mesh, and external-buffer tests continue to pass.
