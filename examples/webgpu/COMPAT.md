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

The committed manifest currently covers the `35` positive DRP2 fixtures under
`spec/drp2/fixtures/positive`.

Current status as of this note:

- positive fixture count: `35`
- last manual dashboard run before adding C-emitted WGSL scene fixtures:
  `32 pass, 0 unsupported, 0 fail`
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
- `point-list`, `line-list`, `line-strip`, `triangle-list`, and `triangle-strip` topology
- `uint16` and `uint32` index buffers
- sampled texture bindings
- sampler bindings
- uniform buffer bindings
- storage buffer bindings
- dynamic buffer offsets by materializing adjusted bind groups at `SetBindGroup` time
- compute pipelines, compute passes, and direct workgroup dispatch for the positive fixture subset


## PoC-Local Adaptations

These are compatibility choices in the browser runner for ad hoc demo streams and older command
forms. The fixture dashboard requires explicit bind-group layout and render-pipeline metadata.

- The main demo page can keep persistent WebGPU resources for a loaded stream and replay only the
  frame command slice after the first command encoder. The strict fixture dashboard still executes
  each stream as a one-shot command list.
- `texture_id: 0` means the current browser canvas texture.
- Pipeline color target format `"canvas"` means `navigator.gpu.getPreferredCanvasFormat()`.
- Texture dimensions `"canvas"` for width/height mean the current canvas pixel extent.
- Missing `CreateRenderPipeline.color_targets` follows the DRP2 default for standalone demo streams:
  the configured canvas format for canvas targets, otherwise `rgba8unorm` when no attachment format
  is available.
- Missing `vertex_buffers` means vertex pulling or builtins in DRP2. Standalone demo streams still
  get a one-slot compatibility fallback when their vertex shader declares `@location(0)`.
- Tight `CopyTextureToBuffer.bytes_per_row` values are adapted through an aligned temporary buffer
  because WebGPU requires copy row pitch to be a multiple of 256 bytes.
- Buffer binding offsets that are valid in DRP2 but not aligned for WebGPU are bound from offset 0
  in the PoC so fixture command paths can still execute.
- The live pan/zoom dropdown entry reuses the scene-generated point stream and updates the MVP and
  viewport uniform buffers by their current scene-emitted ids. This is demo metadata, not a stable
  DRP2 contract.
- Bind-group layout `visibility`, storage `access`, render-pipeline `vertex_buffers`, and
  render-pipeline `color_targets` are required by the fixture dashboard. Standalone demo streams
  still use DRP2 defaults and shader-source inference as compatibility fallbacks.
- Destroy commands are accepted as no-op lifecycle markers; the PoC relies on JavaScript object
  lifetime instead of implementing DRP2 object-use lifetime validation.


## DRP2 Contract Gaps Exposed

The first WebGPU pass resolved these DRP2 portability questions in the protocol notes:

- `CreateRenderPipeline.color_targets` remains optional with a backend-selected default.
- `CreateRenderPipeline.vertex_buffers` remains optional, but portable producers should provide it
  when the vertex shader declares user input locations.
- bind-group layout entries may now carry `visibility`.
- storage layout entries may now carry `access` (`read` or `read_write`).
- tight `CopyTextureToBuffer.bytes_per_row` remains valid DRP2, with backend adaptation allowed when
  a backend requires stricter row-pitch alignment.
- dynamic buffer offsets remain DRP2 offsets; backends must validate alignment or adapt by
  materializing an equivalent aligned binding.

The positive fixture dashboard now runs without bind-group or render-pipeline metadata fallbacks.
The same strict fixture assumptions are checked without a browser by:

```bash
just webgpu-fixture-preflight
```
