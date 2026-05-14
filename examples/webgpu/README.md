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

The first stream is `streams/hello_triangle_wgsl.json`. It uses WGSL shader modules and `texture_id:
0` as a PoC-local alias for the current browser canvas texture.

Supported commands in this first slice:

- `HelloRenderer`
- `RendererHelloReply`
- `CreateShaderModule`
- `CreateRenderPipeline`
- `BeginCommandEncoder`
- `BeginRenderPass`
- `SetPipeline`
- `Draw`
- `EndRenderPass`
- `FinishCommandEncoder`
- `QueueSubmit`

The next useful slice is vertex-buffer support: `CreateBuffer`, `WriteBuffer`, and
`SetVertexBuffer`.
