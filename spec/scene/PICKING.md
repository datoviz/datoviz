# Scene Picking

This document defines how picking should work in the future scene layer.

Picking is a scene-side semantic feature.

It is not just a backend readback trick, and it is not merely a visual-family implementation detail.


## Purpose

The picking model should:

1. let the user identify scene objects from panel-local interaction,
2. preserve visual, item, and group identity across the execution boundary,
3. remain compatible with batched rendering,
4. work across ordinary visuals, grouped visuals, and panel-local overlays,
5. fit naturally into `FramePlan` and readback planning.


## Position

Picking sits across:

1. panel-local interaction state,
2. visual-family draw contributions,
3. `FramePlan` picking passes and readback nodes,
4. scene-level interpretation of the result.

The intended flow is:

1. the panel receives an interaction request at a position,
2. the scene decides which picking work is required,
3. a picking render/readback path runs if needed,
4. the result is mapped back to scene identities,
5. scene selection or hover state is updated.


## Core Rule

Picking should return scene identity, not backend identity.

The result of a pick should be interpretable in terms of:

1. panel identity,
2. visual identity,
3. family identity,
4. item identity when applicable,
5. group identity when applicable,
6. optional family-defined auxiliary payload.


## Non-Goals

This document does not define:

1. the exact encoded payload format,
2. the exact picking render-target format,
3. the exact latency policy,
4. the exact final API for asynchronous delivery,
5. the exact DRP2 implementation commands.


## Picking Is Panel-Aware

Picking is always panel-local in its request origin.

That means a pick request should be defined by:

1. panel identity,
2. panel-local position,
3. request kind such as hover or click,
4. optional policy such as nearest-hit or exact-hit.

The same visual may appear in multiple panels, so a pick result must always remain anchored to the
requesting panel.


## Picking Is Visual-Aware

Picking should also preserve which visual contribution produced the hit.

This matters because:

1. one panel may contain several visuals from the same family,
2. one logical family may appear with different variants,
3. one family may have multiple semantic hit policies.

So a valid pick result should always be able to report:

1. which visual was hit,
2. which family semantics apply to the result.


## Identity Levels

The picking model should explicitly recognize several identity levels.


### 1. Scene Identity

This is the top-level identity of the owning `Scene`.

It is usually implicit in normal use, but it is still part of the semantic routing boundary.


### 2. Panel Identity

This identifies which panel issued the request and which panel-local view produced the hit.


### 3. Visual Identity

This identifies which visual instance produced the picked contribution.


### 4. Item Identity

This identifies one logical item inside an `ItemTable` or equivalent per-item representation.

Examples:

1. one point,
2. one marker,
3. one segment,
4. one impostor sphere instance.


### 5. Group Identity

This identifies one logical group inside a `GroupedItemTable`.

Examples:

1. one path,
2. one label,
3. one wiggle trace.

This identity must survive even if many groups are rendered together in one GPU batch.


### 6. Sub-Item Or Auxiliary Identity

Some families may need finer-grained payload when item or group identity alone is not enough.

Examples:

1. a vertex index within a path group,
2. a glyph index within a text label,
3. a face or primitive id inside a mesh-oriented path.

This payload should remain optional and family-defined.


## Why Group Identity Matters

The grouped picking case should be explicit in the scene spec.

For grouped families such as `path` and `glyph`, the scene needs both:

1. one efficient batched rendering path,
2. correct semantic identification of one logical group.

Without explicit group identity, the scene would be forced toward one of two bad outcomes:

1. one resource and one draw path per path or label, which hurts batching,
2. one anonymous batch with no reliable semantic round-trip.

So picking is another strong reason the `GroupedItemTable` concept is correct.


## Picking Result Shape

The scene-level result should be able to report fields conceptually like:

1. `panel_id`
2. `visual_id`
3. `family_id`
4. `item_id`
5. `group_id`
6. `aux_payload`
7. `hit_valid`

The final public C shape is still open, but the semantic content should already be stable at the spec
level.


## Family-Level Picking Expectations


### `pixel`

Expected hit identity:

1. visual id
2. item id

No group identity is normally required.


### `point`

Expected hit identity:

1. visual id
2. item id


### `marker`

Expected hit identity:

1. visual id
2. item id

Optional auxiliary payload may be useful later for marker-shape-specific hit policies, but it should
not be required by the base contract.


### `segment`

Expected hit identity:

1. visual id
2. item id


### `path`

Expected hit identity:

1. visual id
2. group id
3. optional item or vertex payload when needed

The default semantic hit should usually be the logical path, not merely one anonymous vertex inside a
batch.


### `glyph`

Expected hit identity:

1. visual id
2. group id for the logical label or text object
3. optional glyph index when needed


### `image`

Expected hit identity:

1. visual id
2. optional image item id when multiple image placements exist
3. optional sampled-value payload or local image coordinate payload when the family chooses to expose
   it

For slice-like `image` mode, any auxiliary coordinate payload should still be interpreted as image-
family semantics, not as backend texture semantics.


### `mesh`

Expected hit identity:

1. visual id
2. optional item or primitive payload depending on the mesh contract

The exact granularity can remain open for now.


### `sphere`

Expected hit identity:

1. visual id
2. item id for the sphere instance

This should hold even when the family uses impostor-first rendering.


### `volume`

Expected hit identity:

1. visual id
2. optional family-defined auxiliary payload

The exact semantics may differ between:

1. nearest-hit style interaction,
2. sampled-value readout,
3. probe-like interaction.

This can stay partially deferred until the volume family contract is refined further.


## Pick Request Types

The scene layer should distinguish at least:

1. hover request,
2. click request,
3. explicit query request for tools or inspection.

These may share the same underlying picking path, but the scene-side semantics differ:

1. hover may be throttled or coalesced,
2. click usually needs a stable round-trip result,
3. explicit query may expose more diagnostics or payload.


## Pick Timing

The spec should allow both:

1. immediate-style logical requests during interaction,
2. deferred delivery after the relevant frame completes.

The important contract is:

1. the scene must know which request a result belongs to,
2. stale results must be detectable and discardable,
3. the final selection or hover update must map back to current scene identity safely.


## Request Coalescing

Hover picking should be allowed to coalesce or supersede older pending requests.

This is important because pointer motion may outpace the picking readback path.

The scene model should therefore allow:

1. one latest hover request per panel,
2. replacement of stale hover requests,
3. stronger guarantees for click requests than hover requests.


## Picking And `FramePlan`

Picking should appear explicitly in `FramePlan`.

Typical plan contributions include:

1. one picking target,
2. one picking `RenderNode` when picking is enabled or requested,
3. one `ReadbackNode` when the result is needed,
4. resource dependencies linking picking output to readback interpretation.

This keeps picking visible as scene planning work rather than a hidden backend side-channel.


## Always-On Versus On-Demand Picking

The spec should allow both:

1. always-on picking participation for some panels or visuals,
2. on-demand picking when a request is pending.

The exact default policy may be implementation-dependent, but the semantic difference matters:

1. always-on may reduce interaction latency,
2. on-demand may reduce steady-state cost.

Either way, the scene layer should make the policy explicit rather than accidental.


## Picking And Invalidation

Picking-related changes may invalidate more than one layer.

Examples:

1. enabling picking on a visual may invalidate `VisualPropsDirty`, `FramePlanDirty`, and
   `ReadbackRoutingDirty`,
2. changing pick payload shape may invalidate readback routing and the picking pass,
3. changing only pointer position may require a new pick request without requiring full scene
   normalization or `FramePlan` topology change.

This should align with `INVALIDATION_AND_CACHING.md`.


## Picking And Axes

Axes are scene-side composite objects, so their picking semantics should be explicit too.

The default expectation should be:

1. axis lines and ticks may be pickable if enabled,
2. labels may be pickable if useful,
3. returned identity should map back to the owning axis object plus optional component detail.

This does not require axes to become a primitive visual family.
It only means their derived contributions need a coherent scene-level identity route.


## Picking And Grouped Resources

For grouped families, picking should preserve at least group identity by default.

Examples:

1. picking a line strip in a path batch should identify the logical path group,
2. picking a text label in a glyph batch should identify the logical label group.

Optional finer-grained payload may be added later, but the default semantic hit should stay at the
logical-object level.


## Picking And Selection State

Picking results should update scene selection or hover state through scene-owned state transitions,
not through direct backend callbacks.

The intended flow is:

1. pick request issued,
2. picking work planned and executed,
3. result interpreted at the scene level,
4. hover or selection state mutated,
5. redraw requested if needed.


## Diagnostics

The scene should be able to report:

1. which visuals participate in picking,
2. which grouped families return group ids,
3. whether a result came from a stale request,
4. whether a panel uses always-on or on-demand picking,
5. which `FramePlan` nodes were added because of picking.


## Recommended Next Step

The next spec iteration should define controllers and interaction more explicitly.

Picking now has a clear semantic model, but it still depends on a broader document for:

1. event routing,
2. hover and click state machines,
3. panzoom and camera controller ownership,
4. how interaction requests feed frame scheduling.
