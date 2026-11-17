# DRP2 WebGPU PoC

This directory contains an experimental browser WebGPU replayer for a narrow DRP2 fixture subset.

RC3 ships the existing polygon paint rendering with rendered paint queries explicitly unsupported panel-wide while drawable paint is present; hidden/empty paint, disabled draw spans, and provably zero effective material alpha are exceptions. Composed GPU paint picking and WebGPU device ownership/loss replacement are [RC4 work](https://github.com/datoviz/datoviz/blob/main/spec/scene/slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md). Current `recoverRuntime()` recreates the runtime on the same device and does not replace a lost `GPUDevice`.

The bounded 2026-11-17 investigation isolates the previous instance-loss signature to default Chrome canvas presentation in this Linux environment: plain offscreen WebGPU/DRP2 readback passes, while default canvas presentation cannot allocate its swapchain backing image. An explicit Vulkan browser profile passes polygon and choropleth rendering in headless and headed/Xvfb modes on Chrome 156.0.8078.79 with non-fallback NVIDIA Blackwell WebGPU. This is configured-provider development evidence, not physical interaction or universal browser support. The deeper device-replacement work remains RC4 scope.

See [the configured browser reproduction commands](COMPAT.md#rc3-browser-reproduction-2026-11-17), including strict `--require-render` acceptance, before treating a provider skip as rendering proof.

Run from the repository root:

```bash
python3 -m http.server 8765
```

Then open:

```text
http://localhost:8765/examples/webgpu/
```

The fixture dashboard runs the committed positive DRP2 fixture manifest, WebGPU-specific attachment
streams, and semantic negative fixture parity checks against the browser WebGPU runner:

```text
http://localhost:8765/examples/webgpu/fixtures.html
```

The WASM examples build the generic scene ABI with Emscripten, route pointer input through compiled
controllers, and execute emitted DRP2 packets with the browser WebGPU runtime. Reusable browser
bridge/session code lives under `web/wasm/`. Public live examples use `live.html?id=<example_id>`
and must be backed by the same canonical C scenario as native validation:

```bash
just wasm-scene-smoke
just webgpu-browser-smoke
```

```text
http://localhost:8765/examples/webgpu/live.html?id=features_timer_animation
http://localhost:8765/examples/webgpu/live.html?id=features_picking
http://localhost:8765/examples/webgpu/live.html?id=features_image_probe
```

The broader `examples.html` routes are development samplers for renderer and ABI work. They are not
the public WebGPU promotion surface:

```text
http://localhost:8765/examples/webgpu/examples.html?demo=wasm-2d
http://localhost:8765/examples/webgpu/examples.html?demo=wasm-3d
```

Refresh the committed fixture manifest after adding or removing positive or negative fixtures:

```bash
node examples/webgpu/generate_fixture_manifest.mjs
```

The default stream is `streams/indexed_quad_wgsl.json`. It renders a four-vertex quad through
`SetIndexBuffer` and `DrawIndexed`. Use the page menu to switch streams.

Developer DVZR recordings can be adapted to the browser stream shape:

```bash
python3 tools/dvzr_to_webgpu_stream.py \
    build/examples/c/visuals/mesh.dvzr \
    examples/webgpu/streams/mesh_dvzr_wgsl.json \
    --frames 1:600 \
    --frame-stride 25
```

The adapter remaps large native ids to JavaScript-safe ids, replaces known built-in GLSL/SPIR-V
shader modules with WGSL variants, maps the recorded render target to the browser canvas, and may
encode large binary payloads as `base64+gzip`. For DVZR recordings, the adapter also stores the
recorded canvas extent so the browser page can preserve the original replay aspect ratio. Streams
with a `frames` table can be played with the page's Play button.

The earlier smoke streams remain available:

```text
http://localhost:8765/examples/webgpu/?stream=mesh_dvzr_wgsl
http://localhost:8765/examples/webgpu/?stream=triangle_wgsl
http://localhost:8765/examples/webgpu/?stream=triangle_vertex_buffer_wgsl
http://localhost:8765/examples/webgpu/?stream=triangle_offscreen_readback_wgsl
http://localhost:8765/examples/webgpu/?stream=depth_overlap_wgsl
http://localhost:8765/examples/webgpu/?stream=texture_sampling_wgsl
http://localhost:8765/examples/webgpu/?stream=indexed_quad_wgsl
http://localhost:8765/examples/webgpu/?stream=scene_primitive_wgsl
http://localhost:8765/examples/webgpu/?stream=scene_point_wgsl
http://localhost:8765/examples/webgpu/?stream=scene_image_wgsl
http://localhost:8765/examples/webgpu/?stream=attachment_multi_color_wgsl
http://localhost:8765/examples/webgpu/?stream=attachment_depth_wgsl
```

Browser presentation streams use `texture_id: 0` in render-pass color attachments as the current
browser canvas texture, and may use `"canvas"` width/height together for browser-sized textures.
Strict dashboard fixtures use explicit resources except for browser-canvas streams.

Supported commands in this first slice:

- `HelloRenderer`
- `RendererHelloReply`
- `CreateBuffer`
- `WriteBuffer`
- `CreateTexture`
- `CreateTextureView`
- `WriteTexture`
- `CreateSampler`
- `CreateBindGroupLayout`
- `CreateBindGroup`
- `CreateShaderModule`
- `CreateRenderPipeline`
- `BeginCommandEncoder`
- `BeginRenderPass`
- `SetPipeline`
- `SetVertexBuffer`
- `SetIndexBuffer`
- `SetBindGroup`
- `SetViewport`
- `SetScissor`
- `SetBlendConstant`
- `SetStencilReference`
- `Draw`
- `DrawIndexed`
- `EndRenderPass`
- `CopyBufferToBuffer`
- `CopyBufferToTexture`
- `CopyTextureToBuffer`
- `CopyTextureToTexture`
- `FinishCommandEncoder`
- `QueueSubmit`
- `QueueSubmitReply` and `Error` are accepted as diagnostic/reply markers.
- Destroy commands validate DRP2 object lifetimes and release browser objects when WebGPU exposes a
  destroy hook.

Browserless validation from the repository root:

```bash
just webgpu-fixture-preflight
just webgpu-runner-smoke
just wasm-scene-smoke
just webgpu-browser-smoke
```

Manual checks:

- Default stream: the canvas should show a single indexed quad with smoothly interpolated corner
  colors.
- Texture stream: the canvas should show four large color regions from the uploaded 2x2 texture.
- Depth stream: the smaller green triangle must appear in front of the larger red triangle where
  they overlap.
- Scene points stream: the canvas should show five circular points emitted as instanced quads.
- Offscreen readback stream: the status line should include `readback nonzero=` with a nonzero value.
- No-buffer and vertex-buffer streams should still render a single RGB triangle.

The main stream page keeps the WebGPU runtime resources alive after loading a stream and replays the
DRP2 frame command slice. Browser interaction for the release-visible scene path is exercised by
`live.html`, where DOM input is routed through the generic WASM scene ABI and emitted DRP2 update
streams.
