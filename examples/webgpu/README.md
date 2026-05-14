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

The default stream is `streams/triangle_vertex_buffer_wgsl.json`. It uses WGSL shader modules,
`CreateBuffer`, `WriteBuffer`, an explicit vertex layout, and `SetVertexBuffer`.

The first no-buffer smoke stream remains available:

```text
http://localhost:8765/examples/webgpu/?stream=hello_triangle_wgsl
```

Both streams use `texture_id: 0` as a PoC-local alias for the current browser canvas texture.

Supported commands in this first slice:

- `HelloRenderer`
- `RendererHelloReply`
- `CreateBuffer`
- `WriteBuffer`
- `CreateShaderModule`
- `CreateRenderPipeline`
- `BeginCommandEncoder`
- `BeginRenderPass`
- `SetPipeline`
- `SetVertexBuffer`
- `Draw`
- `EndRenderPass`
- `FinishCommandEncoder`
- `QueueSubmit`

The next useful slice is offscreen texture support: `CreateTexture`, render-pass attachments by
texture id, and `CopyTextureToBuffer` readback.
