# Visual Command Stream Design

Status: active proposal.

This note captures a renderer-independent command boundary above DRP2. The goal is to keep the
retained scene layer authoritative while allowing non-DRP2 consumers to observe visual creation,
updates, and per-frame drawing intent without depending on Vulkan, vklite, canvas internals, or
DRP2's GPU-shaped command vocabulary.


## Motivation

The current active path is:

```text
retained Scene / Panel / Visual state
    -> FramePlan
        -> scene-to-DRP2 emitter
            -> DvzDrp2CommandStream
                -> DRP2 runtime / vklite / Vulkan
```

`FramePlan` is already the boundary between scene state and DRP2 lowering. It contains ordered
upload, compute, render, copy, and readback nodes, plus frame-graph-like resource and pass records.
Render nodes retain scene-visible visual ids and typed `DvzFramePlanVisualMeta` metadata.

DRP2 is backend-agnostic, but it is still GPU-shaped: buffers, textures, bind groups, render
pipelines, passes, and draws. A consumer that wants semantic visual changes should not need to
interpret those lower-level GPU concepts.


## Proposed Boundary

Add a renderer-independent visual command stream above DRP2:

```text
retained Scene / Panel / Visual state
    -> FramePlan
        -> VisualCommandStream       renderer-independent semantic path
        -> DvzDrp2CommandStream      existing GPU execution path
```

The visual command stream should be scene/visual-specific rather than backend-specific. It should
describe what visual work exists and what changed, not how a GPU backend realizes buffers,
pipelines, synchronization, or presentation.


## Two Complementary Streams

### Visual Mutation Stream

A mutation stream records retained-object lifecycle and semantic updates as they happen.

Candidate commands:

| Command | Meaning |
|---|---|
| `CreateVisual` | a visual handle/family was created |
| `DestroyVisual` | a visual was destroyed or detached permanently |
| `AttachVisual` | a visual was attached to a panel with attachment options |
| `SetVisible` | visibility changed |
| `SetAttrData` | a dense attribute payload was replaced |
| `SetAttrRange` | a contiguous dense attribute range changed |
| `SetStrings` | text/string payload changed |
| `SetField` | a sampled field binding or field content changed |
| `SetBuffer` | a scene buffer binding or index buffer changed |
| `SetMaterial` | material/style parameter state changed |
| `SetStyle` | family-specific style state changed |

This stream is useful for external renderers, debugging tools, hosted UI layers, and tests that
need to mirror the retained scene model. It should be driven from scene mutator functions and
version/dirty tracking, not reconstructed from DRP2 commands.


### Visual Frame Stream

A frame stream records per-frame rendering intent after planning and adaptation.

Candidate commands:

| Command | Meaning |
|---|---|
| `BeginFrame` | frame identity and figure target metadata |
| `BeginPanel` | panel identity, layout, viewport, and controller transform state |
| `BeginPass` | logical pass role, target identity, clear/load/store intent |
| `DrawVisual` | one planned visual draw contribution |
| `ReadbackRequest` | picking, probe, export, or tooling readback route |
| `EndPass` | closes the logical pass |
| `EndPanel` | closes the panel scope |
| `EndFrame` | closes the frame scope |

`DrawVisual` should carry scene-visible data such as:

| Field | Source |
|---|---|
| panel id and visual id | `FramePlan` render node |
| visual family/type | `DvzFramePlanVisualMeta.visual_type` |
| pass role | `DvzFramePlanNode.u.render.pass_role` |
| resource ids | visual metadata resource ids |
| vertex/index/instance counts | visual metadata draw counts |
| topology | visual metadata topology |
| alpha/depth state | visual metadata and draw contract |
| transform/controller mode | render node MVP, viewport, and attachment mode |
| field/texture facts | visual metadata field and texture fields |

This stream is the easiest first implementation because `FramePlan` already has most of the
required information.


## Relationship To FramePlan

`FramePlan` should remain the scene-owned producer artifact for one frame. It is the best source for
the visual frame stream because it already resolves panel membership, pass ordering, graph-backed
techniques, capability adaptation, resource ids, draw counts, and readback routing.

The visual frame stream should not replace `FramePlan`; it should be another emitter consuming it.

```text
DvzFramePlan
    -> dvz_frame_plan_emit_visual_commands(...)
    -> dvz_frame_plan_emitter_emit_drp2(...)
```

The mutation stream is different: it is not naturally recoverable from `FramePlan`, because a frame
plan is a frame artifact and not a full history of semantic edits. Mutation commands should be
recorded at scene/visual mutator boundaries if exact change history is required.


## Relationship To DRP2

DRP2 remains the low-level GPU execution protocol. The visual command stream should not expose or
require:

1. Vulkan handles or vklite objects;
2. swapchain, canvas, or presentation state;
3. DRP2 bind-group, shader-module, or render-pipeline ids;
4. command-buffer recording lifecycle;
5. backend synchronization details.

The scene-to-DRP2 emitter may continue using `FramePlan` metadata to lower built-in visuals to GPU
resources and draw commands. The visual command stream gives alternate consumers a semantic path
without forcing them to reverse-engineer that lowering.


## Initial Implementation Direction

The first useful slice should be internal and narrow:

1. Define a `DvzVisualCommandStream` type with deterministic append/read/destroy helpers.
2. Add `dvz_frame_plan_emit_visual_commands()` as a sibling to the DRP2 emitter.
3. Emit frame-scope, panel-scope, pass-scope, and `DrawVisual` commands from existing
   `DvzFramePlan` nodes.
4. Include only borrowed string ids and POD metadata already present in `FramePlan`.
5. Add focused tests that compare a simple point/image/mesh scene's visual command stream to the
   expected planned visual families, counts, pass roles, and resource ids.

After that, add a scene-level mutation journal only if a concrete consumer needs exact retained
object lifecycle and update history.


## Open Questions

1. Should the first API be public, test-only, or internal until a real external renderer consumes it?
2. Should mutation commands be retained until acknowledged by a consumer, or exposed as a snapshot
   plus dirty-range query?
3. Should `DrawVisual` expose family-specific payload structs, or only generic ids plus a visual
   data-query API?
4. How should graph-backed techniques such as WBOIT, depth peeling, EDL, SSAO, and MSAA be named in
   the visual frame stream without leaking GPU implementation details?
5. Should renderer-independent consumers receive normalized visual-space data directly, or only
   stable scene resource ids that they can query separately?
