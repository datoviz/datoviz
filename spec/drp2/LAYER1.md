# DRP2 Layer 1

Human-readable contract for the future Datoviz Rendering Protocol v2.

Status: draft
Scope: intentionally reduced first contract


## Purpose

DRP2 is the backend-agnostic rendering contract that future Datoviz runtimes should execute.

It exists to let higher-level producers emit one logical GPU command stream that can be consumed by:

1. a native runtime with a Vulkan backend,
2. a browser runtime over WebGPU,
3. future tooling for replay, validation, capture, and testing.


## Non-Goals

This first contract freeze is not trying to define:

1. the full future scene API,
2. native interop escape hatches,
3. profiling and benchmarking APIs,
4. a complete binary transport format,
5. advanced memory-management policy.

Those topics matter, but they should remain outside the initial DRP2 contract freeze unless they are
strictly required by the minimal renderer slice.


## Design Principles

1. Backend-agnostic public contract.
2. WebGPU-shaped semantics where practical.
3. No `Vk*` types or Vulkan constants in public DRP2 definitions.
4. Deterministic validation and replay at the contract level.
5. Narrow first version, expand later.


## Object Model

The first DRP2 contract defines logical objects only.
Backends remain free to implement them with different physical strategies.

Object kinds:

1. buffer
2. texture
3. texture view
4. sampler
5. shader module
6. bind group layout
7. bind group
8. pipeline layout
9. render pipeline
10. compute pipeline
11. command encoder
12. render pass encoder
13. compute pass encoder

Each object is addressed by an explicit logical id chosen by the producer.


## Command Categories

The reduced v1 contract should cover only these categories:

1. resource creation and destruction,
2. resource upload and copy,
3. shader and pipeline creation,
4. command encoder lifecycle,
5. render pass lifecycle,
6. compute pass lifecycle,
7. draw and dispatch,
8. queue submission.


## Required First-Slice Commands

The minimal contract should include:

1. `CreateBuffer`
2. `WriteBuffer`
3. `CreateTexture`
4. `WriteTexture`
5. `CreateTextureView`
6. `CreateSampler`
7. `CreateShaderModule`
8. `CreateBindGroupLayout`
9. `CreateBindGroup`
10. `CreatePipelineLayout`
11. `CreateRenderPipeline`
12. `CreateComputePipeline`
13. `BeginCommandEncoder`
14. `FinishCommandEncoder`
15. `BeginRenderPass`
16. `EndRenderPass`
17. `SetPipeline`
18. `SetBindGroup`
19. `SetViewport`
20. `SetScissor`
21. `Draw`
22. `DrawIndexed`
23. `BeginComputePass`
24. `EndComputePass`
25. `DispatchWorkgroups`
26. `CopyBufferToBuffer`
27. `CopyBufferToTexture`
28. `CopyTextureToBuffer`
29. `QueueSubmit`

Destruction commands may be added in the first contract if object lifetime is not sufficiently clear
without them. If omitted initially, runtime-owned teardown rules must still be explicit.


## Execution Semantics

1. Commands are immutable once emitted.
2. Commands are consumed in order.
3. Validation is part of the contract, not an optional debug feature.
4. Implicit synchronization should follow WebGPU-like semantics in the first version.
5. Explicit backend-specific synchronization is deferred unless it becomes necessary for the minimal
   renderer slice.


## Resource Rules

1. Every resource has an explicit logical id.
2. Resource usage must be declared at creation time.
3. Upload and copy commands must obey documented range, layout, and alignment rules.
4. Texture formats and shader formats are validated against the capability model.
5. The public contract describes logical resources, not allocation strategy.


## Shader Rules

1. WGSL should be the default contract-level shader language.
2. Native-only ingestion paths such as SPIR-V may exist behind explicit capability flags.
3. A shader module must declare enough metadata for deterministic validation.
4. Pipeline creation must fail early if declared resources, layouts, or stages are incompatible.


## Pass Rules

Render and compute passes are explicit encoder scopes.

Rules:

1. Draw commands are valid only inside a render pass.
2. Dispatch commands are valid only inside a compute pass.
3. Attachments, load/store operations, and pipeline state must be validated before execution.
4. Pass compatibility is a contract-level concern, not only a backend detail.


## Validation

Every runtime should implement the same logical validation model.

At minimum, validation covers:

1. object existence,
2. object type compatibility,
3. command ordering,
4. pass scope correctness,
5. binding compatibility,
6. resource range and layout checks,
7. capability gating,
8. version compatibility.

Detailed symbolic codes live in `ERRORS.md`.


## Capability Model

DRP2 must have an explicit capability report used before feature-dependent command streams are emitted.

At minimum the capability model must cover:

1. supported protocol versions,
2. shader language support,
3. texture format support,
4. sample count support,
5. compute availability,
6. FP64 support,
7. key size and binding limits.

Detailed rules live in `CAPABILITIES.md`.


## Versioning

Every stream declares a DRP2 protocol version.
The schema, fixture set, and human-readable Layer 1 contract for a given version must be kept in
lockstep.

Detailed rules live in `VERSIONING.md`.


## Conformance

The contract is not ready until it has:

1. machine-readable schemas,
2. canonical fixtures,
3. negative fixtures for validation failures,
4. native and browser replay expectations.


## Pressure Tests From Future Scene Work

The first contract freeze should be checked against at least these producer stories:

1. static geometry in a panel,
2. dynamic buffer updates across frames,
3. texture upload and sampling,
4. picking-oriented render-to-texture plus readback,
5. one compute-assisted data path if compute is considered mandatory for v1.

If DRP2 cannot express those cleanly without backend leakage, the contract is not ready.

```
CreateBindGroup {
  id,
  layout,
  entries: [
    { binding, resource: bufferId, offset?, size? },
    { binding, resource: textureViewId },
    { binding, resource: samplerId },
    ...
  ]
}
```

---

# 6. Shader Modules

## 6.1 CreateShaderModule

```
CreateShaderModule {
  id,
  format: "wgsl" | "spirv" | "glsl",
  code: string or bytes
}
```

Backend rules:

* Vulkan: prefers SPIR-V; can compile GLSL → SPIR-V if `shader_glsl` enabled.
* WebGPU: WGSL only. `spirv` accepted only if native backend supports it.

---

# 7. Pipeline Creation

## 7.1 CreateRenderPipeline

```
CreateRenderPipeline {
  id,
  layout,

  vertex: {
    module,
    entryPoint,
    buffers: [
      {
        arrayStride,
        stepMode,
        attributes: [
          {shaderLocation, offset, format}
        ]
      }, ...
    ]
  },

  fragment?: {
    module,
    entryPoint,
    targets: [
      {
        format,
        blend?: {
          color: {src, dst, op},
          alpha: {src, dst, op}
        },
        writeMask
      }, ...
    ]
  },

  primitive: {
    topology,
    stripIndexFormat?,
    frontFace?,
    cullMode?
  },

  depthStencil?: {
    format,
    depthWriteEnabled?,
    depthCompare?,
    stencilFront?,
    stencilBack?,
    depthBias?, depthBiasSlopeScale?, depthBiasClamp?
  },

  multisample?: {
    count,
    mask?,
    alphaToCoverageEnabled?
  },

}
```

---

## 7.2 CreateComputePipeline

```
CreateComputePipeline {
  id,
  layout,
  compute: {
    module,
    entryPoint
  }
}
```

---

# 8. Command Encoding

## 8.1 BeginCommandEncoder

```
BeginCommandEncoder { id }
```

Creates a GPUCommandEncoder.

---

## 8.2 FinishCommandEncoder

```
FinishCommandEncoder {
  encoder,
  id   // id of resulting GPUCommandBuffer
}
```

---

# 9. Render Pass Commands

## 9.1 BeginRenderPass

```
BeginRenderPass {
  id,
  encoder,
  colorAttachments: [
    {
      view,
      resolveTarget?,
      loadOp,          // "load"|"clear"
      storeOp,         // "store"|"discard"
      clearValue?
    }
  ],
  depthStencilAttachment?: {
    view,
    depthLoadOp?, depthStoreOp?, depthClearValue?,
    stencilLoadOp?, stencilStoreOp?, stencilClearValue?
  }
}
```

Backend:

* Vulkan: mapped to `DvzRendering` + `dvz_cmd_rendering_begin`.

---

## 9.2 Render-pass commands

```
SetPipeline { pass, pipeline }
SetBindGroup { pass, index, bindGroup, dynamicOffsets? }
SetViewport { pass, x, y, width, height, minDepth, maxDepth }
SetScissor { pass, x, y, width, height }
SetBlendConstant { pass, r, g, b, a }
SetStencilReference { pass, value }

Draw { pass, vertexCount, instanceCount?, firstVertex?, firstInstance? }
DrawIndexed { pass, indexCount, instanceCount?, firstIndex?, baseVertex?, firstInstance? }
DrawIndirect { pass, buffer, offset, count? }
DrawIndexedIndirect { pass, buffer, offset, count? }
```

---

## 9.3 EndRenderPass

```
EndRenderPass { pass }
```

---

# 10. Compute Pass Commands

## 10.1 BeginComputePass

```
BeginComputePass { id, encoder }
```

---

## 10.2 Compute-pass commands

```
SetPipeline { pass, pipeline }
SetBindGroup { pass, index, bindGroup, dynamicOffsets? }
DispatchWorkgroups { pass, x, y, z }
DispatchWorkgroupsIndirect { pass, buffer, offset }
```

---

## 10.3 EndComputePass

```
EndComputePass { pass }
```

---

# 11. Copy and Blit Commands

```
CopyBufferToBuffer { src, srcOffset, dst, dstOffset, size }
CopyBufferToTexture { src, srcOffset, dstTexture, origin, size, bytesPerRow, rowsPerImage }
CopyTextureToBuffer { srcTexture, origin, size, dst, dstOffset, bytesPerRow, rowsPerImage }
```

Semantics identical to WebGPU.

---

# 12. Submission

## QueueSubmit

```
QueueSubmit {
  queue,
  commandBuffers: [ids]
}
```

Backend:

* Vulkan: `vkQueueSubmit2`
* WebGPU: `queue.submit([cmdBuffers])`

---

# 13. Synchronization & Barriers

## Default: implicit WebGPU-style tracking

The renderer:

* Tracks resource usage (copy, attachment, sampled, storage, etc.)
* Inserts necessary barriers automatically

No DRP command is required for this.

---

## Optional explicit barriers (requires `vulkan_barriers`)

```
ResourceBarrier {
  resource,
  oldState?,
  newState?,
  srcStage?,
  dstStage?,
  srcAccess?,
  dstAccess?
}
```

Backend mapping:
Vulkan → vklite `DvzBarriers` and `dvz_cmd_barriers`.

Ignored by WebGPU backend.

---

# 14. Shader Language Handling

| Format | Vulkan Backend                   | WebGPU Backend                         |
| ------ | -------------------------------- | -------------------------------------- |
| WGSL   | needs transpile → SPIR-V or Naga | native (preferred)                     |
| SPIR-V | native                           | optional (depending on implementation) |
| GLSL   | compile via glslang → SPIR-V     | not accepted                           |

Recommended workflow:

* Author in GLSL or WGSL.
* Precompile GLSL → SPIR-V and/or WGSL.
* DRP uses WGSL for WebGPU, SPIR-V for Vulkan.

---

# 15. Validation Rules

Examples (subset):

* Using an object before creation → error
* Destroyed object referenced → error
* Bind group layout mismatch → validation error
* Texture format incompatible with usage → error
* Using same resource as both color attachment and sampled texture in same pass → forbidden
* Binding buffer with missing or too-small range → error
* Draw commands require render pipeline bound
* Dispatch commands require compute pipeline bound

Renderer emits:

```
Error {
  severity: "error"|"warning",
  message: string
}
```

---

# 16. Backend Mapping Summary

## 16.1 WebGPU Backend

* DRP objects → direct WebGPU equivalents
* Shader language → WGSL only
* Barriers ignored
* Push constants unsupported (ignored)
* Command structure maps 1:1 to WebGPU API calls

---

## 16.2 Vulkan/vklite Backend

* GPUBuffer → suballocation inside large `DvzBuffer`
* GPUTexture → `DvzImages`
* GPUTextureView → `DvzImageViews`
* Sampler → `DvzSampler`
* Bind groups → `DvzDescriptors`
* Pipelines → `DvzGraphics`, `DvzCompute`
* Render passes → `DvzRendering` + `dvz_cmd_rendering_begin/end`
* Barriers → `DvzBarriers` (optional explicit)
* Shaders → SPIR-V preferred; GLSL allowed as extension
* QueueSubmit → `dvz_submit` → `vkQueueSubmit2`

---

# 17. Future Extensions

Potential upcoming optional features:

* **timeline_semaphores** (explicit work submission control)
* **multi-queue** (compute & transfer queue separation)
* **mesh shaders / compute-driven rendering**
* **acceleration structures (Ray Tracing)**
* **video encode/decode interop**

These must be negotiated through the extension mechanism.
