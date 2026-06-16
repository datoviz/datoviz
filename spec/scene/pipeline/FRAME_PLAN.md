# Scene FramePlan IR

## Status

Normative for the scene-owned producer artifact used to plan one frame before scene-to-DRP2
conversion. It is not a public render-graph API and does not freeze runtime ownership, backend
scheduling, or DRP2 object-cache policy.

## Purpose

`FramePlan` sits between scene state and the emitted DRP2 command stream:

```text
Scene/Panel/Visual/Resource state -> FramePlan -> scene-to-DRP2 converter -> DRP2 runtime
```

It must provide deterministic planning, separate scene mutation from DRP2 emission, and remain
inspectable/serializable for tests.

## Core Rules

1. One scene-level `FramePlan` is produced for each frame build.
2. Panels contribute subplans, node groups, or target configuration inside that one plan.
3. Cross-panel ordering must be explicit; it must not be left to unrelated top-level plans.
4. `FramePlan` is scene-owned producer data, not runtime-owned execution state.
5. It must not contain Vulkan, Metal, WebGPU, GLFW, swapchain, command-buffer, or image-view handles.
6. It describes logical work and dependencies, not backend command-buffer mechanics.
7. It is deterministic for a given scene state, input event set, and capability record.
8. Upload work for scene resources is emitted through the plan, not a parallel execution path.

## Minimum Shape

A valid first-slice plan contains:

| Part | Required content |
|---|---|
| Metadata | frame index, scene revision, capability snapshot, validation/adaptation summary, target panels, flags |
| Targets | logical color/depth/picking/offscreen/transient targets |
| Resources | stable logical resource ids referenced this frame |
| Nodes | ordered `UploadNode`, `ComputeNode`, `RenderNode`, `CopyNode`, `ReadbackNode` entries |
| Dependencies | explicit edges or topological order plus read/write sets |
| Readbacks | deterministic producer-visible request descriptors |
| Diagnostics | planning warnings/errors before DRP2 emission |

## Logical Targets

Target kinds:

- panel color target;
- panel depth target;
- picking target;
- offscreen export target;
- transient intermediate target for multi-stage composition.

Targets describe logical format requirements, dimensions/sizing policy, sample count, clear/load/
store intent, and whether readback is required. They never expose backend handles.

Intermediate color targets are linear. Final display output is sRGB by default. The runtime decides
whether final linear-to-sRGB encoding is performed by an sRGB-capable target/swapchain or by an
explicit final encode pass.

Scene-level DRP2 emission must not depend on Vulkan-specific swapchain details. See
[`../semantics/COLOR_MANAGEMENT.md`](../semantics/COLOR_MANAGEMENT.md).

## Logical Resources

Resources are referenced by stable scene ids. Each referenced resource records:

- kind: buffer, texture, uniform/parameter block, readback buffer, or equivalent;
- usage role in the current frame;
- creation intent;
- upload or subrange-write intent;
- nodes that read it;
- nodes that write it.

Producer-side hints may mark resources immutable/dynamic, shared/panel-local, or transient/
persistent. These guide planning and validation only.

Derived resources must be classified as one of:

1. authoritative scene data;
2. reusable persistent derived cache;
3. frame-local transient output.

## Node Kinds

| Node | Required behavior |
|---|---|
| `UploadNode` | first-use creation intent, dirty buffer ranges, texture writes, ordering before consumers |
| `ComputeNode` | input/output resources, logical shader/program variant, dispatch dimensions, downstream consumers |
| `RenderNode` | attachments, clear/load/store behavior, viewport/scissor policy, ordered draw items, depth/blend/pick mode |
| `CopyNode` | logical buffer/texture transfers visible at FramePlan/DRP2 level |
| `ReadbackNode` | deterministic producer-visible retrieval for picking, image export, or test/tooling data |

There is no `OverlayNode`. External UI overlays are runtime-injected after scene submission and
before present; see `FRAME_LIFECYCLE.md`.

`RayTraceNode` is reserved for future hardware ray tracing. If added, it replaces `RenderNode` for
ray-traced visuals while preserving the same scene/planning boundary.

## Draw Items

Each `RenderNode` contains a logical draw-item list. A draw item identifies:

- source visual id;
- material/shader variant identity;
- geometry/resource bindings;
- transform inputs or resolved transform id;
- optional picking payload id;
- draw parameters such as vertex or index count.

Material is not a separate FramePlan concept. Shading parameters are regular parameter-block
resources attached to visuals.

## Dependencies

The dependency representation may be explicit edges or a topologically ordered node list with
per-node read/write sets. It must express:

- upload before first use;
- compute before render/copy/readback consumers;
- compute `STORAGE` writes before visual vertex reads, including a lowering path to DRP2
  `ResourceBarrier`;
- picking render before picking readback;
- offscreen render before export readback;
- panel-local ordering;
- cross-panel shared-resource ordering.

Scene compute objects lower to `ComputeNode` entries. Their declared buffer bindings define the
node read/write sets, and explicit ordering such as `compute before visual` decides where the node
appears relative to normal render nodes. Dispatch dimensions are frame-varying scene state: they may
be initialized in the compute descriptor and changed before a later frame is emitted.

## Capability Adaptation And Diagnostics

Adaptation policy runs before planning. `FramePlan` records the chosen outcome and may still reject
invalid topology. The result is one of:

1. valid plan;
2. deterministic degraded plan;
3. scene-visible diagnostic before DRP2 submission.

Planning diagnostics include unsupported capabilities, unresolved resource dependencies, invalid
target configuration, incompatible visual/material combinations, and unsupported picking/readback
requests. Diagnostics are scene-level and must not leak backend handles.

## Shader And Pipeline Identity

The scene-to-DRP2 converter assigns deterministic runtime ids from scene shader keys:

- shader modules are keyed by stage, source hash, and transport format;
- render pipelines are keyed by vertex module id, fragment module id, and pipeline state;
- built-in scene shaders and pipelines also carry stable family/variant identity plus a contract
  version in DRP2 metadata;
- create commands are emitted once per unique key and omitted when already live;
- destroy commands are emitted when the referencing visual is removed or its variant changes.

The built-in contract version is scene-owned metadata. It is not a custom-shader API and should only
change when the built-in binding/resource contract changes.

## Relationship To Scene Objects And DRP2

Scene objects contribute:

| Source | Contribution |
|---|---|
| `Scene` | global shared resources and scheduling policy |
| `Panel` | targets, camera state, panel-local visual membership, node grouping |
| `Visual` | draw items, stage participation, resource requirements |
| `Resource` | creation intent, dirty ranges, sharing information |
| `Animation` / `Controller` | state changes before planning begins |
| `Compute` | custom compute shader identity, dispatch state, buffer bindings, and visual ordering |

DRP2 conversion:

| FramePlan node | DRP2 category |
|---|---|
| Upload | resource creation/write commands |
| Compute | compute-pass commands plus required resource barriers for downstream consumers |
| Render | render-pass commands and draws |
| Copy | DRP2 copy commands |
| Readback | `QueueSubmit.readbacks` and reply routing metadata |

The converter owns exact command spelling, id assignment, and omission of already-created runtime
objects.

## Required Coverage

The first IR is acceptable only if it can represent:

1. one-panel static plot;
2. dynamic buffer updates without whole-scene rebuild;
3. picking pass plus single-pixel readback;
4. offscreen rendering plus deterministic image readback;
5. one compute-assisted visual path followed by rendering;
6. a compute dispatch whose dimensions change between emitted frames.
