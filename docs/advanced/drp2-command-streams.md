# DRP2

DRP2 is Datoviz Rendering Protocol v2: the backend-neutral command stream between scene planning
and runtime execution. It describes logical GPU work: create buffers and textures, upload bytes,
compile shader modules, create bind groups and pipelines, begin passes, bind resources, draw or
dispatch, copy/read back, and submit work.

```text
scene frame artifact -> DRP2 packets -> runtime
```

Status: advanced/unstable. Ordinary visualization code should use scene, visual, app, and example
APIs.

## What DRP2 Owns

| Area | Role |
| --- | --- |
| Commands | Typed setup, update, frame, copy, render, compute, and submit operations. |
| Validation | Command order, lifetimes, pass state, formats, capabilities, and malformed streams. |
| Fixtures | Positive and negative traces shared by native and WebGPU validation. |
| Packets | Binary setup/update/frame transport plus payload arenas. |
| Recording | DVZR/debug capture and replay workflows. |
| Capabilities | Runtime feature and format reporting. |

DRP2 does not own scene semantics, visual-family APIs, panel layout, controller behavior, or user
interaction policy.

## Concrete Shape

A DRP2 stream is close to WebGPU's object and command model, but expressed as Datoviz-owned C data
and binary packets instead of JavaScript calls. A backend receives logical ids and commands, then
maps them to Vulkan, WebGPU, or a validation/replay tool.

Example shape:

```text
create buffer id=10 usage=vertex|copy_dst size=24000
upload buffer id=10 offset=0 bytes=<positions>
create shader id=20 format=wgsl/glsl source=<point shader>
create bind group layout id=30 entries=<uniforms,textures,buffers>
create bind group id=31 layout=30 resources=<buffer ids, texture ids>
create render pipeline id=40 shader=20 vertex_layout=<position,color>

begin command encoder id=50
begin render pass color_target=swapchain clear=<panel background>
bind pipeline id=40
bind vertex buffer slot=0 buffer=10
bind group slot=0 id=31
draw vertex_count=1000 instance_count=1
end render pass
submit encoder id=50
```

The same stream can be checked by fixtures, executed by the native `vklite` runtime, or translated
to browser WebGPU objects. That is the point of DRP2: scene code emits one GPU-shaped contract,
while each runtime owns the backend-specific handles and submission details.

| DRP2 concept | Rough WebGPU equivalent |
| --- | --- |
| buffer, texture, sampler | `GPUBuffer`, `GPUTexture`, `GPUSampler` |
| shader module | `GPUShaderModule` |
| bind group layout, bind group | `GPUBindGroupLayout`, `GPUBindGroup` |
| render/compute pipeline | `GPURenderPipeline`, `GPUComputePipeline` |
| command encoder | `GPUCommandEncoder` |
| render/compute pass | `GPURenderPassEncoder`, `GPUComputePassEncoder` |
| submit | `GPUQueue.submit()` |

The contract is WebGPU-shaped where practical: explicit logical objects, declared usages, typed
passes, bind groups, pipelines, copies, draw/dispatch commands, and deterministic validation. It is
not a WebGPU binding; native Vulkan remains a first-class execution backend.

## Current Surface

| Topic | Status |
| --- | --- |
| Native command streams | active |
| Fixture validation | active |
| `vklite` execution | active for scene/app runtime subset |
| WebGPU execution | experimental fixture and promoted live-route subset |
| Compute and `ResourceBarrier` | narrow experimental compute-to-render slice |
| Full output conformance | deferred |

## Validation

Use these checks for DRP2/spec work:

```sh
just drp2-fixtures
just spec-check
just test drp2
```

Focused direct tools:

```sh
python3 tools/drp2_fixture_runner.py
python3 tools/webgpu_fixture_preflight.py
```

## Source Of Truth

Durable DRP2 authority lives under `spec/drp2/`:

| Spec | Purpose |
| --- | --- |
| `spec/drp2/AUTHORITY.md` | Conflict resolution and source-of-truth order. |
| `spec/drp2/LAYER1.md` | Human-readable contract overview. |
| `spec/drp2/COMMANDS.md` | Active command surface. |
| `spec/drp2/LIFETIMES.md` | Object lifetime and encoder/pass state rules. |
| `spec/drp2/PACKETS.md` | Binary packet and payload arena transport. |
| `spec/drp2/ERRORS.md` | Validation and error model. |
| `spec/drp2/CAPABILITIES.md` | Capability and format reporting. |
| `spec/drp2/CONFORMANCE.md` | Conformance levels and requirements. |
| `spec/drp2/fixtures/` | Canonical fixture corpus. |

## See Also

- [Runtime internals](runtime-internals.md)
- [WebGPU subset](../reference/webgpu-subset.md)
- [Compute and graphics](../reference/compute-graphics.md)
- [Record and replay frame streams](../how-to/record-replay.md)
- [Adding a DRP2 command](../contributors/adding-a-drp2-command.md)
