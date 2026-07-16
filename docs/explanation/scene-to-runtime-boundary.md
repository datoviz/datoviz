# Scene to runtime boundary

The scene owns authored meaning and producer-side planning. The runtime owns execution. Between them,
an immutable frame artifact carries already-decided logical GPU work without exposing backend
handles to scene code.

**Audience:** contributors deciding where a new capability or fix belongs. **Prerequisite:**
[Architecture](architecture.md). This boundary is an internal design contract; most applications use
scene/app APIs rather than emit artifacts or DRP2 directly.


## The contract

```text
authored scene state
  -> resolve dirtiness
  -> semantic validation
  -> explicit capability adaptation
  -> one FramePlan
  -> immutable frame artifact
  -> DRP2 runtime execution
```

| Scene side | Artifact/DRP2 boundary | Runtime side |
| --- | --- | --- |
| Visual family semantics | Logical ids and typed commands | Concrete buffers/textures/pipelines |
| Panel layout and transforms | Viewports, targets, bindings, pass/draw work | Backend command recording |
| Controller, selection, and query policy | Query/readback request metadata | Submission and completion |
| Validation and fallback choice | Chosen work plus scene-visible diagnostics | Capability enforcement and execution errors |
| Dirty ranges and dependencies | Setup/update/frame packets and payloads | Resource cache and synchronization |


## Capability order

The runtime exposes a backend-neutral capability snapshot before planning: formats, limits, sample
counts, shader/precision support, readback/offscreen availability, alignment constraints, blending,
and attachment limits. The scene validates preferred intent, applies an explicit deterministic
adaptation, and then plans the chosen path.

If an essential capability is absent, the scene rejects or deactivates the feature with a
scene-visible diagnostic. The runtime must not silently simplify a planned technique or reinterpret
ordering. Conversely, scene code must not inspect Vulkan/WebGPU objects to choose a hidden path.


## Logical targets and view ownership

Figures carry logical dimensions and panel layout. A `DvzView` maps a figure to a concrete native,
offscreen, replay, or hosted target and owns runtime reuse. The scene does not reference canvas,
stream, sink, swapchain, window-system, or host-surface objects directly.

The scene may plan logical color, depth, picking, transient, or readback targets. The runtime maps
those requirements to concrete backend resources. App/canvas code owns presentation, PNG capture,
DVZR recording/replay, and optional native UI overlay integration.


## Diagnostics and completion identity

Failures and completions cross the boundary using scene-visible identities: plan node, logical
target/resource, request, frame, item, or sample. Raw backend handles and DRP2 error codes may appear
in verbose debug text but are not required application semantics.


## Decide where a change belongs

| Change | Owning layer |
| --- | --- |
| New visual attribute or selection meaning | Scene semantics and visual-family contract. |
| New upload/compute/render/copy/readback dependency | `FramePlan`, then DRP2/runtime lowering. |
| New portable logical GPU operation | DRP2 command/lifetime/capability contract and supporting runtimes. |
| Native window, offscreen, capture, or host integration | App/canvas/provider boundary. |
| Vulkan resource or submission optimization | `vklite`/native runtime, preserving DRP2 meaning. |
| Browser execution of an existing scene feature | WebGPU runtime support for the same packets; no JavaScript scene fork. |


## Sources of truth

- [Normative runtime boundary](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/scene/core/RUNTIME_BOUNDARY.md)
- [Scene authority and source order](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/scene/AUTHORITY.md)
- [DRP2 capabilities](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/CAPABILITIES.md)
- [Frame lifecycle](frame-lifecycle.md)
