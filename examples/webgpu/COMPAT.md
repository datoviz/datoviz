# DRP2 WebGPU Compatibility

This note records the current browser WebGPU proof-of-concept compatibility surface.

The WebGPU runner is a strict subset check, not a full DRP2 backend. It tracks the active DRP2
command surface and validates the currently portable fixture slice: `37` positive DRP2 fixtures,
`2` WebGPU-only attachment streams, and `81` expected-failure semantic negative fixtures.

As of the capability-preflight slice (`c03e89227`), the pure browser WebGPU runner is considered
closed for the v0.4 RC experimental subset. Remaining WebGPU/WASM release work is scene/WASM
emission and transport, not additional pure WebGPU command coverage for this subset.


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
- recorded browser dashboard result on 2026-05-28 after the repeated-runtime-frame smoke slice
  (`183812f27`): `120 pass, 0 unsupported, 0 fail`
- recorded browser dashboard result on 2026-05-29 after the retained browser-runtime stress slice
  (`292e82899`): fixture compatibility `120 pass, 0 unsupported, 0 fail`; retained runtime stress
  `4 pass, 0 fail`
- recorded browser dashboard result on 2026-05-29 after the demo-runtime reload stress slice
  (`a1c0d7306`): fixture compatibility `120 pass, 0 unsupported, 0 fail`; retained runtime stress
  `7 pass, 0 fail`
- recorded browser dashboard result on 2026-05-29 after the capability-preflight diagnostics slice
  (`c03e89227`): fixture compatibility `120 pass, 0 unsupported, 0 fail`; retained runtime stress
  `7 pass, 0 fail`
- recorded manual browser result on 2026-05-30 after the generic WASM subset documentation,
  diagnostic ABI, and panzoom metadata slices (`9f5e93bbb`): fixture compatibility
  `120 pass, 0 unsupported, 0 fail`; retained runtime stress `7 pass, 0 fail`; 2D WASM
  point/primitive/image/mesh page rendered and pan/zoom worked; 3D WASM cube page rendered and
  arcball interaction worked.
- recorded manual browser result on 2026-05-30 after WASM ABI diagnostic hardening: fixture
  compatibility `120 pass, 0 unsupported, 0 fail`; retained runtime stress `7 pass, 0 fail`; 2D
  WASM point/primitive/image/mesh page rendered and pan/zoom worked; 3D WASM cube page rendered and
  arcball interaction worked.
- recorded browserless WASM robustness proof on 2026-05-31: `just wasm-scene-smoke` validates
  visual-family stream shape for point, primitive, RGBA8 image, basic mesh, 2D update streams, and
  3D mesh/arcball update streams; generated 2D and 3D WASM streams pass WebGPU fixture preflight and
  execute through the JS WebGPU runner's repeated-frame resource-stability smoke.
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
- `ResourceBarrier` as a validated ordering marker
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


## Capability Reporting

`initWebGPU()` returns a DRP2-shaped capability snapshot alongside the adapter device, canvas
context, and canvas format. The browser runtime uses that snapshot as the default capability set and
lets fixture-level `capabilities` entries narrow it for negative tests. The current snapshot reports:

- `supported_shader_formats`: `wgsl`
- `supported_texture_formats`: the committed WebGPU texture subset plus the preferred canvas format
- `supported_sample_counts`: `1` and `4`
- `max_texture_dimension_2d` when exposed by the WebGPU device limits
- `supports_fp64`: `false`

The fixture dashboard stores this snapshot in the summary tooltip so browser runs can report the
capability context that was used for validation.

The runner preflights stream-level capabilities before command execution. Unsupported commands and
unsupported shader, texture, render-target, or depth/stencil capability choices return DRP2-level
diagnostics before creating browser GPU resources.


## Browser Canvas Target Conventions

The browser runner has an explicit external-target convention for WebGPU presentation streams:

- `texture_id: 0` in a render-pass color attachment means the current browser canvas texture.
- `texture_id: 0` in a render-pass depth/stencil attachment means a transient browser-owned
  `depth32float` attachment matching the pass extent.
- Pipeline color target format `"canvas"` means `navigator.gpu.getPreferredCanvasFormat()`.
- Texture dimensions `"canvas"` for width/height mean the current canvas pixel extent. The runner
  and fixture preflight require both width and height to use the alias together.

## PoC-Local Adaptations

These are compatibility choices in the browser runner for ad hoc demo streams and older command
forms. The fixture dashboard requires explicit bind-group layout and render-pipeline metadata.

- The main demo page can keep persistent WebGPU resources for a loaded stream and replay only the
  frame command slice after the first command encoder. The strict fixture dashboard still executes
  each stream as a one-shot command list.
- Missing `CreateRenderPipeline.color_targets` follows the DRP2 default for ad hoc developer
  streams: the configured canvas format for canvas targets, otherwise `rgba8unorm` when no
  attachment format is available. Committed browser streams now carry explicit color targets.
- Missing `vertex_buffers` means vertex pulling or builtins in DRP2. The historical one-slot
  compatibility fallback for shaders declaring `@location(0)` is now opt-in through the main demo
  session only; normal runner and fixture paths reject that missing metadata.
- Tight `CopyTextureToBuffer.bytes_per_row` values are adapted through an aligned temporary buffer
  because WebGPU requires copy row pitch to be a multiple of 256 bytes.
- Buffer binding offsets must be WebGPU-aligned in the browser runner. DRP2 streams that use
  non-aligned offsets are rejected explicitly instead of silently binding a different range.
- The live pan/zoom dropdown entry reuses the scene-generated point stream and discovers its MVP and
  viewport uniform buffers from `metadata.datoviz.interactive_uniforms.panzoom`. This metadata is
  browser-demo metadata; it is not an executable DRP2 command.
- Bind-group layout `visibility`, storage `access`, render-pipeline `vertex_buffers`, and
  render-pipeline `color_targets` are required by the fixture dashboard and present in committed
  browser streams. Shader input vertex-buffer inference is limited to the explicit
  demo-compatibility option.
- The fixture dashboard retained runtime stress section loads `scene_point_wgsl`,
  `scene_primitive_wgsl`, `texture_sampling_wgsl`, and `attachment_depth_wgsl` once each, renders
  `10` repeated frames through `Drp2WebGpuRuntime`, and checks stable resource counts with no open
  or recorded references after every frame.
- The fixture dashboard demo-path stress rows drive the same `WebGpuDemoSession` used by the main
  demo page through pan/zoom uniform updates, resize-triggered reload, and stream reload while
  checking stable persistent resource counts and no open or recorded references.
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
- dynamic buffer offsets remain DRP2 offsets; the browser runner currently validates WebGPU
  alignment and rejects unsupported offsets rather than silently rebasing them.
- interactive demo uniform targets are now stream metadata, so demo code no longer carries
  scene-emitted buffer ids in the dropdown configuration.

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
