# DRP2 WebGPU Compatibility

This note records the current browser WebGPU proof-of-concept compatibility surface.

The WebGPU runner is a strict subset check, not a full DRP2 backend. It tracks the active DRP2
command surface and validates the currently portable fixture slice: `37` positive DRP2 fixtures,
`2` WebGPU-only attachment streams, and `81` expected-failure semantic negative fixtures.


## Fixture Dashboard

Run from the repository root:

```bash
python3 -m http.server 8765
```

Then open:

```text
http://localhost:8765/examples/webgpu/fixtures.html
```

The committed dashboard manifest currently covers:

- `37` positive DRP2 fixtures under `spec/drp2/fixtures/positive`
- `2` WebGPU-only strict stream checks under `examples/webgpu/streams`
- `81` semantic negative fixtures under `spec/drp2/fixtures/negative`
- `120` total dashboard rows

Current status as of this note:

- positive fixture count: `37`
- WebGPU stream count: `2`
- negative parity fixture count: `81`
- expected browser dashboard result for the committed subset: `120 pass, 0 unsupported, 0 fail`
- remaining unsupported entries in the committed subset: none

This subset is intentionally labeled as the "WebGPU fixture subset": passing it means the browser
runner can execute the committed portable fixtures and WebGPU-specific attachment probes, and reject
the semantic negative fixtures with the expected `commandIndex`, `cmd`, and `code`. It does not mean
deferred DRP2 commands or every future schema command are browser-supported.


## Supported Commands

The PoC currently executes these DRP2 commands:

- `HelloRenderer`
- `RendererHelloReply`
- `Error` as a no-op diagnostic marker
- `CreateBuffer`
- `DestroyBuffer`
- `WriteBuffer`
- `CreateTexture`
- `DestroyTexture`
- `CreateTextureView`
- `DestroyTextureView`
- `WriteTexture`
- `CreateSampler`
- `DestroySampler`
- `CreateBindGroupLayout`
- `DestroyBindGroupLayout`
- `CreateBindGroup`
- `DestroyBindGroup`
- `CreateShaderModule`
- `DestroyShaderModule`
- `CreateRenderPipeline`
- `DestroyRenderPipeline`
- `CreateComputePipeline`
- `DestroyComputePipeline`
- `BeginCommandEncoder`
- `FinishCommandEncoder`
- `BeginRenderPass`
- `EndRenderPass`
- `BeginComputePass`
- `EndComputePass`
- `SetPipeline` for render and compute passes
- `SetVertexBuffer`
- `SetIndexBuffer`
- `SetBindGroup`
- `SetViewport`
- `SetScissor`
- `SetBlendConstant`
- `SetStencilReference`
- `Draw`
- `DrawIndexed`
- `DispatchWorkgroups`
- `CopyBufferToBuffer`
- `CopyBufferToTexture`
- `CopyTextureToBuffer`
- `CopyTextureToTexture`
- `QueueSubmit`
- `QueueSubmitReply` as a no-op reply marker


## Unsupported Commands

All active commands listed in `spec/drp2/schema/README.md` currently have a WebGPU runner switch case,
either as executable behavior or as explicit lifecycle/diagnostic handling.

The following schema files are deferred and non-authoritative for the current DRP2 command surface, so
the WebGPU runner rejects them through the unsupported-command path if they appear in an ad hoc stream:

- `CreatePipelineLayout`
- `DestroyPipelineLayout`
- `ResourceBarrier`
- `DrawIndirect`
- `DrawIndexedIndirect`
- `DispatchWorkgroupsIndirect`


## Supported Fields And Narrow Mappings

The PoC supports the fixture subset of:

- WGSL shader modules only
- `rgba8unorm`, `bgra8unorm`, and `depth32float` textures
- `r32uint` integer render targets for picking-style readback fixtures
- one or more color attachments per render pass for the committed fixture subset
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
- Destroy commands validate live-object dependencies, use-after-destroy, recorded-work references,
  and submitted-work references. The runner calls native WebGPU `destroy()` only for object types
  that expose it and otherwise tombstones the DRP2 object id.


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

The broader browserless WebGPU smoke path also executes the `37 + 2` subset with a fake WebGPU device
and checks the manifest's `81` DRP2 semantic negative fixtures for parity of `commandIndex`, `cmd`,
and `code`:

```bash
just webgpu-runner-smoke
```

Both checks are part of `just spec-check`.
