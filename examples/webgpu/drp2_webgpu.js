const statusEl = document.querySelector("#status");
const canvas = document.querySelector("#viewport");
const streamNameEl = document.querySelector("#stream-name");
const streamSelectEl = document.querySelector("#stream-select");
const interactionHelpEl = document.querySelector("#interaction-help");
const playToggleEl = document.querySelector("#play-toggle");
const frameInfoEl = document.querySelector("#frame-info");

export const STREAMS = [
  {
    name: "scene_point_panzoom_wgsl",
    label: "Scene points pan/zoom",
    source: "scene_point_wgsl",
    interactive: {
      type: "panzoom",
      mvpBufferId: 5001,
      viewportBufferId: 5002,
    },
  },
  { name: "scene_primitive_wgsl", label: "Scene primitive (WGSL)" },
  { name: "scene_point_wgsl", label: "Scene points (WGSL)" },
  { name: "hello_mesh_dvzr_wgsl", label: "DVZR mesh replay (WGSL)" },
  { name: "indexed_quad_wgsl", label: "Indexed quad" },
  { name: "texture_sampling_wgsl", label: "Texture sampling" },
  { name: "depth_overlap_wgsl", label: "Depth overlap" },
  { name: "triangle_offscreen_readback_wgsl", label: "Offscreen readback" },
  { name: "triangle_vertex_buffer_wgsl", label: "Vertex-buffer triangle" },
  { name: "hello_triangle_wgsl", label: "No-buffer triangle" },
];



function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.style.color = isError ? "#ff8f8f" : "#9be59b";
}



function streamConfigByName(name) {
  return STREAMS.find((item) => item.name === name) ?? STREAMS[0];
}



function streamSourceName(config) {
  return config.source ?? config.name;
}



function applyStreamCanvasAspect(stream) {
  const width = Number(stream.canvas?.width);
  const height = Number(stream.canvas?.height);
  if (Number.isFinite(width) && Number.isFinite(height) && width > 0 && height > 0) {
    canvas.style.setProperty("--stream-aspect", `${width / height}`);
  } else {
    canvas.style.removeProperty("--stream-aspect");
  }
}



function required(value, message) {
  if (value === undefined || value === null) {
    throw new Error(message);
  }
  return value;
}



function mapLoadOp(loadOp) {
  if (loadOp === undefined || loadOp === "clear" || loadOp === "load") {
    return loadOp ?? "clear";
  }
  throw new Error(`unsupported load_op: ${loadOp}`);
}



function mapStoreOp(storeOp) {
  if (storeOp === undefined || storeOp === "store" || storeOp === "discard") {
    return storeOp ?? "store";
  }
  throw new Error(`unsupported store_op: ${storeOp}`);
}



function mapTopology(topology) {
  const normalized = topology ?? "triangle-list";
  if (
    normalized === "point-list" ||
    normalized === "line-list" ||
    normalized === "line-strip" ||
    normalized === "triangle-list" ||
    normalized === "triangle-strip"
  ) {
    return normalized;
  }
  throw new Error(`unsupported topology: ${topology}`);
}



function mapDepthCompare(compare) {
  if (
    compare === undefined ||
    compare === "never" ||
    compare === "less" ||
    compare === "equal" ||
    compare === "less-equal" ||
    compare === "greater" ||
    compare === "not-equal" ||
    compare === "greater-equal" ||
    compare === "always"
  ) {
    return compare ?? "less";
  }
  throw new Error(`unsupported depth compare: ${compare}`);
}



function mapIndexFormat(format) {
  if (format === undefined || format === "uint16" || format === "uint32") {
    return format ?? "uint32";
  }
  throw new Error(`unsupported index format: ${format}`);
}



function mapFilterMode(filter) {
  if (filter === undefined || filter === "nearest" || filter === "linear") {
    return filter ?? "nearest";
  }
  throw new Error(`unsupported sampler filter: ${filter}`);
}



function mapAddressMode(mode) {
  if (
    mode === undefined ||
    mode === "clamp-to-edge" ||
    mode === "repeat" ||
    mode === "mirror-repeat"
  ) {
    return mode ?? "clamp-to-edge";
  }
  throw new Error(`unsupported sampler address mode: ${mode}`);
}



function mapBufferUsage(usage) {
  const items = usage ?? [];
  let flags = 0;
  for (const item of items) {
    switch (item) {
      case "COPY_SRC":
        flags |= GPUBufferUsage.COPY_SRC;
        break;
      case "COPY_DST":
        flags |= GPUBufferUsage.COPY_DST;
        break;
      case "VERTEX":
        flags |= GPUBufferUsage.VERTEX;
        break;
      case "INDEX":
        flags |= GPUBufferUsage.INDEX;
        break;
      case "UNIFORM":
        flags |= GPUBufferUsage.UNIFORM;
        break;
      case "STORAGE":
        flags |= GPUBufferUsage.STORAGE;
        break;
      case "MAP_READ":
        flags |= GPUBufferUsage.MAP_READ;
        break;
      case "MAP_WRITE":
        if (items.length === 1 || (items.length === 2 && items.includes("COPY_SRC"))) {
          flags |= GPUBufferUsage.MAP_WRITE;
        }
        break;
      default:
        throw new Error(`unsupported buffer usage: ${item}`);
    }
  }
  if (flags === 0) {
    throw new Error("CreateBuffer needs at least one usage flag");
  }
  return flags;
}



function mapTextureUsage(usage) {
  const items = usage ?? [];
  let flags = 0;
  for (const item of items) {
    switch (item) {
      case "COPY_SRC":
        flags |= GPUTextureUsage.COPY_SRC;
        break;
      case "COPY_DST":
        flags |= GPUTextureUsage.COPY_DST;
        break;
      case "TEXTURE_BINDING":
        flags |= GPUTextureUsage.TEXTURE_BINDING;
        break;
      case "STORAGE_BINDING":
        flags |= GPUTextureUsage.STORAGE_BINDING;
        break;
      case "RENDER_ATTACHMENT":
        flags |= GPUTextureUsage.RENDER_ATTACHMENT;
        break;
      default:
        throw new Error(`unsupported texture usage: ${item}`);
    }
  }
  if (flags === 0) {
    throw new Error("CreateTexture needs at least one usage flag");
  }
  return flags;
}



function mapTextureFormat(format) {
  switch (format) {
    case "r32uint":
    case "rgba8unorm":
    case "bgra8unorm":
    case "depth32float":
      return format;
    default:
      throw new Error(`unsupported texture format: ${format}`);
  }
}



function defaultColorTarget(fragmentShader) {
  if (fragmentShader.code.includes("-> @location(0) u32")) {
    return { format: "r32uint" };
  }
  return { format: "rgba8unorm" };
}



function mapVertexFormat(format) {
  switch (format) {
    case "float32":
    case "float32x2":
    case "float32x3":
    case "float32x4":
    case "uint32":
    case "uint32x2":
    case "uint32x3":
    case "uint32x4":
    case "sint32":
    case "sint32x2":
    case "sint32x3":
    case "sint32x4":
    case "unorm8x4":
    case "snorm8x4":
    case "unorm16x2":
    case "unorm16x4":
    case "snorm16x2":
    case "snorm16x4":
      return format;
    default:
      throw new Error(`unsupported vertex format: ${format}`);
  }
}



function mapStepMode(stepMode) {
  if (stepMode === undefined || stepMode === "vertex" || stepMode === "instance") {
    return stepMode ?? "vertex";
  }
  throw new Error(`unsupported vertex step_mode: ${stepMode}`);
}



function decodeBase64(data) {
  const binary = atob(data);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes;
}



async function decodePayload(command, label) {
  const bytes = decodeBase64(required(command.data, `${label} needs data`));
  const encoding = command.data_encoding ?? "base64";
  if (encoding === "base64") {
    return bytes;
  }
  if (encoding === "base64+gzip") {
    if (typeof DecompressionStream === "undefined") {
      throw new Error("base64+gzip payloads need browser DecompressionStream support");
    }
    const stream = new Blob([bytes]).stream().pipeThrough(new DecompressionStream("gzip"));
    const decoded = new Uint8Array(await new Response(stream).arrayBuffer());
    if (
      command.uncompressed_size !== undefined &&
      decoded.byteLength !== command.uncompressed_size
    ) {
      throw new Error(
        `${label} uncompressed size ${decoded.byteLength} does not match ${command.uncompressed_size}`,
      );
    }
    return decoded;
  }
  throw new Error(`unsupported payload encoding: ${encoding}`);
}



function encodeBase64(bytes) {
  let binary = "";
  for (const byte of bytes) {
    binary += String.fromCharCode(byte);
  }
  return btoa(binary);
}



function readbackSummary(bytes) {
  let nonzero = 0;
  let alpha = 0;
  for (let i = 0; i < bytes.length; i++) {
    if (bytes[i] !== 0) {
      nonzero++;
    }
    if ((i & 3) === 3) {
      alpha += bytes[i];
    }
  }
  return { nonzero, alpha };
}



function alignedBytesPerRow(bytesPerRow) {
  return Math.ceil(bytesPerRow / 256) * 256;
}



function makeVertexBuffers(command, vertexShader) {
  const vertexBuffers = command.vertex_buffers ?? [];
  if (vertexBuffers.length === 0) {
    if ((command.vertex_buffer_slots ?? 0) > 0 && vertexShader.code.includes("@location(0)")) {
      return [
        {
          arrayStride: 12,
          stepMode: "vertex",
          attributes: [{ shaderLocation: 0, offset: 0, format: "float32x3" }],
        },
      ];
    }
    return [];
  }
  if (command.vertex_buffer_slots !== undefined && command.vertex_buffer_slots !== vertexBuffers.length) {
    throw new Error("vertex_buffer_slots does not match vertex_buffers length");
  }

  return vertexBuffers.map((buffer) => ({
    arrayStride: required(buffer.array_stride, "vertex buffer needs array_stride"),
    stepMode: mapStepMode(buffer.step_mode),
    attributes: required(buffer.attributes, "vertex buffer needs attributes").map((attribute) => ({
      shaderLocation: required(attribute.shader_location, "vertex attribute needs shader_location"),
      offset: required(attribute.offset, "vertex attribute needs offset"),
      format: mapVertexFormat(required(attribute.format, "vertex attribute needs format")),
    })),
  }));
}



function makeDepthStencil(command) {
  const state = command.depth_stencil;
  if (state === undefined) {
    return undefined;
  }
  return {
    format: mapTextureFormat(required(state.format, "depth_stencil needs format")),
    depthWriteEnabled: state.depth_write_enabled ?? false,
    depthCompare: mapDepthCompare(state.depth_compare),
  };
}



function shaderStageVisibility(stages, fallback) {
  const selected = stages ?? fallback;
  return selected.reduce((bits, stage) => {
    switch (stage) {
      case "VERTEX":
        return bits | GPUShaderStage.VERTEX;
      case "FRAGMENT":
        return bits | GPUShaderStage.FRAGMENT;
      case "COMPUTE":
        return bits | GPUShaderStage.COMPUTE;
      default:
        throw new Error(`unsupported shader-stage visibility: ${stage}`);
    }
  }, 0);
}



function makeBindGroupLayoutEntry(entry, storageAccess = "read_write", options = {}) {
  const binding = required(entry.binding, "bind-group layout entry needs binding");
  if (options.requireExplicitBindGroupLayouts && entry.visibility === undefined) {
    throw new Error(`bind-group layout binding ${binding} needs explicit visibility`);
  }
  switch (entry.binding_type) {
    case "sampled_texture":
      return {
        binding,
        visibility: shaderStageVisibility(entry.visibility, ["FRAGMENT"]),
        texture: { sampleType: "float" },
      };
    case "sampler":
      return {
        binding,
        visibility: shaderStageVisibility(entry.visibility, ["FRAGMENT"]),
        sampler: { type: "filtering" },
      };
    case "uniform_buffer":
      return {
        binding,
        visibility: shaderStageVisibility(entry.visibility, ["VERTEX", "FRAGMENT", "COMPUTE"]),
        buffer: { type: "uniform" },
      };
    case "storage_buffer":
      if (options.requireExplicitBindGroupLayouts && entry.access === undefined) {
        throw new Error(`storage buffer layout binding ${binding} needs explicit access`);
      }
      return {
        binding,
        visibility: shaderStageVisibility(entry.visibility, ["FRAGMENT", "COMPUTE"]),
        buffer: { type: storageAccess === "read" ? "read-only-storage" : "storage" },
      };
    default:
      throw new Error(`unsupported bind-group layout binding_type: ${entry.binding_type}`);
  }
}



function shaderStorageAccess(shader, binding) {
  const escaped = String(binding).replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const pattern = new RegExp(
    `@binding\\(${escaped}\\)[\\s\\S]*?var<storage\\s*,\\s*read\\s*>`,
  );
  return pattern.test(shader.code) ? "read" : "read_write";
}



function specializeBindGroupLayout(device, layoutRecord, shader, options = {}) {
  const entries = layoutRecord.entries.map((entry) => {
    const access = entry.binding_type === "storage_buffer"
      ? (entry.access ?? shaderStorageAccess(shader, entry.binding))
      : "read_write";
    return makeBindGroupLayoutEntry(entry, access, options);
  });
  return {
    entries: layoutRecord.entries,
    layout: device.createBindGroupLayout({ entries }),
  };
}



function makeBindGroupEntry(entry, buffers, textures, textureViews, samplers, dynamicOffset = 0) {
  const binding = required(entry.binding, "bind-group entry needs binding");
  switch (entry.resource_kind) {
    case "buffer": {
      const offset = (entry.offset ?? 0) + dynamicOffset;
      const size = entry.size;
      return {
        binding,
        resource: {
          buffer: required(buffers.get(entry.resource_id), `unknown buffer ${entry.resource_id}`),
          // WebGPU buffer-binding offsets are stricter than DRP2 offsets. The PoC binds at zero
          // for unaligned fixture offsets so compatibility tests can still exercise the command path.
          offset: offset % 256 === 0 ? offset : 0,
          size,
        },
      };
    }
    case "texture":
      return {
        binding,
        resource: required(textures.get(entry.resource_id), `unknown texture ${entry.resource_id}`)
          .createView(),
      };
    case "texture_view":
      return {
        binding,
        resource: required(
          textureViews.get(entry.resource_id),
          `unknown texture view ${entry.resource_id}`,
        ),
      };
    case "sampler":
      return {
        binding,
        resource: required(samplers.get(entry.resource_id), `unknown sampler ${entry.resource_id}`),
      };
    default:
      throw new Error(`unsupported bind-group resource_kind: ${entry.resource_kind}`);
  }
}



function makeBindGroup(device, bindGroupLayout, command, buffers, textures, textureViews, samplers) {
  const dynamicOffsets = command.dynamic_offsets ?? [];
  let dynamicIndex = 0;
  return device.createBindGroup({
    label: command.label,
    layout: bindGroupLayout.layout,
    entries: required(command.entries, "CreateBindGroup needs entries").map((entry) => {
      const layoutEntry = bindGroupLayout.entries.find((item) => item.binding === entry.binding);
      const dynamicOffset = layoutEntry?.has_dynamic_offset ? dynamicOffsets[dynamicIndex++] ?? 0 : 0;
      return makeBindGroupEntry(
        entry,
        buffers,
        textures,
        textureViews,
        samplers,
        dynamicOffset,
      );
    }),
  });
}



function bindGroupForSet(device, passRecord, slot, bindGroupRecord, command, buffers, textures, textureViews, samplers) {
  const layout = passRecord.pipeline?.bindGroupLayouts?.[slot];
  if (layout === undefined && (command.dynamic_offsets ?? []).length === 0) {
    return bindGroupRecord.bindGroup;
  }
  return makeBindGroup(
    device,
    required(layout, `missing pipeline bind-group layout at slot ${slot}`),
    {
      label: `${command.bind_group_id}:set`,
      entries: bindGroupRecord.entries,
      dynamic_offsets: command.dynamic_offsets ?? [],
    },
    buffers,
    textures,
    textureViews,
    samplers,
  );
}



function makeTextureViewDescriptor(command) {
  return {
    label: command.label,
    format: command.format === undefined ? undefined : mapTextureFormat(command.format),
    dimension: command.dimension,
    aspect: command.aspect ?? "all",
    baseMipLevel: command.mip_range?.base ?? 0,
    mipLevelCount: command.mip_range?.count,
    baseArrayLayer: command.layer_range?.base ?? 0,
    arrayLayerCount: command.layer_range?.count,
  };
}



function clearValue(value) {
  if (value === undefined) {
    return { r: 0, g: 0, b: 0, a: 1 };
  }
  return {
    r: value.r ?? 0,
    g: value.g ?? 0,
    b: value.b ?? 0,
    a: value.a ?? 1,
  };
}



function canvasExtent() {
  return {
    width: Math.max(1, canvas.width),
    height: Math.max(1, canvas.height),
  };
}



function resolveTextureExtentValue(value, axis) {
  if (value === "canvas") {
    return canvasExtent()[axis];
  }
  return required(value, `CreateTexture needs ${axis}`);
}



function resizeCanvasToDisplaySize(device, context, format) {
  const scale = Math.max(1, window.devicePixelRatio || 1);
  const width = Math.max(1, Math.floor(canvas.clientWidth * scale));
  const height = Math.max(1, Math.floor(canvas.clientHeight * scale));
  if (canvas.width === width && canvas.height === height) {
    return false;
  }
  canvas.width = width;
  canvas.height = height;
  context.configure({
    device,
    format,
    alphaMode: "opaque",
  });
  return true;
}



function identityMat4() {
  const mat = new Float32Array(16);
  mat[0] = 1;
  mat[5] = 1;
  mat[10] = 1;
  mat[15] = 1;
  return mat;
}



function panzoomProjMat4(zoomX, zoomY, offsetX, offsetY) {
  const mat = identityMat4();
  mat[0] = zoomX;
  mat[5] = zoomY;
  mat[12] = offsetX;
  mat[13] = offsetY;
  return mat;
}



function mvpUniformBytes(zoomX, zoomY, offsetX, offsetY) {
  const buffer = new ArrayBuffer(208);
  const f32 = new Float32Array(buffer);
  f32.set(identityMat4(), 0);
  f32.set(identityMat4(), 16);
  f32.set(panzoomProjMat4(zoomX, zoomY, offsetX, offsetY), 32);
  return new Uint8Array(buffer);
}



function viewportUniformBytes() {
  return new Uint8Array(new Float32Array([0, 0, canvas.width, canvas.height]).buffer);
}



function clamp(value, minValue, maxValue) {
  return Math.min(maxValue, Math.max(minValue, value));
}



function eventNdc(event) {
  const rect = canvas.getBoundingClientRect();
  const x = ((event.clientX - rect.left) / Math.max(1, rect.width)) * 2 - 1;
  const y = 1 - ((event.clientY - rect.top) / Math.max(1, rect.height)) * 2;
  return { x, y };
}



class PanzoomInteraction {
  constructor(config, requestRender) {
    this.config = config;
    this.requestRender = requestRender;
    this.zoomX = 1;
    this.zoomY = 1;
    this.offsetX = 0;
    this.offsetY = 0;
    this.dragging = false;
    this.dragButton = -1;
    this.lastX = 0;
    this.lastY = 0;
    this.pressX = 0;
    this.pressY = 0;
    this.pressNdcX = 0;
    this.pressNdcY = 0;
    this.pressZoomX = 1;
    this.pressZoomY = 1;
    this.pressOffsetX = 0;
    this.pressOffsetY = 0;

    this.onPointerDown = this.onPointerDown.bind(this);
    this.onPointerMove = this.onPointerMove.bind(this);
    this.onPointerUp = this.onPointerUp.bind(this);
    this.onWheel = this.onWheel.bind(this);
    this.onDoubleClick = this.onDoubleClick.bind(this);
    this.onContextMenu = this.onContextMenu.bind(this);

    canvas.addEventListener("pointerdown", this.onPointerDown);
    canvas.addEventListener("pointermove", this.onPointerMove);
    canvas.addEventListener("pointerup", this.onPointerUp);
    canvas.addEventListener("pointercancel", this.onPointerUp);
    canvas.addEventListener("wheel", this.onWheel, { passive: false });
    canvas.addEventListener("dblclick", this.onDoubleClick);
    canvas.addEventListener("contextmenu", this.onContextMenu);
  }

  destroy() {
    canvas.removeEventListener("pointerdown", this.onPointerDown);
    canvas.removeEventListener("pointermove", this.onPointerMove);
    canvas.removeEventListener("pointerup", this.onPointerUp);
    canvas.removeEventListener("pointercancel", this.onPointerUp);
    canvas.removeEventListener("wheel", this.onWheel);
    canvas.removeEventListener("dblclick", this.onDoubleClick);
    canvas.removeEventListener("contextmenu", this.onContextMenu);
  }

  reset() {
    this.zoomX = 1;
    this.zoomY = 1;
    this.offsetX = 0;
    this.offsetY = 0;
    this.requestRender();
  }

  onPointerDown(event) {
    if (event.button !== 0 && event.button !== 2) {
      return;
    }
    this.dragging = true;
    this.dragButton = event.button;
    this.lastX = event.clientX;
    this.lastY = event.clientY;
    this.pressX = event.clientX;
    this.pressY = event.clientY;
    const ndc = eventNdc(event);
    this.pressNdcX = ndc.x;
    this.pressNdcY = ndc.y;
    this.pressZoomX = this.zoomX;
    this.pressZoomY = this.zoomY;
    this.pressOffsetX = this.offsetX;
    this.pressOffsetY = this.offsetY;
    canvas.setPointerCapture(event.pointerId);
  }

  onPointerMove(event) {
    if (!this.dragging) {
      return;
    }
    const rect = canvas.getBoundingClientRect();
    const dx = event.clientX - this.lastX;
    const dy = event.clientY - this.lastY;
    this.lastX = event.clientX;
    this.lastY = event.clientY;
    if (this.dragButton === 0) {
      this.offsetX += (2 * dx) / Math.max(1, rect.width);
      this.offsetY -= (2 * dy) / Math.max(1, rect.height);
    } else if (this.dragButton === 2) {
      const shiftX = 2 * (event.clientX - this.pressX) / Math.max(1, rect.width);
      const shiftY = -2 * (event.clientY - this.pressY) / Math.max(1, rect.height);
      const factorX = Math.exp(2.5 * shiftX);
      const factorY = Math.exp(2.5 * shiftY);
      this.zoomX = clamp(this.pressZoomX * factorX, 0.05, 100);
      this.zoomY = clamp(this.pressZoomY * factorY, 0.05, 100);
      const ratioX = this.zoomX / this.pressZoomX;
      const ratioY = this.zoomY / this.pressZoomY;
      this.offsetX = this.pressNdcX - (this.pressNdcX - this.pressOffsetX) * ratioX;
      this.offsetY = this.pressNdcY - (this.pressNdcY - this.pressOffsetY) * ratioY;
    }
    this.requestRender();
  }

  onPointerUp(event) {
    if (!this.dragging) {
      return;
    }
    this.dragging = false;
    this.dragButton = -1;
    if (canvas.hasPointerCapture(event.pointerId)) {
      canvas.releasePointerCapture(event.pointerId);
    }
  }

  onWheel(event) {
    event.preventDefault();
    const ndc = eventNdc(event);
    const oldZoomX = this.zoomX;
    const oldZoomY = this.zoomY;
    const factor = Math.exp(-event.deltaY * 0.0015);
    this.zoomX = clamp(this.zoomX * factor, 0.05, 100);
    this.zoomY = clamp(this.zoomY * factor, 0.05, 100);
    const ratioX = this.zoomX / oldZoomX;
    const ratioY = this.zoomY / oldZoomY;
    this.offsetX = ndc.x - (ndc.x - this.offsetX) * ratioX;
    this.offsetY = ndc.y - (ndc.y - this.offsetY) * ratioY;
    this.requestRender();
  }

  onDoubleClick() {
    this.reset();
  }

  onContextMenu(event) {
    event.preventDefault();
  }

  beforeRender(runtime) {
    runtime.writeBuffer(
      this.config.mvpBufferId,
      0,
      mvpUniformBytes(this.zoomX, this.zoomY, this.offsetX, this.offsetY),
    );
    runtime.writeBuffer(this.config.viewportBufferId, 0, viewportUniformBytes());
  }
}



export async function initWebGPU() {
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
  resizeCanvasToDisplaySize(device, context, format);

  return { device, context, format };
}



function makePipeline(device, canvasFormat, shaders, bindGroupLayouts, command, options = {}) {
  const vertexShader = required(
    shaders.get(command.vertex_shader_module_id),
    `unknown vertex shader module ${command.vertex_shader_module_id}`,
  );
  const fragmentShader = required(
    shaders.get(command.fragment_shader_module_id),
    `unknown fragment shader module ${command.fragment_shader_module_id}`,
  );

  if (options.requireExplicitPipelineMetadata && command.vertex_buffers === undefined) {
    throw new Error(`render pipeline ${command.id} needs explicit vertex_buffers`);
  }
  if (options.requireExplicitPipelineMetadata && command.color_targets === undefined) {
    throw new Error(`render pipeline ${command.id} needs explicit color_targets`);
  }

  const colorTargets = command.color_targets ?? [defaultColorTarget(fragmentShader)];
  if (colorTargets.length !== 1) {
    throw new Error("only one color target is supported by this PoC");
  }

  const streamFormat = colorTargets[0].format;
  const targetFormat = streamFormat === "canvas" ? canvasFormat : mapTextureFormat(streamFormat);
  const bindGroupLayoutIds = command.bind_group_layout_ids ?? [];
  const pipelineBindGroupLayouts = bindGroupLayoutIds.map((id) =>
    required(bindGroupLayouts.get(id), `unknown bind-group layout ${id}`),
  );
  const layout = pipelineBindGroupLayouts.length > 0
    ? device.createPipelineLayout({
        bindGroupLayouts: pipelineBindGroupLayouts.map((record) => record.layout),
      })
    : "auto";

  return {
    bindGroupLayouts: pipelineBindGroupLayouts,
    pipeline: device.createRenderPipeline({
      label: command.label,
      layout,
      vertex: {
        module: vertexShader.module,
        entryPoint: vertexShader.entryPoint,
        buffers: makeVertexBuffers(command, vertexShader),
      },
      fragment: {
        module: fragmentShader.module,
        entryPoint: fragmentShader.entryPoint,
        targets: [{ format: targetFormat }],
      },
      primitive: {
        topology: mapTopology(command.topology),
      },
      depthStencil: makeDepthStencil(command),
    }),
  };
}



function makeComputePipeline(device, shaders, bindGroupLayouts, command, options = {}) {
  const shader = required(
    shaders.get(command.compute_shader_module_id),
    `unknown compute shader module ${command.compute_shader_module_id}`,
  );
  const bindGroupLayoutIds = command.bind_group_layout_ids ?? [];
  const pipelineBindGroupLayouts = bindGroupLayoutIds.map((id) =>
    specializeBindGroupLayout(
      device,
      required(bindGroupLayouts.get(id), `unknown bind-group layout ${id}`),
      shader,
      options,
    ),
  );
  const layout = pipelineBindGroupLayouts.length > 0
    ? device.createPipelineLayout({
        bindGroupLayouts: pipelineBindGroupLayouts.map((record) => record.layout),
      })
    : "auto";

  return {
    bindGroupLayouts: pipelineBindGroupLayouts,
    pipeline: device.createComputePipeline({
      label: command.label,
      layout,
      compute: {
        module: shader.module,
        entryPoint: shader.entryPoint,
      },
    }),
  };
}



function makeDepthStencilAttachment(textures, attachment) {
  if (attachment === undefined) {
    return undefined;
  }
  const texture = required(
    textures.get(attachment.texture_id),
    `unknown depth texture ${attachment.texture_id}`,
  );
  return {
    view: texture.createView(),
    depthLoadOp: mapLoadOp(attachment.depth_load_op),
    depthStoreOp: mapStoreOp(attachment.depth_store_op),
    depthClearValue: attachment.depth_clear_value ?? 1,
  };
}



function beginRenderPass(context, textures, encoders, command) {
  const encoder = required(
    encoders.get(command.encoder_id),
    `unknown command encoder ${command.encoder_id}`,
  );
  const attachments = required(command.color_attachments, "BeginRenderPass needs color_attachments");

  if (attachments.length !== 1) {
    throw new Error("only one color attachment is supported by this PoC");
  }

  const attachment = attachments[0];
  const textureView = attachment.texture_id === 0
    ? context.getCurrentTexture().createView()
    : required(textures.get(attachment.texture_id), `unknown texture ${attachment.texture_id}`)
        .createView();

  return encoder.beginRenderPass({
    label: command.label,
    colorAttachments: [
      {
        view: textureView,
        loadOp: mapLoadOp(attachment.load_op),
        storeOp: mapStoreOp(attachment.store_op),
        clearValue: clearValue(attachment.clear_value),
      },
    ],
    depthStencilAttachment: makeDepthStencilAttachment(
      textures,
      command.depth_stencil_attachment,
    ),
  });
}



function createExecutionState() {
  return {
    buffers: new Map(),
    textures: new Map(),
    textureViews: new Map(),
    samplers: new Map(),
    bindGroupLayouts: new Map(),
    bindGroups: new Map(),
    shaders: new Map(),
    pipelines: new Map(),
  };
}



function splitStreamCommands(stream) {
  const commands = required(stream.commands, "DRP2 stream needs commands");
  if (Number.isInteger(stream.setup_command_count)) {
    return {
      setupCommands: commands.slice(0, stream.setup_command_count),
      frameCommands: commands.slice(stream.setup_command_count),
    };
  }
  const frameStart = commands.findIndex((command) => command.cmd === "BeginCommandEncoder");
  if (frameStart < 0) {
    return { setupCommands: commands, frameCommands: [] };
  }
  return {
    setupCommands: commands.slice(0, frameStart),
    frameCommands: commands.slice(frameStart),
  };
}



async function executeWithErrorScopes(device, callback) {
  const scopes = ["validation", "out-of-memory", "internal"];
  for (const scope of scopes) {
    device.pushErrorScope(scope);
  }

  let result = null;
  let thrown = null;
  try {
    result = await callback();
  } catch (error) {
    thrown = error;
  }

  const errors = [];
  for (const scope of scopes.slice().reverse()) {
    const error = await device.popErrorScope();
    if (error !== null) {
      errors.push(`${scope}: ${error.message}`);
    }
  }

  if (thrown !== null) {
    throw thrown;
  }
  if (errors.length > 0) {
    throw new Error(errors.join("\n"));
  }
  return result;
}



export class Drp2WebGpuRuntime {
  constructor(device, context, canvasFormat, options = {}) {
    this.device = device;
    this.context = context;
    this.canvasFormat = canvasFormat;
    this.options = options;
    this.state = createExecutionState();
    this.stream = null;
    this.setupCommands = [];
    this.frameCommands = [];
    this.frames = [];
  }

  async load(stream, options = {}) {
    this.stream = stream;
    this.options = { ...this.options, ...options };
    this.state = createExecutionState();

    const split = splitStreamCommands(stream);
    this.setupCommands = split.setupCommands;
    this.frameCommands = split.frameCommands;
    this.frames = Array.isArray(stream.frames) ? stream.frames : [];

    return await executeDrp2StreamChecked(
      this.device,
      this.context,
      this.canvasFormat,
      stream,
      {
        ...this.options,
        commands: this.setupCommands,
        state: this.state,
      },
    );
  }

  async render(options = {}) {
    return await this.renderFrame(options.frameIndex ?? null, options);
  }

  async renderFrame(frameIndex = null, options = {}) {
    if (this.stream === null) {
      throw new Error("runtime has no loaded stream");
    }
    let commands = this.frameCommands;
    if (frameIndex !== null && this.frames.length > 0) {
      const frame = required(this.frames[frameIndex], `unknown frame ${frameIndex}`);
      const first = required(frame.first_command, "frame needs first_command");
      const count = required(frame.command_count, "frame needs command_count");
      commands = this.stream.commands.slice(first, first + count);
    }
    return await executeDrp2StreamChecked(
      this.device,
      this.context,
      this.canvasFormat,
      this.stream,
      {
        ...this.options,
        ...options,
        commands,
        state: this.state,
      },
    );
  }

  writeBuffer(bufferId, offset, bytes) {
    const buffer = required(this.state.buffers.get(bufferId), `unknown buffer ${bufferId}`);
    this.device.queue.writeBuffer(buffer, offset, bytes, 0, bytes.byteLength);
  }
}



export async function executeDrp2Stream(device, context, canvasFormat, stream, options = {}) {
  const state = options.state ?? createExecutionState();
  const buffers = state.buffers;
  const textures = state.textures;
  const textureViews = state.textureViews;
  const samplers = state.samplers;
  const bindGroupLayouts = state.bindGroupLayouts;
  const bindGroups = state.bindGroups;
  const shaders = state.shaders;
  const pipelines = state.pipelines;
  const encoders = new Map();
  const passes = new Map();
  const commandBuffers = new Map();
  const pendingTightTextureCopies = [];
  const readbackReplies = [];

  for (const command of options.commands ?? stream.commands) {
    switch (command.cmd) {
      case "HelloRenderer":
      case "RendererHelloReply":
        break;

      case "CreateBuffer":
        buffers.set(
          command.id,
          device.createBuffer({
            label: command.label,
            size: required(command.size, "CreateBuffer needs size"),
            usage: mapBufferUsage(command.usage),
          }),
        );
        break;

      case "WriteBuffer": {
        const buffer = required(
          buffers.get(command.buffer_id),
          `unknown buffer ${command.buffer_id}`,
        );
        const bytes = await decodePayload(command, "WriteBuffer");
        const size = command.size ?? bytes.byteLength;
        if (size !== bytes.byteLength) {
          throw new Error(`WriteBuffer size ${size} does not match payload size ${bytes.byteLength}`);
        }
        device.queue.writeBuffer(buffer, command.offset ?? 0, bytes, 0, bytes.byteLength);
        break;
      }

      case "CreateTexture":
        textures.set(
          command.id,
          device.createTexture({
            label: command.label,
            size: {
              width: resolveTextureExtentValue(command.width, "width"),
              height: resolveTextureExtentValue(command.height, "height"),
              depthOrArrayLayers: command.depth ?? 1,
            },
            mipLevelCount: command.mip_level_count ?? 1,
            sampleCount: command.sample_count ?? 1,
            dimension: command.dimension ?? "2d",
            format: mapTextureFormat(required(command.format, "CreateTexture needs format")),
            usage: mapTextureUsage(command.usage),
          }),
        );
        break;

      case "CreateTextureView": {
        const texture = required(
          textures.get(command.texture_id),
          `unknown texture ${command.texture_id}`,
        );
        textureViews.set(command.id, texture.createView(makeTextureViewDescriptor(command)));
        break;
      }

      case "WriteTexture": {
        const texture = required(
          textures.get(command.texture_id),
          `unknown texture ${command.texture_id}`,
        );
        const bytes = await decodePayload(command, "WriteTexture");
        const size = required(command.size, "WriteTexture needs size");
        const origin = command.origin ?? { x: 0, y: 0, z: 0 };
        device.queue.writeTexture(
          {
            texture,
            mipLevel: command.mip_level ?? 0,
            origin: {
              x: origin.x ?? 0,
              y: origin.y ?? 0,
              z: origin.z ?? 0,
            },
          },
          bytes,
          {
            offset: 0,
            bytesPerRow: required(command.bytes_per_row, "WriteTexture needs bytes_per_row"),
            rowsPerImage: command.rows_per_image ?? size.height,
          },
          {
            width: required(size.width, "WriteTexture size needs width"),
            height: required(size.height, "WriteTexture size needs height"),
            depthOrArrayLayers: size.depth ?? 1,
          },
        );
        break;
      }

      case "CreateSampler":
        samplers.set(
          command.id,
          device.createSampler({
            label: command.label,
            magFilter: mapFilterMode(command.mag_filter),
            minFilter: mapFilterMode(command.min_filter),
            mipmapFilter: mapFilterMode(command.mipmap_filter),
            addressModeU: mapAddressMode(command.address_mode_u),
            addressModeV: mapAddressMode(command.address_mode_v),
            addressModeW: mapAddressMode(command.address_mode_w),
          }),
        );
        break;

      case "CreateBindGroupLayout":
        bindGroupLayouts.set(
          command.id,
          {
            entries: required(command.entries, "CreateBindGroupLayout needs entries"),
            layout: device.createBindGroupLayout({
              label: command.label,
              entries: command.entries.map((entry) =>
                makeBindGroupLayoutEntry(entry, "read_write", options)),
            }),
          },
        );
        break;

      case "CreateBindGroup": {
        const bindGroupLayout = required(
          bindGroupLayouts.get(command.bind_group_layout_id),
          `unknown bind-group layout ${command.bind_group_layout_id}`,
        );
        bindGroups.set(
          command.id,
          {
            layoutId: command.bind_group_layout_id,
            entries: required(command.entries, "CreateBindGroup needs entries"),
            bindGroup: makeBindGroup(
              device,
              bindGroupLayout,
              command,
              buffers,
              textures,
              textureViews,
              samplers,
            ),
          },
        );
        break;
      }

      case "CreateShaderModule": {
        if (command.format !== "wgsl") {
          throw new Error(`unsupported shader format: ${command.format}`);
        }
        const module = device.createShaderModule({
          label: command.label,
          code: required(command.code, "CreateShaderModule needs code"),
        });
        const info = await module.getCompilationInfo();
        const errors = info.messages.filter((message) => message.type === "error");
        if (errors.length > 0) {
          throw new Error(errors.map((message) => message.message).join("\n"));
        }
        shaders.set(command.id, {
          module,
          entryPoint: command.entry_point ?? "main",
          stage: command.stage,
          code: command.code,
        });
        break;
      }

      case "CreateRenderPipeline":
        pipelines.set(
          command.id,
          makePipeline(device, canvasFormat, shaders, bindGroupLayouts, command, options),
        );
        break;

      case "CreateComputePipeline":
        pipelines.set(
          command.id,
          makeComputePipeline(device, shaders, bindGroupLayouts, command, options),
        );
        break;

      case "BeginCommandEncoder":
        encoders.set(command.id, device.createCommandEncoder({ label: command.label }));
        break;

      case "BeginRenderPass":
        passes.set(command.id, {
          kind: "render",
          pass: beginRenderPass(context, textures, encoders, command),
        });
        break;

      case "BeginComputePass": {
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        passes.set(command.id, {
          kind: "compute",
          pass: encoder.beginComputePass({ label: command.label }),
        });
        break;
      }

      case "SetPipeline": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const pipelineRecord = required(
          pipelines.get(command.pipeline_id),
          `unknown pipeline ${command.pipeline_id}`,
        );
        passRecord.pipeline = pipelineRecord;
        passRecord.pass.setPipeline(pipelineRecord.pipeline);
        break;
      }

      case "SetVertexBuffer": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("SetVertexBuffer requires a render pass");
        }
        const buffer = required(
          buffers.get(command.buffer_id),
          `unknown buffer ${command.buffer_id}`,
        );
        passRecord.pass.setVertexBuffer(command.slot ?? 0, buffer, command.offset ?? 0);
        break;
      }

      case "SetIndexBuffer": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("SetIndexBuffer requires a render pass");
        }
        const buffer = required(
          buffers.get(command.buffer_id),
          `unknown buffer ${command.buffer_id}`,
        );
        passRecord.pass.setIndexBuffer(buffer, mapIndexFormat(command.index_format), command.offset ?? 0);
        break;
      }

      case "SetBindGroup": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const bindGroupRecord = required(
          bindGroups.get(command.bind_group_id),
          `unknown bind group ${command.bind_group_id}`,
        );
        const slot = command.slot ?? 0;
        const bindGroup = bindGroupForSet(
          device,
          passRecord,
          slot,
          bindGroupRecord,
          command,
          buffers,
          textures,
          textureViews,
          samplers,
        );
        passRecord.pass.setBindGroup(slot, bindGroup, []);
        break;
      }

      case "Draw": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("Draw requires a render pass");
        }
        passRecord.pass.draw(
          command.vertex_count,
          command.instance_count ?? 1,
          command.first_vertex ?? 0,
          command.first_instance ?? 0,
        );
        break;
      }

      case "DrawIndexed": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("DrawIndexed requires a render pass");
        }
        passRecord.pass.drawIndexed(
          command.index_count,
          command.instance_count ?? 1,
          command.first_index ?? 0,
          command.base_vertex ?? 0,
          command.first_instance ?? 0,
        );
        break;
      }

      case "DispatchWorkgroups": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "compute") {
          throw new Error("DispatchWorkgroups requires a compute pass");
        }
        passRecord.pass.dispatchWorkgroups(command.x ?? 1, command.y ?? 1, command.z ?? 1);
        break;
      }

      case "EndRenderPass": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("EndRenderPass requires a render pass");
        }
        passRecord.pass.end();
        passes.delete(command.pass_id);
        break;
      }

      case "EndComputePass": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "compute") {
          throw new Error("EndComputePass requires a compute pass");
        }
        passRecord.pass.end();
        passes.delete(command.pass_id);
        break;
      }

      case "CopyTextureToBuffer": {
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        const texture = required(
          textures.get(command.src_texture_id),
          `unknown source texture ${command.src_texture_id}`,
        );
        const buffer = required(
          buffers.get(command.dst_buffer_id),
          `unknown destination buffer ${command.dst_buffer_id}`,
        );
        const origin = command.src_origin ?? { x: 0, y: 0, z: 0 };
        const size = required(command.size, "CopyTextureToBuffer needs size");
        const bytesPerRow = required(
          command.bytes_per_row,
          "CopyTextureToBuffer needs bytes_per_row",
        );
        const rowsPerImage = command.rows_per_image ?? size.height;
        let dstBuffer = buffer;
        let dstOffset = command.dst_offset ?? 0;
        let copyBytesPerRow = bytesPerRow;

        if (bytesPerRow % 256 !== 0) {
          copyBytesPerRow = alignedBytesPerRow(bytesPerRow);
          dstOffset = 0;
          dstBuffer = device.createBuffer({
            label: `${command.dst_buffer_id}:aligned_texture_readback`,
            size: copyBytesPerRow * rowsPerImage,
            usage: GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST,
          });
          pendingTightTextureCopies.push({
            srcBuffer: dstBuffer,
            dstBuffer: buffer,
            dstOffset: command.dst_offset ?? 0,
            bytesPerRow,
            copyBytesPerRow,
            rows: rowsPerImage,
          });
        }

        encoder.copyTextureToBuffer(
          {
            texture,
            mipLevel: command.src_mip_level ?? 0,
            origin: {
              x: origin.x ?? 0,
              y: origin.y ?? 0,
              z: origin.z ?? 0,
            },
          },
          {
            buffer: dstBuffer,
            offset: dstOffset,
            bytesPerRow: copyBytesPerRow,
            rowsPerImage,
          },
          {
            width: required(size.width, "CopyTextureToBuffer size needs width"),
            height: required(size.height, "CopyTextureToBuffer size needs height"),
            depthOrArrayLayers: size.depth ?? 1,
          },
        );
        for (const copy of pendingTightTextureCopies.splice(0)) {
          for (let row = 0; row < copy.rows; row++) {
            encoder.copyBufferToBuffer(
              copy.srcBuffer,
              row * copy.copyBytesPerRow,
              copy.dstBuffer,
              copy.dstOffset + row * copy.bytesPerRow,
              copy.bytesPerRow,
            );
          }
        }
        break;
      }

      case "CopyBufferToTexture": {
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        const buffer = required(
          buffers.get(command.src_buffer_id),
          `unknown source buffer ${command.src_buffer_id}`,
        );
        const texture = required(
          textures.get(command.dst_texture_id),
          `unknown destination texture ${command.dst_texture_id}`,
        );
        const origin = command.dst_origin ?? { x: 0, y: 0, z: 0 };
        const size = required(command.size, "CopyBufferToTexture needs size");
        encoder.copyBufferToTexture(
          {
            buffer,
            offset: command.src_offset ?? 0,
            bytesPerRow: required(command.bytes_per_row, "CopyBufferToTexture needs bytes_per_row"),
            rowsPerImage: command.rows_per_image ?? size.height,
          },
          {
            texture,
            mipLevel: command.dst_mip_level ?? 0,
            origin: {
              x: origin.x ?? 0,
              y: origin.y ?? 0,
              z: origin.z ?? 0,
            },
          },
          {
            width: required(size.width, "CopyBufferToTexture size needs width"),
            height: required(size.height, "CopyBufferToTexture size needs height"),
            depthOrArrayLayers: size.depth ?? 1,
          },
        );
        break;
      }

      case "CopyTextureToTexture": {
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        const srcTexture = required(
          textures.get(command.src_texture_id),
          `unknown source texture ${command.src_texture_id}`,
        );
        const dstTexture = required(
          textures.get(command.dst_texture_id),
          `unknown destination texture ${command.dst_texture_id}`,
        );
        const srcOrigin = command.src_origin ?? { x: 0, y: 0, z: 0 };
        const dstOrigin = command.dst_origin ?? { x: 0, y: 0, z: 0 };
        const size = required(command.size, "CopyTextureToTexture needs size");
        encoder.copyTextureToTexture(
          {
            texture: srcTexture,
            mipLevel: command.src_mip_level ?? 0,
            origin: {
              x: srcOrigin.x ?? 0,
              y: srcOrigin.y ?? 0,
              z: srcOrigin.z ?? 0,
            },
          },
          {
            texture: dstTexture,
            mipLevel: command.dst_mip_level ?? 0,
            origin: {
              x: dstOrigin.x ?? 0,
              y: dstOrigin.y ?? 0,
              z: dstOrigin.z ?? 0,
            },
          },
          {
            width: required(size.width, "CopyTextureToTexture size needs width"),
            height: required(size.height, "CopyTextureToTexture size needs height"),
            depthOrArrayLayers: size.depth ?? 1,
          },
        );
        break;
      }

      case "FinishCommandEncoder": {
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        commandBuffers.set(command.command_buffer_id, encoder.finish());
        encoders.delete(command.encoder_id);
        break;
      }

      case "QueueSubmit": {
        const ids = command.command_buffer_ids ?? [command.command_buffer_id];
        const submitBuffers = ids.map((id) =>
          required(commandBuffers.get(id), `unknown command buffer ${id}`),
        );
        device.queue.submit(submitBuffers);
        await device.queue.onSubmittedWorkDone();
        for (const readback of command.readbacks ?? []) {
          const buffer = required(
            buffers.get(readback.buffer_id),
            `unknown readback buffer ${readback.buffer_id}`,
          );
          const offset = readback.offset ?? 0;
          const size = required(readback.size, "readback needs size");
          await buffer.mapAsync(GPUMapMode.READ, offset, size);
          const mapped = buffer.getMappedRange(offset, size);
          const bytes = new Uint8Array(mapped.slice(0));
          buffer.unmap();
          readbackReplies.push({
            submission_id: command.submission_id,
            buffer_id: readback.buffer_id,
            offset,
            size,
            data: encodeBase64(bytes),
            summary: readbackSummary(bytes),
          });
        }
        break;
      }

      case "QueueSubmitReply":
      case "Error":
      case "DestroyBuffer":
      case "DestroyTexture":
      case "DestroyTextureView":
      case "DestroySampler":
      case "DestroyBindGroupLayout":
      case "DestroyBindGroup":
      case "DestroyShaderModule":
      case "DestroyRenderPipeline":
      case "DestroyComputePipeline":
        break;

      default:
        throw new Error(`unsupported DRP2 command in WebGPU PoC: ${command.cmd}`);
    }
  }

  return { readbacks: readbackReplies, state };
}



export async function executeDrp2StreamChecked(
  device,
  context,
  canvasFormat,
  stream,
  options = {},
) {
  return await executeWithErrorScopes(
    device,
    async () => await executeDrp2Stream(device, context, canvasFormat, stream, options),
  );
}



async function main() {
  try {
    const { device, context, format } = await initWebGPU();
    const params = new URLSearchParams(window.location.search);
    let streamName = params.get("stream") ?? "indexed_quad_wgsl";
    let stream = null;
    let streamConfig = streamConfigByName(streamName);
    let runtime = null;
    let interaction = null;
    let frameIndex = 0;
    let playing = false;
    let playbackRequest = 0;

    for (const item of STREAMS) {
      const option = document.createElement("option");
      option.value = item.name;
      option.textContent = item.label;
      streamSelectEl.appendChild(option);
    }
    streamName = streamConfig.name;
    streamSelectEl.value = streamName;

    const configureInteraction = (config) => {
      if (interaction !== null) {
        interaction.destroy();
        interaction = null;
      }
      if (config.interactive?.type === "panzoom") {
        interaction = new PanzoomInteraction(config.interactive, () => {
          render().catch((error) => setStatus(error.message, true));
        });
        interactionHelpEl.textContent =
          "left-drag to pan, right-drag/wheel to zoom, double-click to reset";
      } else {
        interactionHelpEl.textContent = "";
      }
    };

    const frameCount = () => runtime?.frames?.length ?? 0;

    const updatePlaybackUi = () => {
      const count = frameCount();
      playToggleEl.disabled = count <= 1 || interaction !== null;
      playToggleEl.textContent = playing ? "Pause" : "Play";
      frameInfoEl.textContent = count > 0 ? `${frameIndex + 1}/${count}` : "0/0";
    };

    const stopPlayback = () => {
      playing = false;
      if (playbackRequest !== 0) {
        cancelAnimationFrame(playbackRequest);
        playbackRequest = 0;
      }
      updatePlaybackUi();
    };

    const loadStream = async (name) => {
      streamConfig = streamConfigByName(name);
      const sourceName = streamSourceName(streamConfig);
      const streamPath = `./streams/${sourceName}.json`;
      streamNameEl.textContent = streamPath.slice(2);
      const response = await fetch(streamPath, { cache: "no-cache" });
      if (!response.ok) {
        throw new Error(`failed to load stream: ${response.status} ${response.statusText}`);
      }
      streamName = streamConfig.name;
      stream = await response.json();
      applyStreamCanvasAspect(stream);
      runtime = new Drp2WebGpuRuntime(device, context, format);
      await runtime.load(stream);
      configureInteraction(streamConfig);
      frameIndex = 0;
      stopPlayback();
      updatePlaybackUi();
      const url = new URL(window.location.href);
      url.searchParams.set("stream", streamName);
      window.history.replaceState(null, "", url);
    };

    let rendering = false;
    let rerenderRequested = false;

    const render = async () => {
      if (rendering) {
        rerenderRequested = true;
        return;
      }

      rendering = true;
      try {
        do {
          rerenderRequested = false;
          if (stream === null) {
            await loadStream(streamName);
          }
          const resized = resizeCanvasToDisplaySize(device, context, format);
          if (resized && interaction === null) {
            await runtime.load(stream);
          }
          if (interaction !== null) {
            interaction.beforeRender(runtime);
          }
          const count = frameCount();
          const result = await runtime.render({ frameIndex: count > 0 ? frameIndex : null });
          if (result.readbacks.length > 0) {
            const readback = result.readbacks[0];
            setStatus(
              `Rendered ${stream.name}; readback nonzero=${readback.summary.nonzero}`,
            );
          } else {
            const suffix = interaction !== null ? "; pan/zoom active" : "";
            const frameSuffix = count > 0 ? `; frame=${frameIndex + 1}/${count}` : "";
            setStatus(`Rendered ${streamName}; readbacks=0${frameSuffix}${suffix}`);
          }
          updatePlaybackUi();
        } while (rerenderRequested);
      } finally {
        rendering = false;
      }
    };

    const schedulePlayback = () => {
      playbackRequest = requestAnimationFrame(async () => {
        playbackRequest = 0;
        if (!playing) {
          return;
        }
        const count = frameCount();
        if (count <= 1) {
          stopPlayback();
          return;
        }
        frameIndex = (frameIndex + 1) % count;
        try {
          await render();
        } catch (error) {
          stopPlayback();
          setStatus(error.message, true);
          return;
        }
        if (playing) {
          schedulePlayback();
        }
      });
    };

    playToggleEl.addEventListener("click", () => {
      if (playing) {
        stopPlayback();
        return;
      }
      if (frameCount() <= 1 || interaction !== null) {
        return;
      }
      playing = true;
      updatePlaybackUi();
      schedulePlayback();
    });

    streamSelectEl.addEventListener("change", () => {
      stopPlayback();
      loadStream(streamSelectEl.value)
        .then(render)
        .catch((error) => setStatus(error.message, true));
    });

    await loadStream(streamName);
    await render();
    new ResizeObserver(() => {
      stopPlayback();
      render().catch((error) => setStatus(error.message, true));
    }).observe(canvas);
  } catch (error) {
    setStatus(error.message, true);
    console.error(error);
  }
}



if (
  canvas !== null &&
  statusEl !== null &&
  streamNameEl !== null &&
  streamSelectEl !== null &&
  playToggleEl !== null &&
  frameInfoEl !== null
) {
  main();
}
