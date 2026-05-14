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

The default stream is `streams/triangle_offscreen_readback_wgsl.json`. It renders a triangle to an
offscreen texture, copies the texture to a readback buffer, then renders the same triangle to the
visible canvas.

The earlier smoke streams remain available:

```text
http://localhost:8765/examples/webgpu/?stream=hello_triangle_wgsl
http://localhost:8765/examples/webgpu/?stream=triangle_vertex_buffer_wgsl
```

All streams use `texture_id: 0` as a PoC-local alias for the current browser canvas texture.

Supported commands in this first slice:

- `HelloRenderer`
- `RendererHelloReply`
- `CreateBuffer`
- `WriteBuffer`
- `CreateTexture`
- `CreateShaderModule`
- `CreateRenderPipeline`
- `BeginCommandEncoder`
- `BeginRenderPass`
- `SetPipeline`
- `SetVertexBuffer`
- `Draw`
- `EndRenderPass`
- `CopyTextureToBuffer`
- `FinishCommandEncoder`
- `QueueSubmit`
- `QueueSubmitReply` is accepted as a no-op fixture/reply marker.

The next useful slice is depth support: a depth texture attachment, `depth_stencil` pipeline state,
and a small two-triangle or cube-style ordering smoke.
