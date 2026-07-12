const configuredCanvasContexts = new WeakMap();

function supportedTextureFormats(canvasFormat) {
  const formats = [
    "r8unorm",
    "r16float",
    "r32uint",
    "r32sint",
    "rg32uint",
    "rgba8unorm",
    "bgra8unorm",
    "rgba16float",
    "depth32float",
  ];
  if (canvasFormat !== undefined && !formats.includes(canvasFormat)) {
    formats.push(canvasFormat);
  }
  return formats;
}

export function runtimeCapabilities(device, canvasFormat, adapter = null) {
  const limits = device?.limits ?? adapter?.limits ?? {};
  const maxTextureDimension2d = limits.maxTextureDimension2D ?? limits.maxTextureDimension2d;
  const maxBindGroups = limits.maxBindGroups;
  const maxVertexBuffers = limits.maxVertexBuffers;
  const maxBufferSize = limits.maxBufferSize;
  const capabilities = {
    supported_shader_formats: ["wgsl"],
    supported_texture_formats: supportedTextureFormats(canvasFormat),
    supported_sample_counts: [1, 4],
    supports_color_blending: true,
    supports_fp64: false,
    min_texture_copy_bytes_per_row_alignment: 256,
  };
  if (Number.isFinite(maxTextureDimension2d)) {
    capabilities.max_texture_dimension_2d = maxTextureDimension2d;
  }
  if (Number.isFinite(maxBindGroups)) {
    capabilities.max_bind_groups = maxBindGroups;
  }
  if (Number.isFinite(maxVertexBuffers)) {
    capabilities.max_vertex_buffers = maxVertexBuffers;
  }
  if (Number.isFinite(maxBufferSize)) {
    capabilities.max_buffer_size = maxBufferSize;
  }
  return capabilities;
}

export function resizeCanvasToDisplaySize(canvas, device, context, format) {
  const scale = Math.max(1, window.devicePixelRatio || 1);
  const width = Math.max(1, Math.floor(canvas.clientWidth * scale));
  const height = Math.max(1, Math.floor(canvas.clientHeight * scale));
  const resized = canvas.width !== width || canvas.height !== height;
  const configured = configuredCanvasContexts.get(context) ?? null;
  const needsConfigure = (
    resized ||
    configured === null ||
    configured.device !== device ||
    configured.format !== format
  );
  if (!needsConfigure) {
    return false;
  }
  if (resized) {
    canvas.width = width;
    canvas.height = height;
  }
  context.configure({
    device,
    format,
    alphaMode: "opaque",
  });
  configuredCanvasContexts.set(context, { device, format });
  return resized;
}

export function resizeWebGpuCanvas(canvas, device, context, format) {
  return resizeCanvasToDisplaySize(canvas, device, context, format);
}

export async function initWebGPU(canvas) {
  if (canvas === undefined || canvas === null) {
    throw new Error("initWebGPU needs a canvas");
  }
  if (!navigator.gpu) {
    throw new Error("WebGPU is not available in this browser");
  }

  const adapter = await navigator.gpu.requestAdapter();
  if (!adapter) {
    throw new Error("no WebGPU adapter is available");
  }

  const device = await adapter.requestDevice();
  const context = canvas.getContext("webgpu");
  if (!context) {
    throw new Error("failed to create a WebGPU canvas context");
  }

  const format = navigator.gpu.getPreferredCanvasFormat();
  resizeCanvasToDisplaySize(canvas, device, context, format);
  const capabilities = runtimeCapabilities(device, format, adapter);

  return { device, context, format, capabilities };
}
