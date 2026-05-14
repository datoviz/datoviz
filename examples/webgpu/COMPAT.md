# DRP2 WebGPU Compatibility

This note records the current browser WebGPU proof-of-concept compatibility surface.


## Fixture Dashboard

Run from the repository root:

```bash
python3 -m http.server 8765
```

Then open:

```text
http://localhost:8765/examples/webgpu/fixtures.html
```

The committed manifest currently covers the `32` positive DRP2 fixtures under
`spec/drp2/fixtures/positive`.

Current status as of this note:

- positive fixture count: `32`
- last manual dashboard run: `32 pass, 0 unsupported, 0 fail`
- remaining unsupported positive fixture group: none in the committed positive fixture manifest


## Supported Commands

The PoC currently executes these DRP2 commands:

- `HelloRenderer`
- `RendererHelloReply`
- `Error` as a no-op diagnostic marker
- `CreateBuffer`
- `DestroyBuffer` as a no-op lifecycle marker
- `WriteBuffer`
- `CreateTexture`
- `DestroyTexture` as a no-op lifecycle marker
- `CreateTextureView`
- `DestroyTextureView` as a no-op lifecycle marker
- `WriteTexture`
- `CreateSampler`
- `DestroySampler` as a no-op lifecycle marker
- `CreateBindGroupLayout`
- `DestroyBindGroupLayout` as a no-op lifecycle marker
- `CreateBindGroup`
- `DestroyBindGroup` as a no-op lifecycle marker
- `CreateShaderModule`
- `DestroyShaderModule` as a no-op lifecycle marker
- `CreateRenderPipeline`
- `DestroyRenderPipeline` as a no-op lifecycle marker
- `BeginCommandEncoder`
- `FinishCommandEncoder`
- `BeginRenderPass`
- `EndRenderPass`
- `SetPipeline` for render passes
- `SetVertexBuffer`
- `SetIndexBuffer`
- `SetBindGroup`
- `Draw`
- `DrawIndexed`
- `CopyBufferToTexture`
- `CopyTextureToBuffer`
- `CopyTextureToTexture`
- `QueueSubmit`
- `QueueSubmitReply` as a no-op reply marker


## Unsupported Commands

All commands used by the committed positive fixture manifest are currently supported.

Indirect draw/dispatch commands and explicit pipeline-layout/resource-barrier commands are not active in
the current DRP2 command surface and are not implemented in the PoC.


## Supported Fields And Narrow Mappings

The PoC supports the fixture subset of:

- WGSL shader modules only
- `rgba8unorm`, `bgra8unorm`, and `depth32float` textures
- `r32uint` integer render targets for picking-style readback fixtures
- one color attachment per render pass
- `triangle-list` topology
- `uint16` and `uint32` index buffers
- sampled texture bindings
- sampler bindings
- uniform buffer bindings
- storage buffer bindings
- dynamic buffer offsets by materializing adjusted bind groups at `SetBindGroup` time
- compute pipelines, compute passes, and direct workgroup dispatch for the positive fixture subset


## PoC-Local Adaptations

These are compatibility choices in the browser runner, not stable DRP2 semantics.

- `texture_id: 0` means the current browser canvas texture.
- Pipeline color target format `"canvas"` means `navigator.gpu.getPreferredCanvasFormat()`.
- Texture dimensions `"canvas"` for width/height mean the current canvas pixel extent.
- Missing `CreateRenderPipeline.color_targets` defaults to `rgba8unorm`.
- Missing `vertex_buffers` with a vertex shader using `@location(0)` defaults to one
  `float32x3` vertex attribute at slot 0.
- Tight `CopyTextureToBuffer.bytes_per_row` values are adapted through an aligned temporary buffer
  because WebGPU requires copy row pitch to be a multiple of 256 bytes.
- Buffer binding offsets that are valid in DRP2 but not aligned for WebGPU are bound from offset 0
  in the PoC so fixture command paths can still execute.
- Storage-buffer layout visibility is narrowed to fragment and compute stages because WebGPU rejects
  read-write storage buffers visible to the vertex stage.
- Destroy commands are accepted as no-op lifecycle markers; the PoC relies on JavaScript object
  lifetime instead of implementing DRP2 object-use lifetime validation.


## DRP2 Contract Gaps Exposed

The WebGPU PoC has exposed several portability-significant questions:

- Should `CreateRenderPipeline.color_targets` be required, or should DRP2 define an official default?
- Should `CreateRenderPipeline.vertex_buffers` be required whenever vertex input locations are used?
- Should bind group layout entries carry shader-stage visibility?
- Should storage buffer layout entries distinguish read-only from read-write access?
- Should DRP2 require WebGPU-compatible row pitch for texture-to-buffer copies, or explicitly allow
  backend adaptation for tight rows?
- Should dynamic buffer offsets specify backend alignment requirements, or should runtimes always adapt?

These should be resolved in the DRP2 spec before treating WebGPU as more than a portability probe.
