# DRP2 command streams

Datoviz Rendering Protocol v2 (DRP2) is the backend-neutral command and lifetime contract between
producer-side frame planning and GPU execution. It describes logical GPU objects and work—not
figures, panels, visual families, controllers, or application behavior.

**Status:** **advanced/unstable**, active protocol version 2.0. **Audience:** DRP2 producers,
runtime/backend authors, fixture authors, and replay/diagnostic contributors. **Prerequisites:** C,
GPU resource/pass lifetimes, [Frame lifecycle](../explanation/frame-lifecycle.md), and
[Runtime internals](runtime-internals.md).

Ordinary visualization code should not construct DRP2 streams. Use scene/app APIs and let the
scene-to-DRP2 emitter preserve the supported contracts.


## Protocol boundary

```text
validated/adapted FramePlan
    -> scene-to-DRP2 conversion
    -> immutable artifact-owned command stream
    -> setup/update/frame packet projections
    -> validation + native/WebGPU execution
```

| DRP2 owns | DRP2 does not own |
| --- | --- |
| Typed resource, pipeline, binding, pass, copy, draw, dispatch, barrier, submit, and readback commands. | Scene objects, visual attribute meaning, layout, controller, selection, and fallback policy. |
| Stable logical ids and object/pass/encoder lifecycle rules. | Vulkan/WebGPU handles or allocator objects. |
| Semantic and capability validation before execution. | Backend-specific optimization that changes stream meaning. |
| Binary packets/payload arenas, JSON fixtures/debug export, and DVZR recording shape. | A public JavaScript scene API or a second renderer. |
| Capability declarations and explicit unsupported-feature failures. | Guessing capabilities from backend names. |


## Object and command state

A stream creates logical objects, references them while live, and destroys them only when the
protocol lifetime permits. Render/compute commands also have encoder and pass state:

```text
create resources, shaders, layouts, bindings, pipelines
write retained resource content
begin encoder
    begin render or compute pass
        bind compatible pipeline and resources
        draw or dispatch
    end pass
finish encoder -> command buffer
queue submit command buffer
```

Validation checks object kind and liveness, declared usage, byte ranges, layout compatibility,
pass-kind compatibility, required bindings, and begin/end order. A resource referenced by an open
encoder/pass or a finished unsubmitted command buffer cannot be destroyed. Submitted work is
conservatively treated as using its referenced resources under active 2.0 because the protocol has
no general fence/completion primitive.

Command buffers are immutable recorded work after finish, are consumed by submit, and cannot be
submitted twice in active 2.0.


## Packet transport

Binary packets are the WASM/browser hot path. JSON remains a readable fixture, debug, and recording
view; it is not the runtime transport.

| Packet | Allowed role |
| --- | --- |
| `setup` | Create/recreate/destroy retained objects and initialize new resources. |
| `update` | Mutate retained content without changing identity or dependency topology. |
| `frame` | Encode transient passes, dynamic state, bindings, draw/dispatch/copy, submit, and readback metadata. |

Packet bodies contain no host pointers or JavaScript handles. Large bytes live in an immutable,
8-byte-aligned companion payload arena and are referenced by offsets and sizes. Packet/arena spans
are borrowed from the frame artifact; a browser executor must upload or copy them before release.
Base64 remains valid only in JSON fixtures/debug output.

`resource_version` orders retained setup state and `frame_index` orders packet sets in one runtime
session. All non-empty phases of a set share both counters. Reset clears the runtime's counters and
resource tables; the next set must rebuild every resource later packets reference.


## Capabilities and adaptation

DRP2 capability validation is explicit and backend-neutral. It covers limits, texture/render-target
formats, sample counts, shader formats, optional precision, readback/offscreen support, storage and
blending features, and alignment requirements. Unsupported work fails before backend submission.

Scene adaptation happens *before* the `FramePlan`; it may choose a documented simplification or
reject a feature. DRP2 then validates the chosen command stream. A runtime may not silently replace
that choice with another technique.


## Readback contract

A submitted readback names live `MAP_READ` buffers and in-range byte spans. A non-empty readback list
requires a submission id and exactly one ordered reply. The reply must match the requested buffer
ids, offsets, sizes, and order; each payload contains exactly the requested byte count. Scene code
maps the protocol reply back to request/frame and scene identity.


## Current execution surface

| Area | Current status |
| --- | --- |
| Typed C command stream and semantic validator | active |
| Canonical positive/negative JSON fixtures | active |
| Binary setup/update/frame packets | active browser transport contract |
| Native `vklite` execution for the scene/app subset | active |
| WebGPU fixture and promoted live-route execution | experimental subset |
| Compute plus `ResourceBarrier` to downstream render/copy | narrow experimental slice |
| Full cross-backend pixel conformance | deferred |

This table describes protocol/runtime implementation, not public support for every scene feature.
Use [Feature status](../reference/feature-status.md) and the
[WebGPU matrix](../examples/webgpu-matrix.md) for user-visible claims.


## Change workflow

Do not add a command only because one backend has a convenient primitive. First define the portable
logical operation, lifetime and error rules, capability gates, binary and JSON shape, positive and
negative fixtures, producer lowering, and every claimed runtime mapping. Follow
[Adding a DRP2 command](../contributors/adding-a-drp2-command.md).


## Validation

```sh
just drp2-fixtures
just spec-check
just test drp2
python3 tools/webgpu_fixture_preflight.py
just webgpu-runner-smoke
```

Add focused scene-emitter and native execution tests when a command is produced or executed there.
A JSON fixture passing does not prove native GPU execution or browser visual output; record each
evidence level separately.


## Sources of truth

Use the DRP2 authority order rather than this overview when details conflict:

- [Authority and source order](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/AUTHORITY.md)
- [Layer 1 contract](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/LAYER1.md)
- [Active commands](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/COMMANDS.md)
- [Lifetimes and state](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/LIFETIMES.md)
- [Binary packets](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/PACKETS.md)
- [Errors](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/ERRORS.md)
- [Capabilities](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/CAPABILITIES.md)
- [Conformance](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/CONFORMANCE.md)
