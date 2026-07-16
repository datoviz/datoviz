# GPU resource ownership

Datoviz separates semantic scene ownership from concrete GPU ownership. A scene describes logical
resources and work; a runtime owns or borrows the backend objects needed to execute it. Ownership
includes authority to mutate, transition, submit, reset, and destroy—not only responsibility to call
a destructor.

**Audience:** runtime, embedding, Vulkan/WebGPU, capture, and interop developers. **Prerequisite:**
[Scene to runtime boundary](scene-to-runtime-boundary.md). Exact public lifetimes remain in
[Objects and lifetimes](../reference/objects-and-lifetimes.md).


## Ownership by layer

| Object or memory | Normal owner | Borrower rule |
| --- | --- | --- |
| Scene objects and copied attribute data | `DvzScene` | Views/runtime consume planned snapshots, not mutable scene internals. |
| Frame artifact stream, payloads, packets, diagnostics | `DvzSceneFrameArtifact` | Valid until artifact destruction; copy if needed later. |
| Native buffers, textures, pipelines, encoders, submissions | `DvzDrp2Runtime`/`vklite` according to their contract | Do not expose these as scene identity. |
| Canvas/swapchain/offscreen target | View/canvas or external host | Use only during the granted frame/target lifetime. |
| Host window/surface or Qt/Vulkan object | Host/provider unless transfer is explicit | Datoviz must not destroy, reset, or reconfigure beyond the host contract. |
| WASM packet/arena span | Frame artifact in WASM linear memory | JavaScript must execute or copy before release, next emit where specified, or scene destruction. |
| Mapped readback/result view | Runtime/result owner | Copy before unmap, release, next operation, or destruction specified by the API. |


## Borrowing is capability-specific

A borrowed handle is not “temporarily owned.” Unless the API explicitly grants authority, Datoviz
must not destroy it, begin/end/reset it, submit it, change its layout/state, or extend its lifetime.
This applies to swapchain images, command buffers, external memory, Qt surfaces, platform windows,
textures, and interop synchronization objects.

The reverse also matters: host code must not mutate a Datoviz-owned runtime object behind its back.
Use an external/borrowed API whose contract declares the access and synchronization instead.


## In-flight references

Recorded or submitted work may continue to reference resources after command creation. DRP2 rejects
destruction while an object is referenced by an open pass/encoder or a finished unsubmitted command
buffer. Active DRP2 2.0 has no general fence/completion primitive, so protocol clients must use its
conservative lifetime rules rather than assume immediate GPU completion after submit.

At the higher app layer, repeated view execution owns the runtime session and its safe reuse. Scene
code should express resource dependencies and barriers in the plan/DRP2 stream instead of inserting
backend synchronization shortcuts.


## Synchronization follows ownership

A compute pass that writes storage and a render pass that consumes it need an explicit dependency.
`FramePlan` records the read/write relationship; lowering emits a DRP2 `ResourceBarrier` where the
active contract requires one; each backend maps that logical ordering to supported primitives. The
barrier orders access but does not transfer public ownership of the resource.


## Destruction order

In the ordinary C application path, stop and destroy views/apps before destroying their scene.
Destroy an emitted artifact only after the runtime or host has finished consuming its borrowed
views. External providers may add stricter teardown order; follow their integration contract.


## Sources of truth

- [Scene runtime boundary](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/scene/core/RUNTIME_BOUNDARY.md)
- [DRP2 lifetimes](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/LIFETIMES.md)
- [DRP2 packet artifact boundary](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/PACKETS.md)
- [Compute and graphics](../reference/compute-graphics.md)
