# Runtime internals

The Datoviz runtime executes an immutable DRP2 snapshot produced by scene planning. It owns backend
resources, synchronization, command recording, and submission; it does not reconstruct visual or
panel meaning from command ids.

**Audience:** Vulkan/WebGPU backend authors, native embedding developers, and contributors working
on canvas, stream, capture, replay, or runtime failures. **Prerequisites:** C, GPU resource
lifetimes, and [Scene to runtime boundary](../explanation/scene-to-runtime-boundary.md).

Runtime and DRP2 APIs are **advanced/unstable**. The WebGPU executor is an **experimental subset**.
Application code should normally use scene, app, view, and documented integration APIs.


## Active stack

```text
DvzView render step
    -> scene validates, adapts, and builds one FramePlan
    -> DvzSceneFrameArtifact freezes DRP2 stream + payloads
    -> DvzDrp2Runtime validates and executes
         -> vklite owns native Vulkan execution objects
         -> canvas acquires native/offscreen frames
         -> stream fans frames to presentation/capture/sinks
    -> app presents, captures, records, replays, or polls results
```

The browser replaces the native executor and presentation pieces, not the scene:

```text
C/WASM scene artifact -> setup/update/frame packets -> JavaScript WebGPU runtime -> canvas
```


## Responsibilities and lifetime owners

| Layer | Owns | Important lifetime boundary |
| --- | --- | --- |
| `DvzView` / app | Figure-to-target mapping, render stepping, input/size synchronization, runtime reuse, capture/record/replay orchestration. | Destroy views/apps before their scene; hosted targets may remain host-owned. |
| Frame artifact | Immutable command stream, frozen payloads, encoded packets, versions, and diagnostics. | Borrowed stream/packet/arena views end at artifact destruction. |
| `DvzDrp2Runtime` | Protocol validation state, stable logical-id tables, execution acceptance, backend dispatch. | One retained runtime session must observe object lifetimes and packet order. |
| `vklite` | Vulkan buffers, textures, samplers, shaders, pipelines, bindings, encoders/passes, queues, and synchronization. | Destroy or reuse only according to in-flight reference ownership. |
| Canvas | Native/offscreen target, frame acquisition, target-dependent resources, readback-capable frame behavior. | Borrowed external surfaces/images remain host-owned. |
| Stream/sinks | Frame delivery, presentation/capture/video fan-out and timing. | A frame remains valid only for its documented sink/lease interval. |
| Browser runtime | Adapter/device/context, retained DRP2 object tables, packet counters, WebGPU execution and recovery. | WASM packet views must be consumed/copied before artifact release. |


## Runtime execution contract

Execution has three distinct checks:

1. **Protocol validation** verifies schema, ids, object/pass/encoder state, usages, ranges, and
   lifetime ordering.
2. **Capability validation** verifies formats, sample counts, shader formats, precision, limits,
   and other declared gates.
3. **Backend execution** maps accepted logical commands to concrete resources and submissions.

A failure should retain the narrowest useful identity: command, logical resource, pass/encoder,
plan node, target, request, or frame. A backend error string is diagnostic detail, not application
identity. Unknown ids, unsupported commands, bad states, and unsupported formats must fail
explicitly; the runtime must not infer missing setup or visual semantics.


## Retained sessions and packet phases

The native runtime consumes the artifact-owned stream snapshot. Browser transport encodes the same
logical stream in three phases:

| Phase | Meaning |
| --- | --- |
| `setup` | Create/recreate/destroy retained resources and initialize newly created objects. Advances `resource_version`. |
| `update` | Write or copy content while retaining object identity and dependency shape. |
| `frame` | Encode transient passes, bindings, draws, dispatches, copies, submit, and readback metadata. |

Within one packet set, non-empty phases share `resource_version` and `frame_index`; setup precedes
update, which precedes frame. A runtime session rejects stale versions, non-increasing frame indexes,
duplicate live ids, and frame references to unknown objects. After runtime reset or browser device
recovery, the next accepted set must include the setup needed to reconstruct retained resources.


## Resize and recovery

Target-dependent runtime objects are disposable; the C/WASM scene is durable user state. Resize or
browser device recovery may rebuild the canvas/runtime and replay fresh setup-bearing artifacts, but
must preserve scene objects, controllers, selection, animation, and application state. Recreating
the scene to repair a transient target loses the ownership boundary and is not the normal recovery
model.

Native resize follows the same principle: synchronize figure/target dimensions, recreate only
size-dependent target resources, retain scene data, then render a newly planned frame.


## Capture, recording, and readback

Capture output is tightly packed sRGB RGBA8 unless a public API states otherwise. DVZR recording is
a view/runtime diagnostic: it stores DRP2 commands and payload bytes, not scene objects or video.
Readback requests and replies need submission/frame identity, exact byte ranges, and explicit
completion. Use [Record and replay](../how-to/record-replay.md) for the supported workflow and the
[Queries reference](../reference/queries.md) for scene result semantics.


## Change routing

| Work | Start here |
| --- | --- |
| Change scene meaning, fallback, or plan topology | [Scene to runtime boundary](../explanation/scene-to-runtime-boundary.md) and `spec/scene/` |
| Add or alter a portable logical GPU operation | [DRP2 command streams](drp2-command-streams.md) and [Adding a DRP2 command](../contributors/adding-a-drp2-command.md) |
| Change native object execution | `src/vklite/`, `src/drp2/`, and the relevant DRP2 lifetime contract |
| Change target acquisition or presentation | `src/canvas/`, `src/stream/`, `src/app/` |
| Extend browser execution | [WebGPU subset](../reference/webgpu-subset.md) and [Adding a WebGPU fixture](../contributors/adding-a-webgpu-fixture.md) |
| Inspect exact lower-level symbols | [Runtime C API](../reference/c-api/runtime.md) |


## Validation by boundary

```sh
just test drp2       # protocol validation and command-stream behavior
just test vklite     # native execution objects and mappings
just test canvas     # frame targets and acquisition
just test stream     # frame delivery and sinks
just wasm-scene-smoke
just webgpu-runner-smoke
```

Add Vulkan validation and a bounded native/offscreen smoke when changing targets, command buffers,
synchronization, external handles, or submission lifetimes. Add browser smoke only when the change
affects live canvas execution rather than fixture translation alone.


## Sources of truth

- [Scene runtime boundary](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/scene/core/RUNTIME_BOUNDARY.md)
- [DRP2 reading order](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/drp2/READING_ORDER.md)
- [DRP2 runtime API](../reference/c-api/runtime.md)
- [GPU resource ownership](../explanation/gpu-resource-ownership.md)
