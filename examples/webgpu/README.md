# DRP2 WebGPU PoC

This directory contains an experimental browser WebGPU replayer for a very small DRP2 stream.

Run from the repository root:

```bash
python3 -m http.server 8765
```

Then open:

```text
http://localhost:8765/examples/webgpu/
```

The fixture dashboard runs the committed positive DRP2 fixture manifest against the browser WebGPU
runner:

```text
http://localhost:8765/examples/webgpu/fixtures.html
```

Refresh the committed fixture manifest after adding or removing positive fixtures:

```bash
node examples/webgpu/generate_fixture_manifest.mjs
```

The default stream is `streams/indexed_quad_wgsl.json`. It renders a four-vertex quad through
`SetIndexBuffer` and `DrawIndexed`. Use the page menu to switch streams.

The earlier smoke streams remain available:

```text
http://localhost:8765/examples/webgpu/?stream=hello_triangle_wgsl
http://localhost:8765/examples/webgpu/?stream=triangle_vertex_buffer_wgsl
http://localhost:8765/examples/webgpu/?stream=triangle_offscreen_readback_wgsl
http://localhost:8765/examples/webgpu/?stream=depth_overlap_wgsl
http://localhost:8765/examples/webgpu/?stream=texture_sampling_wgsl
http://localhost:8765/examples/webgpu/?stream=indexed_quad_wgsl
```

All streams use `texture_id: 0` as a PoC-local alias for the current browser canvas texture.

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
- `Draw`
- `DrawIndexed`
- `EndRenderPass`
- `CopyBufferToTexture`
- `CopyTextureToBuffer`
- `CopyTextureToTexture`
- `FinishCommandEncoder`
- `QueueSubmit`
- `QueueSubmitReply`, `Error`, and destroy commands are accepted as no-op fixture markers.

Manual checks:

- Default stream: the canvas should show a single indexed quad with smoothly interpolated corner
  colors.
- Texture stream: the canvas should show four large color regions from the uploaded 2x2 texture.
- Depth stream: the smaller green triangle must appear in front of the larger red triangle where
  they overlap.
- Offscreen readback stream: the status line should include `readback nonzero=` with a nonzero value.
- No-buffer and vertex-buffer streams should still render a single RGB triangle.

The next useful slice is uniform-buffer support in bind groups.
