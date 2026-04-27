# Scene FramePlan IR

This document defines the preferred intermediate representation used by the future scene layer to
plan one frame of work before emitting DRP2.

It is intentionally not a frozen public render-graph API.

Its purpose is narrower:

1. give the scene layer a deterministic planning structure,
2. separate scene-state mutation from DRP2 emission,
3. provide a stable producer-side model while some DRP2 object details remain under active review.


## Normative Status

This document is normative for the producer-side execution artifact.

Examples, deferred questions, and follow-on notes in this document are informative.


## Position In The Stack

`FramePlan` sits between:

1. scene-owned state such as panels, visuals, resources, cameras, and controllers,
2. the final DRP2 command stream emitted for the frame.

The relationship is:

1. scene state is updated first,
2. a `FramePlan` is derived from that state,
3. DRP2 emission is a pure translation of the `FramePlan` plus runtime capabilities.


## Plan Scope

The current spec direction is that one scene-level `FramePlan` is produced for each frame build.

That plan may still contain:

1. panel-local targets,
2. panel-local render or picking nodes,
3. panel-local ordering constraints,
4. optional panel-local derived resources,
5. scene-global nodes that coordinate shared resources or multi-panel composition.

Panels therefore contribute subplans, node groups, or target configuration inside one scene-owned
plan.

The scene should not build several unrelated top-level plans for one frame and leave cross-panel
ordering implicit.


## Goals

The first `FramePlan` IR should be able to express:

1. resource uploads and lazy resource creation,
2. one or more render stages,
3. optional compute stages before or between render stages,
4. offscreen and picking paths,
5. deterministic readback requests,
6. panel-local and scene-global ordering constraints.


## Non-Goals

The first `FramePlan` IR should not freeze:

1. a public user-facing render-graph API,
2. final runtime object ownership,
3. exact DRP2 object-creation policy for pipelines, bind groups, samplers, or texture views,
4. backend-specific scheduling or synchronization controls,
5. performance-tuning policy beyond what is needed for deterministic planning.


## Design Rules

1. `FramePlan` is scene-owned producer data, not runtime-owned execution state.
2. `FramePlan` must not contain Vulkan, Metal, WebGPU, GLFW, or swapchain handles.
3. `FramePlan` should describe logical work and dependencies, not backend command-buffer mechanics.
4. `FramePlan` should be deterministic for a given scene state, input event set, and capability
   record.
5. `FramePlan` should be inspectable and serializable for tests even if its final in-memory
   representation changes later.


## Minimum IR Shape

The first useful `FramePlan` can be modeled as:

1. plan metadata,
2. logical targets,
3. logical resources referenced this frame,
4. ordered plan nodes,
5. explicit dependency edges or equivalent ordering constraints,
6. optional readback requests,
7. diagnostics collected during planning.


## Plan Metadata

Each plan should carry enough metadata for debugging and deterministic tests:

1. frame index,
2. scene revision or equivalent state-generation number,
3. capability snapshot identifier or summary,
4. validation scope or validation generation summary,
5. adaptation outcome summary or identifier,
6. target panel set,
7. planning flags such as offscreen-only or picking-enabled.


## Logical Targets

A target is a logical rendering destination used by plan nodes.

The first scene-level target kinds should be:

1. panel color target,
2. panel depth target,
3. picking target,
4. offscreen export target,
5. transient intermediate target for multi-stage composition.

Targets should describe:

1. logical format requirements,
2. dimensions or sizing policy,
3. sample-count requirements,
4. clear/load/store intent,
5. whether readback is required after execution.

Targets should not expose backend image/view handles.


## Logical Resources

The `FramePlan` should reference scene resources through stable logical ids.

For each referenced resource, the plan should know at least:

1. resource kind such as buffer, texture, uniform block, or readback buffer,
2. usage role in the current frame,
3. whether creation is required,
4. whether upload or subrange write is required,
5. which plan nodes read it,
6. which plan nodes write it.

The plan may also carry producer-side materialization hints such as:

1. immutable versus dynamic,
2. shared versus panel-local,
3. transient versus persistent.

These hints exist to guide planning and validation, not to expose backend allocation strategy.

The plan should also be able to distinguish whether a derived resource is:

1. authoritative scene data,
2. reusable persistent derived cache,
3. frame-local transient output.

This is especially important for compute-written resources and readback paths.


## Node Kinds

The first `FramePlan` does not need many node kinds.

The minimum useful set is:

1. `UploadNode`
2. `ComputeNode`
3. `RenderNode`
4. `CopyNode`
5. `ReadbackNode`

A `RayTraceNode` is reserved as a future node kind for hardware ray tracing.
It would replace `RenderNode` for ray-traced visuals when the capability is available and
requested.
The scene layer emits it identically to other node types; the DRP2 runtime handles BVH
construction and ray tracing command recording.
See `LIGHTING.md` for the forward-compatibility design.


## UploadNode

An `UploadNode` represents host-driven resource materialization needed for this frame.

It should support:

1. first-use resource creation intent,
2. dirty-range buffer writes,
3. texture writes,
4. upload ordering before dependent compute or render nodes.

`UploadNode` is where scene dirty tracking becomes concrete frame work.

`UploadNode` should be the canonical place where already-resolved resource dirtiness becomes explicit
execution work.

The scene should not maintain a parallel execution path that emits upload work outside `FramePlan`.


## ComputeNode

A `ComputeNode` represents one logical compute stage.

It should declare:

1. its input resources,
2. its output resources,
3. the logical shader or program variant it requires,
4. dispatch dimensions or a scene-level equivalent,
5. whether its outputs are later consumed by render, copy, or readback nodes.

It should not encode backend pipeline or encoder internals directly.

Unless a stronger scene contract says otherwise, compute-written outputs should be treated as
frame-local derived resources.

Persistence across frames should be explicit rather than implicit.


## RenderNode

A `RenderNode` represents one logical render pass or render stage.

It should declare:

1. target attachments,
2. clear/load/store behavior,
3. viewport/scissor policy at a logical level,
4. the ordered draw items to execute,
5. the visual set or draw list that contributes to the node,
6. any required depth, blending, or picking mode.

One render node may correspond to one panel pass, one picking pass, or one intermediate composition
pass.


## CopyNode

A `CopyNode` represents explicit logical transfers not covered by uploads.

The first plan only needs copies for:

1. buffer-to-buffer movement when required by the producer model,
2. texture export preparation,
3. staging transfers that are visible at DRP2 level.

If a copy is purely backend-private, it does not belong in `FramePlan`.


## ReadbackNode

A `ReadbackNode` represents deterministic producer-visible data retrieval after execution.

The first scene slice should support:

1. single-pixel picking readback,
2. full or partial offscreen image readback,
3. optional compute-result readback when needed by tests or tooling.

Readback nodes should specify the logical destination for interpreted results, not a backend mapping
API.


## Draw Items Inside RenderNode

The scene layer should be free to change the concrete storage shape later, but each render node needs a
logical draw-item list.

Each draw item should identify:

1. source visual identity,
2. material or shader variant identity,
3. geometry/resource bindings required by that item,
4. transform inputs or resolved transform identity,
5. optional picking payload identity,
6. draw parameters such as vertex count or index count.

This keeps visual semantics visible to planning without freezing the final low-level binding model.


## Dependencies And Ordering

`FramePlan` must make dependencies explicit enough that DRP2 emission is not forced to rediscover them.

The first plan can represent this either as:

1. explicit edges between nodes, or
2. a topologically ordered node list plus per-node read/write sets.

Whichever representation is used, it must express:

1. upload before first use,
2. compute before render when outputs feed rendering,
3. picking render before picking readback,
4. offscreen render before export readback,
5. panel-local ordering and cross-panel shared-resource ordering.


## Capability Adaptation

`FramePlan` should reflect capability-shaped producer decisions, but it should not be the first place
where those decisions are discovered.

Examples:

1. choose a non-FP64 visual variant when FP64 is unavailable,
2. disable a compute-assisted path and choose a fallback plan,
3. select a supported sample count,
4. refuse to build a plan that requires unsupported texture formats.

The result should be one of:

1. a valid plan,
2. a deterministic degraded plan,
3. a scene-visible planning diagnostic before DRP2 submission.

The preferred rule is:

1. adaptation policy runs before planning,
2. `FramePlan` records the chosen adapted outcome,
3. planning diagnostics may still reject a plan if no valid adapted topology exists.


## Planning Diagnostics

Planning may fail before DRP2 emission.

The planning stage should be able to report diagnostics such as:

1. unsupported capability for a required visual path,
2. unresolved resource dependency,
3. invalid panel target configuration,
4. incompatible visual/material combination,
5. unsupported picking or readback request.

These are scene-level diagnostics and should remain free of backend leakage.


## Relationship To Existing Scene Objects

`FramePlan` should be derived from the existing scene object model as follows:

1. `Scene` contributes global shared resources and scheduling policy,
2. `Panel` contributes target configuration, camera state, panel-local visual membership, and any
   panel-local node grouping within the scene-level plan,
3. `Visual` contributes draw items, stage participation, and resource requirements,
4. `Resource` contributes creation intent, dirty ranges, and sharing information,
5. `Animation` and `Controller` contribute state changes before planning begins.


## Relationship To DRP2

The `FramePlan` should be translatable to DRP2 command categories already in scope:

1. upload nodes become resource creation and write commands where supported,
2. compute nodes become compute-pass commands,
3. render nodes become render-pass commands and draw calls,
4. copy nodes become DRP2 copy commands,
5. readback nodes become offscreen/readback requests through the runtime boundary.

Where DRP2 still has deferred object-creation details under review, `FramePlan` should refer to logical
shader, material, and binding identities rather than assuming the final command shape.


## Minimum Worked Examples This IR Must Cover

The first `FramePlan` IR is acceptable only if it can represent:

1. one-panel static plot,
2. dynamic buffer updates without whole-scene rebuild,
3. a picking pass plus single-pixel readback,
4. offscreen rendering plus deterministic image readback,
5. one compute-assisted visual path followed by rendering.


## Resolved Questions

**Transient logical targets** — resolved.
Transient render targets (picking pass buffers, intermediate composition targets, multi-pass
effect intermediates) are internal planning artifacts.
The planner may create, reuse, and destroy them without exposing them to the user.
The only targets that require scene-visible identity are declared outputs: the final swapchain
image, explicit offscreen export targets, and picking readback sinks — all of which the user
already names when setting up a panel or requesting a readback.

**`FramePlan` public inspectability** — resolved.
`FramePlan` is not a first-class public user-facing API.
It is readable and serializable through a diagnostics or test interface.
Normal user code never touches it directly.
Exposing it as a stable user API would freeze internal planning structure prematurely.

**Material binding** — resolved by `VISUAL_CONTRACT.md`.
Material is not a first-class scene concept.
Shading parameters are fields inside a `ParameterBlockResource`.


## Deferred Questions

The following questions should remain explicitly open for now:

1. how shader-module and pipeline identities map onto final DRP2 object-creation commands.


## Immediate Follow-On Specs

This document should be followed by:

1. `VISUAL_CONTRACT.md`
2. `RESOURCE_MODEL.md`
3. `PICKING.md`
4. `DIAGNOSTICS_SCHEMA.md`
5. worked examples that trace scene state to `FramePlan` to DRP2 command categories
