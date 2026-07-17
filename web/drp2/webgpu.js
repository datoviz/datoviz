import { decodeDrp2PacketSet } from "./packet.js";
import {
  resizeCanvasToDisplaySize,
  runtimeCapabilities,
} from "./context.js";
export {
  initWebGPU,
  resizeWebGpuCanvas,
  runtimeCapabilities,
} from "./context.js";

const BROWSER_CANVAS_TEXTURE_ID = 0;
const BROWSER_CANVAS_FORMAT_ALIAS = "canvas";
const BROWSER_CANVAS_EXTENT_ALIAS = "canvas";

export const STREAMS = [
  { name: "scene_primitive_wgsl", label: "Scene primitive (WGSL)" },
  { name: "scene_point_wgsl", label: "Scene points (WGSL)" },
  { name: "attachment_multi_color_wgsl", label: "Attachment validation (multi-color)" },
  { name: "attachment_depth_wgsl", label: "Attachment validation (depth)" },
  { name: "mesh_dvzr_wgsl", label: "DVZR mesh replay (WGSL)" },
  { name: "indexed_quad_wgsl", label: "Indexed quad" },
  { name: "texture_sampling_wgsl", label: "Texture sampling" },
  { name: "depth_overlap_wgsl", label: "Depth overlap" },
  { name: "triangle_offscreen_readback_wgsl", label: "Offscreen readback" },
  { name: "triangle_vertex_buffer_wgsl", label: "Vertex-buffer triangle" },
  { name: "triangle_wgsl", label: "No-buffer triangle" },
];



export function streamConfigByName(name) {
  return STREAMS.find((item) => item.name === name) ?? STREAMS[0];
}



export function streamSourceName(config) {
  return config.source ?? config.name;
}



function required(value, message) {
  if (value === undefined || value === null) {
    throw new Error(message);
  }
  return value;
}



function isBrowserCanvasTextureId(textureId) {
  return textureId === BROWSER_CANVAS_TEXTURE_ID;
}



function isBrowserCanvasFormatAlias(format) {
  return format === BROWSER_CANVAS_FORMAT_ALIAS;
}



function isBrowserCanvasExtentAlias(value) {
  return value === BROWSER_CANVAS_EXTENT_ALIAS;
}



export class Drp2WebGpuError extends Error {
  constructor(commandIndex, command, code, detail, cause = null) {
    super(`command ${commandIndex} ${command?.cmd ?? "Unknown"}: ${detail}`);
    this.name = "Drp2WebGpuError";
    this.commandIndex = commandIndex;
    this.cmd = command?.cmd ?? null;
    this.code = code;
    this.detail = detail;
    if (cause !== null) {
      this.cause = cause;
    }
  }
}



function classifyDrp2WebGpuError(message) {
  if (message.includes("requires feature")) {
    return "DRP2_ERR_FEATURE_REQUIRED";
  }
  if (message.includes("unsupported DRP2 version")) {
    return "DRP2_ERR_UNSUPPORTED_VERSION";
  }
  if (message.startsWith("unsupported ") || message.includes("unsupported DRP2 command")) {
    return "DRP2_ERR_UNSUPPORTED_CAPABILITY";
  }
  if (message.includes("not supported by capabilities")) {
    return "DRP2_ERR_UNSUPPORTED_CAPABILITY";
  }
  if (message.includes("out of range") || message.includes("exceeds")) {
    return "DRP2_ERR_OUT_OF_RANGE";
  }
  if (
    message.includes("lacks") ||
    message.includes("usage") ||
    message.includes("referenced by")
  ) {
    return "DRP2_ERR_USAGE";
  }
  if (message.startsWith("unknown pass") || message.startsWith("unknown command encoder")) {
    return "DRP2_ERR_INVALID_STATE";
  }
  if (message.startsWith("unknown ")) {
    return "DRP2_ERR_INVALID_ID";
  }
  if (message.includes("duplicate or reused object id")) {
    return "DRP2_ERR_DUPLICATE_ID";
  }
  if (message.includes(" is ") && message.includes(", not ")) {
    return "DRP2_ERR_WRONG_OBJECT_TYPE";
  }
  if (message.includes("requires a render pass") || message.includes("requires a compute pass")) {
    return "DRP2_ERR_PASS_MISMATCH";
  }
  if (
    message.includes("destroyed") ||
    message.includes("already been submitted") ||
    message.includes("has no bound pipeline") ||
    message.includes("handshake") ||
    message.includes("diagnostic Error before") ||
    message.includes("open pass") ||
    message.includes("before a pipeline") ||
    message.includes("missing vertex buffer") ||
    message.includes("missing index buffer") ||
    message.includes("dynamic offset count") ||
    message.includes("does not match pipeline slot")
  ) {
    return "DRP2_ERR_INVALID_STATE";
  }
  if (
    message.includes("bytes_per_row") ||
    message.includes("rows_per_image") ||
    message.includes("texture payload") ||
    message.includes("texture layout")
  ) {
    return "DRP2_ERR_LAYOUT";
  }
  return "DRP2_ERR_INVALID_ARGUMENT";
}



function wrapDrp2WebGpuError(commandIndex, command, error) {
  if (error instanceof Drp2WebGpuError) {
    return error;
  }
  const detail = error?.message ?? String(error);
  return new Drp2WebGpuError(
    commandIndex,
    command,
    classifyDrp2WebGpuError(detail),
    detail,
    error,
  );
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
  if (storeOp === "dont_care") {
    return "discard";
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
    case "r8unorm":
    case "r16float":
    case "r32uint":
    case "r32sint":
    case "rg32uint":
    case "rgba8unorm":
    case "bgra8unorm":
    case "rgba16float":
    case "depth32float":
      return format;
    default:
      throw new Error(`unsupported texture format: ${format}`);
  }
}



function effectiveCapabilities(runtimeCaps, streamCaps) {
  return {
    ...(runtimeCaps ?? {}),
    ...(streamCaps ?? {}),
  };
}



function textureFormatBytes(format) {
  switch (format) {
    case "r8unorm":
      return 1;
    case "r16float":
      return 2;
    case "r32uint":
    case "r32sint":
    case "rgba8unorm":
    case "bgra8unorm":
    case "depth32float":
      return 4;
    case "rg32uint":
    case "rgba16float":
      return 8;
    default:
      throw new Error(`unsupported texture format: ${format}`);
  }
}



function mapBlendFactor(factor) {
  if (Number.isInteger(factor)) {
    const factors = [
      "zero",
      "one",
      "src",
      "one-minus-src",
      "dst",
      "one-minus-dst",
      "src-alpha",
      "one-minus-src-alpha",
      "dst-alpha",
      "one-minus-dst-alpha",
      "constant",
      "one-minus-constant",
      "constant",
      "one-minus-constant",
      "src-alpha-saturated",
    ];
    if (factor >= 0 && factor < factors.length) {
      return factors[factor];
    }
  }
  switch (factor) {
    case "zero":
    case "one":
    case "src":
    case "one-minus-src":
    case "src-alpha":
    case "one-minus-src-alpha":
    case "dst":
    case "one-minus-dst":
    case "dst-alpha":
    case "one-minus-dst-alpha":
    case "src-alpha-saturated":
    case "constant":
    case "one-minus-constant":
      return factor;
    default:
      throw new Error(`unsupported blend factor: ${factor}`);
  }
}



function mapBlendOperation(operation) {
  if (Number.isInteger(operation)) {
    const operations = ["add", "subtract", "reverse-subtract", "min", "max"];
    if (operation >= 0 && operation < operations.length) {
      return operations[operation];
    }
  }
  switch (operation) {
    case "add":
    case "subtract":
    case "reverse-subtract":
    case "min":
    case "max":
      return operation;
    default:
      throw new Error(`unsupported blend operation: ${operation}`);
  }
}



function mapBlendComponent(component) {
  return {
    srcFactor: mapBlendFactor(required(component.src_factor, "blend component needs src_factor")),
    dstFactor: mapBlendFactor(required(component.dst_factor, "blend component needs dst_factor")),
    operation: mapBlendOperation(required(component.operation, "blend component needs operation")),
  };
}



function mapBlendState(blend) {
  if (blend === undefined) {
    return undefined;
  }
  return {
    color: mapBlendComponent(required(blend.color, "blend needs color component")),
    alpha: mapBlendComponent(required(blend.alpha, "blend needs alpha component")),
  };
}



function mapColorWriteMask(writeMask) {
  if (writeMask === undefined) {
    return GPUColorWrite.ALL;
  }
  let flags = 0;
  for (const item of writeMask) {
    switch (item) {
      case "red":
        flags |= GPUColorWrite.RED;
        break;
      case "green":
        flags |= GPUColorWrite.GREEN;
        break;
      case "blue":
        flags |= GPUColorWrite.BLUE;
        break;
      case "alpha":
        flags |= GPUColorWrite.ALPHA;
        break;
      case "all":
        flags |= GPUColorWrite.ALL;
        break;
      default:
        throw new Error(`unsupported color write mask: ${item}`);
    }
  }
  return flags;
}



function makeColorTarget(canvasFormat, target) {
  const format = colorTargetFormat(canvasFormat, target);
  validateColorTargetState(format, target);
  return {
    format,
    blend: mapBlendState(target.blend),
    writeMask: mapColorWriteMask(target.write_mask),
  };
}



function colorTargetFormat(canvasFormat, target) {
  const streamFormat = required(target.format, "color target needs format");
  return isBrowserCanvasFormatAlias(streamFormat) ? canvasFormat : mapTextureFormat(streamFormat);
}



function validateColorTargetState(format, target) {
  if (
    target.blend !== undefined &&
    (format === "r32uint" || format === "r32sint" || format === "rg32uint" || format === "depth32float")
  ) {
    throw new Error(`color target format ${format} does not support blending`);
  }
  if (target.write_mask !== undefined && target.write_mask.includes("all") && target.write_mask.length > 1) {
    throw new Error("color target write_mask cannot combine all with individual channels");
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
  if (command.data instanceof Uint8Array) {
    return command.data;
  }
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



function makeVertexBuffers(command, vertexShader, options = {}) {
  const vertexBuffers = command.vertex_buffers ?? [];
  if (vertexBuffers.length === 0) {
    if ((command.vertex_buffer_slots ?? 0) > 0 && vertexShader.code.includes("@location(0)")) {
      if (options.allowDemoCompatibility !== true) {
        throw new Error(
          `render pipeline ${command.id} needs explicit vertex_buffers for shader inputs`,
        );
      }
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



function depthStencilFormat(command) {
  return command.depth_stencil === undefined
    ? null
    : mapTextureFormat(required(command.depth_stencil.format, "depth_stencil needs format"));
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
        texture: { sampleType: options.textureSampleType ?? "float" },
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
    case "storage_texture":
      return {
        binding,
        visibility: shaderStageVisibility(entry.visibility, ["COMPUTE"]),
        storageTexture: {
          access: entry.access ?? "write-only",
          format: mapTextureFormat(entry.format ?? "rgba8unorm"),
        },
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



function shaderTextureSampleType(shader, binding) {
  const escaped = String(binding).replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const pattern = new RegExp(
    `@binding\\(${escaped}\\)[\\s\\S]*?var\\s+\\w+\\s*:\\s*texture_(?:2d|3d|cube|2d_array)<\\s*(i32|u32|f32)\\s*>`,
  );
  const match = pattern.exec(shader.code);
  switch (match?.[1]) {
    case "i32":
      return "sint";
    case "u32":
      return "uint";
    default:
      return "float";
  }
}



function textureSampleTypeForFormat(format) {
  if (typeof format === "string" && format.includes("sint")) {
    return "sint";
  }
  if (typeof format === "string" && format.includes("uint")) {
    return "uint";
  }
  return "float";
}



function specializeBindGroupLayout(device, layoutRecord, shader, options = {}) {
  const entries = layoutRecord.entries.map((entry) => {
    const access = entry.binding_type === "storage_buffer"
      ? (entry.access ?? shaderStorageAccess(shader, entry.binding))
      : "read_write";
    const textureSampleType = entry.binding_type === "sampled_texture"
      ? shaderTextureSampleType(shader, entry.binding)
      : undefined;
    return makeBindGroupLayoutEntry(entry, access, { ...options, textureSampleType });
  });
  return {
    entries: layoutRecord.entries,
    layout: device.createBindGroupLayout({ entries }),
  };
}



function specializeBindGroupLayoutForResources(device, layoutRecord, command, state, options = {}) {
  const entries = layoutRecord.entries.map((entry) => {
    const bindEntry = command.entries?.find((item) => item.binding === entry.binding);
    let textureSampleType = undefined;
    if (entry.binding_type === "sampled_texture" && bindEntry !== undefined) {
      const kind = bindEntry.resource_kind === "texture_view" ? "texture_view" : "texture";
      const record = requireLiveRecord(state, bindEntry.resource_id, kind);
      textureSampleType = textureSampleTypeForFormat(record.format);
    }
    return makeBindGroupLayoutEntry(entry, "read_write", { ...options, textureSampleType });
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
      if (offset % 256 !== 0) {
        throw new Error(`buffer binding ${binding} offset ${offset} is not WebGPU-aligned`);
      }
      return {
        binding,
        resource: {
          buffer: required(buffers.get(entry.resource_id), `unknown buffer ${entry.resource_id}`),
          offset,
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



function resourceKindForBindingType(bindingType) {
  switch (bindingType) {
    case "uniform_buffer":
    case "storage_buffer":
      return "buffer";
    case "sampled_texture":
    case "storage_texture":
      return "texture";
    case "sampler":
      return "sampler";
    default:
      throw new Error(`unsupported bind-group layout binding_type: ${bindingType}`);
  }
}



function validateBindGroupEntry(layoutEntry, entry, resourceRecord) {
  if (layoutEntry.binding_type !== entry.binding_type) {
    throw new Error(`bind-group entry binding type does not match layout binding ${entry.binding}`);
  }
  const expectedKind = resourceKindForBindingType(layoutEntry.binding_type);
  const textureViewAllowed =
    expectedKind === "texture" && entry.resource_kind === "texture_view";
  if (entry.resource_kind !== expectedKind && !textureViewAllowed) {
    throw new Error(`bind-group entry resource kind does not match layout binding ${entry.binding}`);
  }
  if (entry.resource_kind === "texture_view") {
    return;
  }
  if (layoutEntry.binding_type === "sampled_texture") {
    requireUsage(resourceRecord, "TEXTURE_BINDING");
  } else if (layoutEntry.binding_type === "storage_texture") {
    requireUsage(resourceRecord, "STORAGE_BINDING");
  } else if (layoutEntry.binding_type === "uniform_buffer") {
    requireUsage(resourceRecord, "UNIFORM");
  } else if (layoutEntry.binding_type === "storage_buffer") {
    requireUsage(resourceRecord, "STORAGE");
  }
  if (layoutEntry.has_dynamic_offset && (entry.offset === undefined || entry.size === undefined)) {
    throw new Error(`dynamic bind-group binding ${entry.binding} needs offset and size`);
  }
}



function dynamicLayoutEntries(layoutRecord) {
  return layoutRecord.entries.filter((entry) => entry.has_dynamic_offset);
}



function validateDynamicOffsets(state, layoutRecord, bindGroupRecord, dynamicOffsets) {
  const dynamicEntries = dynamicLayoutEntries(layoutRecord);
  if (dynamicOffsets.length !== dynamicEntries.length) {
    throw new Error(
      `dynamic offset count ${dynamicOffsets.length} does not match layout count ${dynamicEntries.length}`,
    );
  }
  for (let i = 0; i < dynamicEntries.length; i++) {
    const layoutEntry = dynamicEntries[i];
    const bindEntry = bindGroupRecord.entries.find((entry) => entry.binding === layoutEntry.binding);
    if (bindEntry === undefined || bindEntry.resource_kind !== "buffer") {
      throw new Error(`dynamic bind-group binding ${layoutEntry.binding} needs buffer resource`);
    }
    const baseOffset = bindEntry.offset ?? 0;
    const size = required(bindEntry.size, `dynamic bind-group binding ${layoutEntry.binding} needs size`);
    const record = requireLiveRecord(state, bindEntry.resource_id, "buffer");
    if (baseOffset + dynamicOffsets[i] + size > record.size) {
      throw new Error(`dynamic offset for binding ${layoutEntry.binding} is out of range`);
    }
  }
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



function commandExtent(command, state) {
  return {
    width: resolveTextureExtentValue(command.width, "width", state),
    height: resolveTextureExtentValue(command.height, "height", state),
    depth: command.depth ?? 1,
  };
}



function mipExtent(textureRecord, mipLevel) {
  return {
    width: Math.max(1, textureRecord.width >> mipLevel),
    height: Math.max(1, textureRecord.height >> mipLevel),
    depth: textureRecord.depth,
  };
}



function requireUsage(record, usage) {
  if (!record.usage?.has(usage)) {
    throw new Error(`${objectLabel(record.kind)} ${record.id} lacks ${usage} usage`);
  }
}



function validateTextureCapabilities(capabilities, command, format, extent) {
  if (
    Array.isArray(capabilities.supported_texture_formats) &&
    !capabilities.supported_texture_formats.includes(format)
  ) {
    throw new Error(`texture format ${format} is not supported by capabilities`);
  }
  const sampleCount = command.sample_count ?? 1;
  if (
    Array.isArray(capabilities.supported_sample_counts) &&
    !capabilities.supported_sample_counts.includes(sampleCount)
  ) {
    throw new Error(`sample count ${sampleCount} is not supported by capabilities`);
  }
  const maxTextureDimension2d = capabilities.max_texture_dimension_2d;
  if (
    (command.dimension ?? "2d") === "2d" &&
    Number.isFinite(maxTextureDimension2d) &&
    (extent.width > maxTextureDimension2d || extent.height > maxTextureDimension2d)
  ) {
    throw new Error(`2d texture extent ${extent.width}x${extent.height} is not supported by capabilities`);
  }
}



const SUPPORTED_DRP2_COMMANDS = new Set([
  "HelloRenderer",
  "RendererHelloReply",
  "Error",
  "CreateBuffer",
  "DestroyBuffer",
  "WriteBuffer",
  "CreateTexture",
  "DestroyTexture",
  "CreateTextureView",
  "DestroyTextureView",
  "WriteTexture",
  "CreateSampler",
  "DestroySampler",
  "CreateBindGroupLayout",
  "DestroyBindGroupLayout",
  "CreateBindGroup",
  "DestroyBindGroup",
  "CreateShaderModule",
  "DestroyShaderModule",
  "CreateRenderPipeline",
  "DestroyRenderPipeline",
  "CreateComputePipeline",
  "DestroyComputePipeline",
  "BeginCommandEncoder",
  "FinishCommandEncoder",
  "BeginRenderPass",
  "EndRenderPass",
  "BeginComputePass",
  "EndComputePass",
  "SetPipeline",
  "SetVertexBuffer",
  "SetIndexBuffer",
  "SetBindGroup",
  "SetViewport",
  "SetScissor",
  "SetBlendConstant",
  "SetStencilReference",
  "Draw",
  "DrawIndexed",
  "DispatchWorkgroups",
  "ResourceBarrier",
  "CopyBufferToBuffer",
  "CopyBufferToTexture",
  "CopyTextureToBuffer",
  "CopyTextureToTexture",
  "QueueSubmit",
  "QueueSubmitReply",
]);



function unsupportedCapability(commandIndex, command, detail) {
  throw new Drp2WebGpuError(
    commandIndex,
    command,
    "DRP2_ERR_UNSUPPORTED_CAPABILITY",
    detail,
  );
}



function featureRequired(commandIndex, command, detail) {
  throw new Drp2WebGpuError(commandIndex, command, "DRP2_ERR_FEATURE_REQUIRED", detail);
}



function requireCapabilityValue(commandIndex, command, capabilities, field, value, label = field) {
  const supported = capabilities[field];
  if (Array.isArray(supported) && !supported.includes(value)) {
    unsupportedCapability(
      commandIndex,
      command,
      `${label} ${value} is not supported by capabilities ${field}=${JSON.stringify(supported)}`,
    );
  }
}



function validateTextureCapabilityPreflight(commandIndex, command, capabilities, state) {
  const format = required(command.format, "CreateTexture needs format");
  validateBrowserCanvasTextureExtentAlias(command);
  requireCapabilityValue(
    commandIndex,
    command,
    capabilities,
    "supported_texture_formats",
    format,
    "texture format",
  );
  const mappedFormat = mapTextureFormat(format);
  const extent = commandExtent(command, state);
  validateTextureCapabilities(capabilities, command, mappedFormat, extent);
}



function validateShaderCapabilityPreflight(commandIndex, command, capabilities) {
  const stage = required(command.stage, "CreateShaderModule needs stage");
  if (stage === "") {
    unsupportedCapability(commandIndex, command, "CreateShaderModule needs non-empty stage");
  }
  const format = required(command.format, "CreateShaderModule needs format");
  if (format === "") {
    unsupportedCapability(commandIndex, command, "CreateShaderModule needs non-empty format");
  }
  const code = required(command.code, "CreateShaderModule needs code");
  if (code === "") {
    unsupportedCapability(commandIndex, command, "CreateShaderModule needs non-empty code");
  }
  requireCapabilityValue(
    commandIndex,
    command,
    capabilities,
    "supported_shader_formats",
    format,
    "shader format",
  );
  if (format !== "wgsl") {
    unsupportedCapability(commandIndex, command, `unsupported shader format: ${format}`);
  }
  if (command.required_features?.includes("fp64") && capabilities.supports_fp64 === false) {
    featureRequired(commandIndex, command, "shader module requires feature fp64");
  }
}



function validateRenderPipelineCapabilityPreflight(commandIndex, command, capabilities, canvasFormat) {
  const colorTargets = command.color_targets ?? [];
  for (const target of colorTargets) {
    const format = colorTargetFormat(canvasFormat, target);
    requireCapabilityValue(
      commandIndex,
      command,
      capabilities,
      "supported_texture_formats",
      format,
      "color target format",
    );
  }

  const depthFormat = depthStencilFormat(command);
  if (depthFormat !== null) {
    requireCapabilityValue(
      commandIndex,
      command,
      capabilities,
      "supported_texture_formats",
      depthFormat,
      "depth/stencil format",
    );
  }
  requireCapabilityValue(
    commandIndex,
    command,
    capabilities,
    "supported_sample_counts",
    command.multisample?.sample_count ?? 1,
    "sample count",
  );
}



function validateStreamCapabilities(commands, capabilities, canvasFormat, state) {
  for (const [commandIndex, command] of commands.entries()) {
    try {
      if (!SUPPORTED_DRP2_COMMANDS.has(command.cmd)) {
        unsupportedCapability(
          commandIndex,
          command,
          `unsupported DRP2 command in WebGPU PoC: ${command.cmd}`,
        );
      }

      switch (command.cmd) {
        case "CreateTexture":
          validateTextureCapabilityPreflight(commandIndex, command, capabilities, state);
          break;

        case "CreateShaderModule":
          validateShaderCapabilityPreflight(commandIndex, command, capabilities);
          break;

        case "CreateRenderPipeline":
        case "CreateComputePipeline":
          validateRenderPipelineCapabilityPreflight(
            commandIndex,
            command,
            capabilities,
            canvasFormat,
          );
          break;

        default:
          break;
      }
    } catch (error) {
      throw wrapDrp2WebGpuError(commandIndex, command, error);
    }
  }
}



function validateTextureRange(textureRecord, mipLevel, origin, size) {
  if (mipLevel >= textureRecord.mipLevelCount) {
    throw new Error(`mip level ${mipLevel} is out of range`);
  }
  const extent = mipExtent(textureRecord, mipLevel);
  if (
    (origin.x ?? 0) + required(size.width, "texture copy size needs width") > extent.width ||
    (origin.y ?? 0) + required(size.height, "texture copy size needs height") > extent.height ||
    (origin.z ?? 0) + (size.depth ?? 1) > extent.depth
  ) {
    throw new Error("texture range is out of range");
  }
}



function validateTextureLayout(textureRecord, size, bytesPerRow, rowsPerImage, payloadSize = null) {
  const rowBytes = required(size.width, "texture layout size needs width") *
    textureFormatBytes(textureRecord.format);
  const height = required(size.height, "texture layout size needs height");
  const depth = size.depth ?? 1;
  if (bytesPerRow < rowBytes) {
    throw new Error(`bytes_per_row ${bytesPerRow} is smaller than row layout ${rowBytes}`);
  }
  if (depth > 1 && rowsPerImage < height) {
    throw new Error(`rows_per_image ${rowsPerImage} is smaller than texture layout height ${height}`);
  }
  if (payloadSize !== null) {
    const requiredBytes = depth > 1
      ? (depth - 1) * rowsPerImage * bytesPerRow + (height - 1) * bytesPerRow + rowBytes
      : (height - 1) * bytesPerRow + rowBytes;
    if (payloadSize < requiredBytes) {
      throw new Error(`texture payload ${payloadSize} is smaller than layout footprint ${requiredBytes}`);
    }
  }
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



function canvasExtent(state) {
  const canvas = state?.canvas;
  if (canvas === undefined || canvas === null) {
    throw new Error("browser canvas extent needs an active canvas");
  }
  return {
    width: Math.max(1, canvas.width),
    height: Math.max(1, canvas.height),
  };
}



function resolveTextureExtentValue(value, axis, state) {
  if (isBrowserCanvasExtentAlias(value)) {
    return canvasExtent(state)[axis];
  }
  return required(value, `CreateTexture needs ${axis}`);
}



function validateBrowserCanvasTextureExtentAlias(command) {
  const widthIsCanvas = isBrowserCanvasExtentAlias(command.width);
  const heightIsCanvas = isBrowserCanvasExtentAlias(command.height);
  if (widthIsCanvas !== heightIsCanvas) {
    throw new Error(
      "CreateTexture canvas extent alias requires both width and height to be canvas",
    );
  }
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
  if (vertexShader.stage !== undefined && vertexShader.stage !== "VERTEX") {
    throw new Error(`vertex shader module ${command.vertex_shader_module_id} has invalid stage`);
  }
  if (fragmentShader.stage !== undefined && fragmentShader.stage !== "FRAGMENT") {
    throw new Error(`fragment shader module ${command.fragment_shader_module_id} has invalid stage`);
  }

  if (options.requireExplicitPipelineMetadata && command.vertex_buffers === undefined) {
    throw new Error(`render pipeline ${command.id} needs explicit vertex_buffers`);
  }
  if (options.requireExplicitPipelineMetadata && command.color_targets === undefined) {
    throw new Error(`render pipeline ${command.id} needs explicit color_targets`);
  }

  const colorTargets = command.color_targets ?? [defaultColorTarget(fragmentShader)];
  const colorTargetFormats = colorTargets.map((target) => colorTargetFormat(canvasFormat, target));
  colorTargets.forEach((target, index) =>
    validateColorTargetState(colorTargetFormats[index], target),
  );
  const pipelineDepthStencilFormat = depthStencilFormat(command);
  const bindGroupLayoutIds = command.bind_group_layout_ids ?? [];
  const pipelineBindGroupLayouts = bindGroupLayoutIds.map((id) =>
    specializeBindGroupLayout(
      device,
      required(bindGroupLayouts.get(id), `unknown bind-group layout ${id}`),
      fragmentShader,
      options,
    ),
  );
  const layout = pipelineBindGroupLayouts.length > 0
    ? device.createPipelineLayout({
        bindGroupLayouts: pipelineBindGroupLayouts.map((record) => record.layout),
      })
    : "auto";
  const sampleCount = command.multisample?.sample_count ?? 1;

  return {
    bindGroupLayoutIds,
    bindGroupLayouts: pipelineBindGroupLayouts,
    colorTargetFormats,
    depthStencilFormat: pipelineDepthStencilFormat,
    sampleCount,
    vertexBufferSlots: command.vertex_buffer_slots ?? (command.vertex_buffers ?? []).length,
    pipeline: device.createRenderPipeline({
      label: command.label,
      layout,
      vertex: {
        module: vertexShader.module,
        entryPoint: vertexShader.entryPoint,
        buffers: makeVertexBuffers(command, vertexShader, options),
      },
      fragment: {
        module: fragmentShader.module,
        entryPoint: fragmentShader.entryPoint,
        targets: colorTargets.map((target) => makeColorTarget(canvasFormat, target)),
      },
      primitive: {
        topology: mapTopology(command.topology),
      },
      depthStencil: makeDepthStencil(command),
      multisample: {
        count: sampleCount,
        alphaToCoverageEnabled: command.multisample?.alpha_to_coverage_enabled ?? false,
      },
    }),
  };
}



function makeComputePipeline(device, shaders, bindGroupLayouts, command, options = {}) {
  const shader = required(
    shaders.get(command.compute_shader_module_id),
    `unknown compute shader module ${command.compute_shader_module_id}`,
  );
  if (shader.stage !== undefined && shader.stage !== "COMPUTE") {
    throw new Error(`compute shader module ${command.compute_shader_module_id} has invalid stage`);
  }
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
    bindGroupLayoutIds,
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



function attachmentExtent(state, attachment) {
  if (isBrowserCanvasTextureId(attachment.texture_id)) {
    return canvasExtent(state);
  }
  const record = requireLiveRecord(state, attachment.texture_id, "texture");
  return { width: record.width, height: record.height };
}



function renderPassExtent(state, command) {
  const attachments = required(command.color_attachments, "BeginRenderPass needs color_attachments");
  return attachmentExtent(state, attachments[0]);
}


function applyInitialRenderPassState(pass, command) {
  if (command.viewport !== undefined) {
    pass.setViewport(
      command.viewport.x,
      command.viewport.y,
      command.viewport.width,
      command.viewport.height,
      command.viewport.min_depth ?? 0,
      command.viewport.max_depth ?? 1,
    );
  }
  if (command.scissor !== undefined) {
    pass.setScissorRect(
      command.scissor.x,
      command.scissor.y,
      command.scissor.width,
      command.scissor.height,
    );
  }
}



function makeDepthStencilAttachment(device, state, textures, command) {
  const attachment = command.depth_stencil_attachment;
  if (attachment === undefined) {
    return undefined;
  }
  const texture = isBrowserCanvasTextureId(attachment.texture_id)
    ? browserCanvasDepthTexture(device, state, renderPassExtent(state, command), command.label)
    : required(textures.get(attachment.texture_id), `unknown depth texture ${attachment.texture_id}`);
  return {
    view: texture.createView(),
    depthLoadOp: mapLoadOp(attachment.depth_load_op),
    depthStoreOp: mapStoreOp(attachment.depth_store_op),
    depthClearValue: attachment.depth_clear_value ?? 1,
  };
}



function makeColorAttachment(device, state, canvasFormat, textures, attachment, label = undefined) {
  const extent = attachmentExtent(state, attachment);
  const textureView = isBrowserCanvasTextureId(attachment.texture_id)
    ? browserPresentTexture(
        device, state, extent, state.browserPresentFormat, label)
        .createView()
    : required(textures.get(attachment.texture_id), `unknown texture ${attachment.texture_id}`)
        .createView();
  if (isBrowserCanvasTextureId(attachment.texture_id)) {
    state.browserPresent.pending = true;
  }
  const result = {
    view: textureView,
    loadOp: mapLoadOp(attachment.load_op),
    storeOp: mapStoreOp(attachment.store_op),
    clearValue: clearValue(attachment.clear_value),
  };
  if (attachment.resolve_target !== undefined) {
    const resolveTextureId = required(
      attachment.resolve_target.texture_id,
      "resolve target needs texture_id",
    );
    result.resolveTarget = isBrowserCanvasTextureId(resolveTextureId)
      ? browserPresentTexture(device, state, extent, state.browserPresentFormat, label).createView()
      : required(textures.get(resolveTextureId), `unknown resolve texture ${resolveTextureId}`)
          .createView();
    if (isBrowserCanvasTextureId(resolveTextureId)) {
      state.browserPresent.pending = true;
    }
  }
  return result;
}



function beginRenderPass(device, state, canvasFormat, textures, encoders, command) {
  const encoder = required(
    encoders.get(command.encoder_id),
    `unknown command encoder ${command.encoder_id}`,
  );
  const attachments = required(command.color_attachments, "BeginRenderPass needs color_attachments");

  const pass = encoder.beginRenderPass({
    label: command.label,
    colorAttachments: attachments.map((attachment) =>
      makeColorAttachment(device, state, canvasFormat, textures, attachment, command.label),
    ),
    depthStencilAttachment: makeDepthStencilAttachment(device, state, textures, command),
  });
  applyInitialRenderPassState(pass, command);
  return pass;
}



function attachmentTextureFormat(state, canvasFormat, attachment) {
  if (isBrowserCanvasTextureId(attachment.texture_id)) {
    return state.browserPresentFormat;
  }
  return requireLiveRecord(state, attachment.texture_id, "texture").format;
}



function depthAttachmentTextureFormat(state, attachment) {
  if (isBrowserCanvasTextureId(attachment.texture_id)) {
    return "depth32float";
  }
  return requireLiveRecord(state, attachment.texture_id, "texture").format;
}



function attachmentTextureSampleCount(state, attachment) {
  if (isBrowserCanvasTextureId(attachment.texture_id)) {
    return 1;
  }
  return requireLiveRecord(state, attachment.texture_id, "texture").sampleCount;
}



function renderPassAttachmentFormats(state, canvasFormat, command) {
  const attachments = required(command.color_attachments, "BeginRenderPass needs color_attachments");
  const colorAttachmentFormats = attachments.map((attachment) =>
    attachmentTextureFormat(state, canvasFormat, attachment),
  );
  const depthStencilFormat = command.depth_stencil_attachment === undefined
    ? null
    : depthAttachmentTextureFormat(state, command.depth_stencil_attachment);
  const sampleCount = attachmentTextureSampleCount(state, attachments[0]);
  return { colorAttachmentFormats, depthStencilFormat, sampleCount };
}



function validateRenderPipelineForPass(passRecord, pipelineRecord, pipelineId) {
  if (pipelineRecord.colorTargetFormats.length !== passRecord.colorAttachmentFormats.length) {
    throw new Error(
      `render pipeline ${pipelineId} color target count ${pipelineRecord.colorTargetFormats.length}` +
        ` does not match render pass ${passRecord.id} color attachment count ` +
        `${passRecord.colorAttachmentFormats.length}`,
    );
  }
  for (let i = 0; i < pipelineRecord.colorTargetFormats.length; i++) {
    if (pipelineRecord.colorTargetFormats[i] !== passRecord.colorAttachmentFormats[i]) {
      throw new Error(
        `render pipeline ${pipelineId} color target ${i} format ` +
          `${pipelineRecord.colorTargetFormats[i]} does not match render pass ${passRecord.id} ` +
          `attachment format ${passRecord.colorAttachmentFormats[i]}`,
      );
    }
  }
  if (pipelineRecord.depthStencilFormat !== passRecord.depthStencilFormat) {
    throw new Error(
      `render pipeline ${pipelineId} depth_stencil format ` +
        `${pipelineRecord.depthStencilFormat ?? "none"} does not match render pass ` +
        `${passRecord.id} attachment format ${passRecord.depthStencilFormat ?? "none"}`,
    );
  }
  if (pipelineRecord.sampleCount !== passRecord.sampleCount) {
    throw new Error(
      `render pipeline ${pipelineId} sample count ${pipelineRecord.sampleCount} does not match ` +
        `render pass ${passRecord.id} sample count ${passRecord.sampleCount}`,
    );
  }
}



function requireNoOpenPass(passes, encoderId) {
  for (const passRecord of passes.values()) {
    if (passRecord.encoderId === encoderId) {
      throw new Error(`command encoder ${encoderId} has an open pass`);
    }
  }
}



function requireBoundPipeline(passRecord) {
  if (passRecord.pipeline === undefined) {
    throw new Error("pass has no bound pipeline");
  }
  return passRecord.pipeline;
}



function validateRenderDrawState(passRecord) {
  const pipeline = requireBoundPipeline(passRecord);
  const vertexBufferSlots = pipeline.vertexBufferSlots ?? 0;
  for (let slot = 0; slot < vertexBufferSlots; slot++) {
    if (!passRecord.vertexBuffers.has(slot)) {
      throw new Error(`missing vertex buffer slot ${slot}`);
    }
  }
}



function validateIndexedDrawState(passRecord) {
  validateRenderDrawState(passRecord);
  if (!passRecord.hasIndexBuffer) {
    throw new Error("missing index buffer");
  }
}



function objectLabel(kind) {
  return kind.replaceAll("_", " ");
}



function registerObject(state, map, id, object, kind, metadata = {}) {
  const existing = state.objects.get(id);
  if (existing !== undefined && existing.destroyed) {
    state.objects.delete(id);
  } else if (existing !== undefined) {
    throw new Error(`duplicate or reused object id ${id}`);
  }
  const record = {
    id,
    kind,
    object,
    destroyed: false,
    dependencies: [],
    dependents: 0,
    openRefs: 0,
    recordedRefs: 0,
    submittedRefs: 0,
    ...metadata,
  };
  state.objects.set(id, record);
  map.set(id, object);
  return record;
}



function requireLiveRecord(state, id, kind) {
  const record = state.objects.get(id);
  if (record === undefined) {
    throw new Error(`unknown ${objectLabel(kind)} ${id}`);
  }
  if (record.kind !== kind) {
    throw new Error(`object ${id} is ${objectLabel(record.kind)}, not ${objectLabel(kind)}`);
  }
  if (record.destroyed) {
    throw new Error(`${objectLabel(kind)} ${id} is destroyed`);
  }
  return record;
}



function retainDependency(owner, dependency) {
  dependency.dependents++;
  owner.dependencies.push(dependency);
}



function releaseDependencies(owner) {
  for (const dependency of owner.dependencies) {
    dependency.dependents = Math.max(0, dependency.dependents - 1);
  }
  owner.dependencies = [];
}



function destroyObject(map, record) {
  if (record.destroyed) {
    throw new Error(`${objectLabel(record.kind)} ${record.id} is already destroyed`);
  }
  if (record.dependents > 0) {
    throw new Error(`${objectLabel(record.kind)} ${record.id} is still referenced by live objects`);
  }
  if (record.openRefs > 0) {
    throw new Error(`${objectLabel(record.kind)} ${record.id} is still referenced by open work`);
  }
  if (record.recordedRefs > 0) {
    throw new Error(`${objectLabel(record.kind)} ${record.id} is still referenced by recorded work`);
  }
  if (record.submittedRefs > 0) {
    throw new Error(`${objectLabel(record.kind)} ${record.id} is still referenced by submitted work`);
  }
  if (typeof record.object.destroy === "function") {
    record.object.destroy();
  }
  releaseDependencies(record);
  record.destroyed = true;
  map.delete(record.id);
}



function objectMapForKind(state, kind) {
  switch (kind) {
    case "buffer":
      return state.buffers;
    case "texture":
      return state.textures;
    case "texture_view":
      return state.textureViews;
    case "sampler":
      return state.samplers;
    case "bind_group_layout":
      return state.bindGroupLayouts;
    case "bind_group":
      return state.bindGroups;
    case "shader_module":
      return state.shaders;
    case "pipeline":
      return state.pipelines;
    default:
      throw new Error(`unsupported object kind ${kind}`);
  }
}



function destroyObjectTree(state, record) {
  for (const dependent of Array.from(state.objects.values())) {
    if (!dependent.destroyed && dependent.dependencies.includes(record)) {
      destroyObjectTree(state, dependent);
    }
  }
  destroyObject(objectMapForKind(state, record.kind), record);
}



function prepareCreateObject(state, id, kind, options) {
  const record = state.objects.get(id);
  if (record === undefined || record.destroyed) {
    return;
  }
  if (record.kind !== kind) {
    throw new Error(`object ${id} is ${objectLabel(record.kind)}, not ${objectLabel(kind)}`);
  }
  if (options.replaceExistingResources !== true) {
    throw new Error(`duplicate or reused object id ${id}`);
  }
  destroyObjectTree(state, record);
}



function addEncoderRef(state, encoderRefs, encoderId, id, kind) {
  const record = requireLiveRecord(state, id, kind);
  const refs = required(encoderRefs.get(encoderId), `unknown command encoder ${encoderId}`);
  if (!refs.has(record)) {
    refs.add(record);
    record.openRefs++;
  }
  return record;
}



function finishEncoderRefs(encoderRefs, commandBuffers, encoderId, commandBufferId, commandBuffer) {
  const refs = required(encoderRefs.get(encoderId), `unknown command encoder ${encoderId}`);
  for (const record of refs) {
    record.openRefs = Math.max(0, record.openRefs - 1);
    record.recordedRefs++;
  }
  commandBuffers.set(commandBufferId, {
    commandBuffer,
    refs,
    submitted: false,
  });
  encoderRefs.delete(encoderId);
}



function submitCommandBuffer(record) {
  if (record.submitted) {
    throw new Error("command buffer has already been submitted");
  }
  for (const objectRecord of record.refs) {
    objectRecord.recordedRefs = Math.max(0, objectRecord.recordedRefs - 1);
    objectRecord.submittedRefs++;
  }
  record.submitted = true;
  return record.commandBuffer;
}



function retireSubmittedRefs(record) {
  if (!record.submitted) {
    return;
  }
  for (const objectRecord of record.refs) {
    objectRecord.submittedRefs = Math.max(0, objectRecord.submittedRefs - 1);
  }
  record.submitted = false;
}



function releaseRefSet(refs, field) {
  for (const objectRecord of refs) {
    objectRecord[field] = Math.max(0, objectRecord[field] - 1);
  }
}



function discardLocalExecutionRefs(encoderRefs, commandBuffers) {
  for (const refs of encoderRefs.values()) {
    releaseRefSet(refs, "openRefs");
  }
  encoderRefs.clear();

  for (const record of commandBuffers.values()) {
    if (!record.submitted) {
      releaseRefSet(record.refs, "recordedRefs");
    }
  }
  commandBuffers.clear();
}



function destroyBrowserCanvasDepth(state) {
  const depth = state.browserCanvasDepth;
  if (depth?.texture !== undefined && typeof depth.texture.destroy === "function") {
    depth.texture.destroy();
  }
  state.browserCanvasDepth = null;
}



function destroyBrowserPresentTexture(record) {
  if (record?.texture !== undefined && typeof record.texture.destroy === "function") {
    record.texture.destroy();
  }
}



function destroyBrowserPresentState(state) {
  if (state === null || state === undefined) {
    return;
  }
  destroyBrowserPresentTexture(state.browserPresent?.current);
  destroyBrowserPresentTexture(state.browserPresent?.previous);
  state.browserPresent = {
    current: null,
    previous: null,
    pending: false,
    resources: null,
  };
}



const BROWSER_PRESENT_SHADER = `
struct VertexOut {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
};

@group(0) @binding(0) var source_texture: texture_2d<f32>;
@group(0) @binding(1) var source_sampler: sampler;

fn linear_to_srgb(linear: vec3f) -> vec3f {
  let clipped = clamp(linear, vec3f(0.0), vec3f(1.0));
  let lo = clipped * vec3f(12.92);
  let hi = vec3f(1.055) * pow(clipped, vec3f(1.0 / 2.4)) - vec3f(0.055);
  return select(hi, lo, clipped <= vec3f(0.0031308));
}

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VertexOut {
  var positions = array<vec2f, 3>(
    vec2f(-1.0, -1.0),
    vec2f(3.0, -1.0),
    vec2f(-1.0, 3.0),
  );
  var uvs = array<vec2f, 3>(
    vec2f(0.0, 1.0),
    vec2f(2.0, 1.0),
    vec2f(0.0, -1.0),
  );
  var out: VertexOut;
  out.position = vec4f(positions[vertex_index], 0.0, 1.0);
  out.uv = uvs[vertex_index];
  return out;
}

@fragment
fn fs_main(in: VertexOut) -> @location(0) vec4f {
  let linear = textureSample(source_texture, source_sampler, in.uv);
  return vec4f(linear_to_srgb(linear.rgb), linear.a);
}
`;



function browserPresentResources(device, state, canvasFormat) {
  const present = required(state.browserPresent, "browser presentation state is missing");
  const resources = present.resources;
  if (resources !== null && resources.canvasFormat === canvasFormat) {
    return resources;
  }

  const module = device.createShaderModule({
    label: "browser-present-shader",
    code: BROWSER_PRESENT_SHADER,
  });
  const bindGroupLayout = device.createBindGroupLayout({
    label: "browser-present-bind-group-layout",
    entries: [
      {
        binding: 0,
        visibility: GPUShaderStage.FRAGMENT,
        texture: { sampleType: "float" },
      },
      {
        binding: 1,
        visibility: GPUShaderStage.FRAGMENT,
        sampler: { type: "filtering" },
      },
    ],
  });
  const pipelineLayout = device.createPipelineLayout({
    label: "browser-present-pipeline-layout",
    bindGroupLayouts: [bindGroupLayout],
  });
  const pipeline = device.createRenderPipeline({
    label: "browser-present-pipeline",
    layout: pipelineLayout,
    vertex: {
      module,
      entryPoint: "vs_main",
    },
    fragment: {
      module,
      entryPoint: "fs_main",
      targets: [{ format: canvasFormat }],
    },
    primitive: {
      topology: "triangle-list",
    },
  });
  const sampler = device.createSampler({
    label: "browser-present-sampler",
    magFilter: "linear",
    minFilter: "linear",
    addressModeU: "clamp-to-edge",
    addressModeV: "clamp-to-edge",
  });
  present.resources = { canvasFormat, bindGroupLayout, pipeline, sampler };
  return present.resources;
}



async function browserPresentRecordToCanvas(device, context, canvasFormat, state, record) {
  if (record === null || record === undefined) {
    return false;
  }
  const resources = browserPresentResources(device, state, canvasFormat);
  const bindGroup = device.createBindGroup({
    label: "browser-present-bind-group",
    layout: resources.bindGroupLayout,
    entries: [
      {
        binding: 0,
        resource: record.texture.createView(),
      },
      {
        binding: 1,
        resource: resources.sampler,
      },
    ],
  });
  const encoder = device.createCommandEncoder({ label: "browser-present-encoder" });
  const pass = encoder.beginRenderPass({
    label: "browser-present-pass",
    colorAttachments: [
      {
        view: context.getCurrentTexture().createView(),
        loadOp: "clear",
        storeOp: "store",
        clearValue: { r: 0, g: 0, b: 0, a: 1 },
      },
    ],
  });
  pass.setPipeline(resources.pipeline);
  pass.setBindGroup(0, bindGroup, []);
  pass.draw(3, 1, 0, 0);
  pass.end();
  device.queue.submit([encoder.finish()]);
  if (typeof device.queue.onSubmittedWorkDone === "function") {
    await device.queue.onSubmittedWorkDone();
  }

  const extent = canvasExtent(state);
  if (
    record === state.browserPresent.current &&
    record.width === extent.width &&
    record.height === extent.height
  ) {
    destroyBrowserPresentTexture(state.browserPresent.previous);
    state.browserPresent.previous = null;
  }
  return true;
}



async function browserPresentStaleFrameIfNeeded(device, context, canvasFormat, state) {
  const present = state.browserPresent;
  const current = present?.current ?? null;
  if (current === null) {
    return false;
  }
  const extent = canvasExtent(state);
  if (
    current.width === extent.width && current.height === extent.height &&
    current.format === state.browserPresentFormat
  ) {
    return false;
  }
  return await browserPresentRecordToCanvas(device, context, canvasFormat, state, current);
}



async function browserPresentPendingFrame(device, context, canvasFormat, state) {
  const present = state.browserPresent;
  if (present?.pending !== true || present.current === null) {
    return false;
  }
  present.pending = false;
  return await browserPresentRecordToCanvas(device, context, canvasFormat, state, present.current);
}



function browserCanvasDepthTexture(device, state, extent, label = undefined) {
  const width = required(extent.width, "browser depth extent needs width");
  const height = required(extent.height, "browser depth extent needs height");
  const depth = state.browserCanvasDepth;
  if (
    depth !== null &&
    depth.width === width &&
    depth.height === height &&
    depth.format === "depth32float"
  ) {
    return depth.texture;
  }
  destroyBrowserCanvasDepth(state);
  const texture = device.createTexture({
    label: label === undefined ? "browser-canvas-depth" : `${label}:browser-depth`,
    size: { width, height },
    format: "depth32float",
    usage: GPUTextureUsage.RENDER_ATTACHMENT,
  });
  state.browserCanvasDepth = { texture, width, height, format: "depth32float" };
  return texture;
}



function browserPresentTexture(device, state, extent, format, label = undefined) {
  const width = required(extent.width, "browser present extent needs width");
  const height = required(extent.height, "browser present extent needs height");
  const present = required(state.browserPresent, "browser presentation state is missing");
  const current = present.current;
  if (
    current !== null &&
    current.width === width &&
    current.height === height &&
    current.format === format
  ) {
    return current.texture;
  }

  destroyBrowserPresentTexture(present.previous);
  present.previous = current;
  const texture = device.createTexture({
    label: label === undefined ? "browser-present-color" : `${label}:browser-present-color`,
    size: { width, height },
    format,
    usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING,
  });
  present.current = { texture, width, height, format };
  present.pending = true;
  return texture;
}



function destroyExecutionState(state) {
  if (state === null || state === undefined) {
    return;
  }
  destroyBrowserCanvasDepth(state);
  destroyBrowserPresentState(state);
  for (const record of state.objects.values()) {
    if (!record.destroyed && typeof record.object.destroy === "function") {
      record.object.destroy();
    }
    record.destroyed = true;
  }
}



function createExecutionState(canvas = null, browserPresentFormat = null) {
  return {
    canvas,
    browserPresentFormat,
    objects: new Map(),
    buffers: new Map(),
    textures: new Map(),
    textureViews: new Map(),
    samplers: new Map(),
    bindGroupLayouts: new Map(),
    bindGroups: new Map(),
    shaders: new Map(),
    pipelines: new Map(),
    browserCanvasDepth: null,
    browserPresent: {
      current: null,
      previous: null,
      pending: false,
      resources: null,
    },
  };
}



function resourceStats(state) {
  const byKind = {};
  let destroyedObjects = 0;
  let openRefs = 0;
  let recordedRefs = 0;
  let submittedRefs = 0;
  for (const record of state.objects.values()) {
    byKind[record.kind] = (byKind[record.kind] ?? 0) + 1;
    if (record.destroyed) {
      destroyedObjects++;
    }
    openRefs += record.openRefs ?? 0;
    recordedRefs += record.recordedRefs ?? 0;
    submittedRefs += record.submittedRefs ?? 0;
  }
  return {
    objects: state.objects.size,
    destroyedObjects,
    buffers: state.buffers.size,
    textures: state.textures.size,
    textureViews: state.textureViews.size,
    samplers: state.samplers.size,
    bindGroupLayouts: state.bindGroupLayouts.size,
    bindGroups: state.bindGroups.size,
    shaders: state.shaders.size,
    pipelines: state.pipelines.size,
    browserCanvasDepthTextures: state.browserCanvasDepth === null ? 0 : 1,
    browserPresentTextures:
      (state.browserPresent?.current === null ? 0 : 1) +
      (state.browserPresent?.previous === null ? 0 : 1),
    byKind,
    refs: {
      open: openRefs,
      recorded: recordedRefs,
      submitted: submittedRefs,
    },
  };
}



const FRAME_PHASE_COMMANDS = new Set([
  "BeginCommandEncoder",
  "BeginRenderPass",
  "BeginComputePass",
  "SetViewport",
  "SetScissor",
  "SetPipeline",
  "SetBindGroup",
  "SetVertexBuffer",
  "SetIndexBuffer",
  "Draw",
  "DrawIndexed",
  "EndRenderPass",
  "DispatchWorkgroups",
  "EndComputePass",
  "ResourceBarrier",
  "CopyBufferToBuffer",
  "CopyBufferToTexture",
  "CopyTextureToBuffer",
  "CopyTextureToTexture",
  "FinishCommandEncoder",
  "QueueSubmit",
  "QueueSubmitReply",
]);



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
    setupCommands: commands.filter((command) => !FRAME_PHASE_COMMANDS.has(command.cmd)),
    frameCommands: commands.filter((command) => FRAME_PHASE_COMMANDS.has(command.cmd)),
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
    let error = null;
    try {
      error = await device.popErrorScope();
    } catch (popError) {
      if (String(popError?.message ?? popError).includes("Instance dropped in popErrorScope")) {
        continue;
      }
      throw popError;
    }
    if (error !== null) {
      errors.push(`${scope}: ${error.message}`);
    }
  }
  const reportableErrors = errors.filter(
    (error) => !error.includes("Instance dropped in popErrorScope"),
  );

  if (thrown !== null) {
    throw thrown;
  }
  if (reportableErrors.length > 0) {
    throw new Error(reportableErrors.join("\n"));
  }
  return result;
}



export class Drp2WebGpuRuntime {
  constructor(device, context, canvasFormat, options = {}) {
    this.device = device;
    this.context = context;
    this.canvasFormat = canvasFormat;
    this.options = options;
    this.canvas = options.canvas ?? null;
    this.state = createExecutionState(this.canvas, options.browserPresentFormat ?? canvasFormat);
    this.stream = null;
    this.setupCommands = [];
    this.frameCommands = [];
    this.frames = [];
    this.packetResourceVersion = null;
    this.packetFrameIndex = null;
  }

  destroy() {
    destroyExecutionState(this.state);
    this.state = createExecutionState(
      this.canvas, this.options.browserPresentFormat ?? this.canvasFormat);
    this.stream = null;
    this.setupCommands = [];
    this.frameCommands = [];
    this.frames = [];
    this.packetResourceVersion = null;
    this.packetFrameIndex = null;
  }

  async load(stream, options = {}) {
    this.stream = stream;
    this.options = { ...this.options, ...options };
    destroyExecutionState(this.state);
    this.canvas = this.options.canvas ?? this.canvas;
    this.state = createExecutionState(
      this.canvas, this.options.browserPresentFormat ?? this.canvasFormat);

    const split = splitStreamCommands(stream);
    this.setupCommands = split.setupCommands;
    this.frameCommands = split.frameCommands;
    this.frames = Array.isArray(stream.frames) ? stream.frames : [];
    this.packetResourceVersion = null;
    this.packetFrameIndex = null;

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

  async update(stream, options = {}) {
    if (this.stream === null) {
      return await this.load(stream, options);
    }

    this.stream = stream;
    this.options = { ...this.options, ...options };

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
        replaceExistingResources: true,
        state: this.state,
      },
    );
  }

  async executePacketSet(packetSet, options = {}) {
    const decoded = decodeDrp2PacketSet(packetSet);
    this.options = { ...this.options, ...options };

    if (this.stream === null || options.reset === true) {
      destroyExecutionState(this.state);
      this.canvas = this.options.canvas ?? this.canvas;
      this.state = createExecutionState(
        this.canvas, this.options.browserPresentFormat ?? this.canvasFormat);
      this.packetResourceVersion = null;
      this.packetFrameIndex = null;
    }

    const setupCommands = decoded.setup?.commands ?? [];
    const updateCommands = decoded.update?.commands ?? [];
    const frameCommands = decoded.frame?.commands ?? [];
    if (decoded.frame === undefined) {
      throw new Error("DRP2 packet set needs a frame packet");
    }
    const packets = [decoded.setup, decoded.update, decoded.frame].filter((packet) => packet !== undefined);
    const resourceVersion = packets[0].resource_version;
    const frameIndex = packets[0].frame_index;
    if (packetSet.source === "wasm_frame_artifact") {
      if (packetSet.artifact_spans_copied !== true || packetSet.artifact_released !== true) {
        throw new Error("WASM frame artifact packet spans must be copied and released before execution");
      }
      if (
        packetSet.artifact_resource_version !== resourceVersion ||
        packetSet.artifact_frame_index !== frameIndex ||
        packetSet.resource_version !== resourceVersion ||
        packetSet.frame_index !== frameIndex
      ) {
        throw new Error("WASM frame artifact packet counters do not match encoded packets");
      }
    }
    for (const packet of packets) {
      if (packet.resource_version !== resourceVersion || packet.frame_index !== frameIndex) {
        throw new Error("DRP2 packet set phases have inconsistent version counters");
      }
    }
    if (this.packetResourceVersion !== null && resourceVersion < this.packetResourceVersion) {
      throw new Error("stale DRP2 packet resource_version");
    }
    if (this.packetFrameIndex !== null && frameIndex <= this.packetFrameIndex) {
      throw new Error("stale DRP2 packet frame_index");
    }
    this.setupCommands = setupCommands;
    this.frameCommands = frameCommands;
    this.frames = [];
    this.stream = {
      name: "drp2_packet_set",
      version: { major: 2, minor: 0 },
      commands: [...setupCommands, ...updateCommands, ...frameCommands],
    };

    const baseOptions = {
      ...this.options,
      state: this.state,
      replaceExistingResources: options.replaceExistingResources ?? true,
    };
    const runPackets = async () => {
      if (setupCommands.length > 0) {
        await executeDrp2Stream(
          this.device, this.context, this.canvasFormat, this.stream,
          { ...baseOptions, commands: setupCommands },
        );
      }
      if (updateCommands.length > 0) {
        await executeDrp2Stream(
          this.device, this.context, this.canvasFormat, this.stream,
          { ...baseOptions, commands: updateCommands },
        );
      }
      if (frameCommands.length === 0) {
        this.packetResourceVersion = resourceVersion;
        this.packetFrameIndex = frameIndex;
        return null;
      }
      const result = await executeDrp2Stream(
        this.device, this.context, this.canvasFormat, this.stream,
        {
          ...baseOptions,
          retireSubmittedRefs: options.retireSubmittedRefs ?? true,
          commands: frameCommands,
        },
      );
      this.packetResourceVersion = resourceVersion;
      this.packetFrameIndex = frameIndex;
      return result;
    };
    return options.errorScopes === false ? await runPackets()
                                         : await executeWithErrorScopes(this.device, runPackets);
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
        retireSubmittedRefs: options.retireSubmittedRefs ?? true,
        commands,
        state: this.state,
      },
    );
  }

  resourceStats() {
    return resourceStats(this.state);
  }
}



export async function fetchWebGpuStream(path) {
  const response = await fetch(path, { cache: "no-cache" });
  if (!response.ok) {
    throw new Error(`failed to load stream: ${response.status} ${response.statusText}`);
  }
  return await response.json();
}



export class WebGpuDemoSession {
  constructor(device, context, canvasFormat, capabilities, options = {}) {
    this.device = device;
    this.context = context;
    this.canvasFormat = canvasFormat;
    this.capabilities = capabilities;
    this.canvas = options.canvas ?? null;
    this.onStreamLoaded = typeof options.onStreamLoaded === "function" ? options.onStreamLoaded : null;
    this.streamName = null;
    this.config = null;
    this.stream = null;
    this.runtime = null;
    this.frameIndex = 0;
  }

  async loadStream(name) {
    const config = streamConfigByName(name);
    const sourceName = streamSourceName(config);
    const stream = await fetchWebGpuStream(`./streams/${sourceName}.json`);
    return await this.loadStreamObject(config.name, stream);
  }

  async loadStreamObject(name, stream) {
    this.config = streamConfigByName(name);
    this.streamName = this.config.name;
    this.stream = stream;
    this.runtime = new Drp2WebGpuRuntime(
      this.device,
      this.context,
      this.canvasFormat,
      {
        allowDemoCompatibility: true,
        canvas: this.canvas,
        capabilities: this.capabilities,
      },
    );
    if (this.onStreamLoaded !== null) {
      this.onStreamLoaded(stream);
    }
    await this.runtime.load(stream);
    this.frameIndex = 0;
    return this;
  }

  async reload() {
    if (this.streamName === null || this.stream === null) {
      throw new Error("demo session has no loaded stream");
    }
    return await this.loadStreamObject(this.streamName, this.stream);
  }

  frameCount() {
    return this.runtime?.frames?.length ?? 0;
  }

  frameTime(index) {
    return Number(this.runtime?.frames?.[index]?.t ?? 0);
  }

  resize() {
    if (this.canvas === null) {
      return false;
    }
    return resizeCanvasToDisplaySize(this.canvas, this.device, this.context, this.canvasFormat);
  }

  async render(options = {}) {
    if (this.runtime === null || this.stream === null) {
      throw new Error("demo session has no loaded stream");
    }
    const resized = options.resize === false ? false : this.resize();
    if (resized && options.reloadOnResize !== false && this.config?.interactive === undefined) {
      await this.runtime.load(this.stream);
    }
    if (typeof options.beforeRender === "function") {
      options.beforeRender(this.runtime);
    }
    return await this.runtime.render({ frameIndex: options.frameIndex ?? null });
  }

  resourceStats() {
    if (this.runtime === null) {
      throw new Error("demo session has no loaded runtime");
    }
    return this.runtime.resourceStats();
  }
}



export async function executeDrp2Stream(device, context, canvasFormat, stream, options = {}) {
  const capabilities = effectiveCapabilities(
    options.capabilities ?? runtimeCapabilities(device, canvasFormat),
    stream.capabilities,
  );
  const state = options.state ?? createExecutionState(
    options.canvas ?? null, options.browserPresentFormat ?? canvasFormat);
  if (state.canvas === null && options.canvas !== undefined) {
    state.canvas = options.canvas;
  }
  const buffers = state.buffers;
  const textures = state.textures;
  const textureViews = state.textureViews;
  const samplers = state.samplers;
  const bindGroupLayouts = state.bindGroupLayouts;
  const bindGroups = state.bindGroups;
  const shaders = state.shaders;
  const pipelines = state.pipelines;
  const encoders = new Map();
  const encoderRefs = new Map();
  const passes = new Map();
  const commandBuffers = new Map();
  const pendingTightTextureCopies = [];
  const readbackReplies = [];
  const commands = options.commands ?? stream.commands;
  const enforceHandshake = options.commands === undefined;
  let sessionState = "initial";

  if (options.validateCapabilities !== false) {
    validateStreamCapabilities(commands, capabilities, canvasFormat, state);
  }

  try {
    for (const [commandIndex, command] of commands.entries()) {
      try {
        if (enforceHandshake) {
          if (command.cmd === "HelloRenderer") {
            if (sessionState !== "initial") {
              throw new Error("handshake already started");
            }
            if (command.version?.major !== 2) {
              throw new Error(`unsupported DRP2 version ${command.version?.major ?? "unknown"}`);
            }
            sessionState = "hello";
          } else if (command.cmd === "RendererHelloReply") {
            if (sessionState !== "hello") {
              throw new Error("handshake reply without pending hello");
            }
            sessionState = command.status === "ok" ? "ready" : "failed";
          } else if (command.cmd === "Error") {
            if (sessionState === "initial") {
              throw new Error("diagnostic Error before handshake");
            }
          } else if (sessionState === "failed") {
            throw new Error("handshake failed before active command");
          } else if (sessionState !== "ready") {
            throw new Error("handshake is not ready");
          }
        }
        switch (command.cmd) {
      case "HelloRenderer":
      case "RendererHelloReply":
        break;

      case "CreateBuffer":
        prepareCreateObject(state, command.id, "buffer", options);
        registerObject(
          state,
          buffers,
          command.id,
          device.createBuffer({
            label: command.label,
            size: required(command.size, "CreateBuffer needs size"),
            usage: mapBufferUsage(command.usage),
          }),
          "buffer",
          { size: required(command.size, "CreateBuffer needs size"), usage: new Set(command.usage ?? []) },
        );
        break;

      case "WriteBuffer": {
        const bufferRecord = requireLiveRecord(state, command.buffer_id, "buffer");
        requireUsage(bufferRecord, "COPY_DST");
        const buffer = required(
          buffers.get(command.buffer_id),
          `unknown buffer ${command.buffer_id}`,
        );
        const bytes = await decodePayload(command, "WriteBuffer");
        const size = command.size ?? bytes.byteLength;
        if (size !== bytes.byteLength) {
          throw new Error(`WriteBuffer size ${size} does not match payload size ${bytes.byteLength}`);
        }
        if ((command.offset ?? 0) + size > bufferRecord.size) {
          throw new Error("WriteBuffer range is out of range");
        }
        device.queue.writeBuffer(buffer, command.offset ?? 0, bytes, 0, bytes.byteLength);
        break;
      }

      case "CreateTexture": {
        const textureFormat = mapTextureFormat(required(command.format, "CreateTexture needs format"));
        validateBrowserCanvasTextureExtentAlias(command);
        const extent = commandExtent(command, state);
        validateTextureCapabilities(capabilities, command, textureFormat, extent);
        prepareCreateObject(state, command.id, "texture", options);
        registerObject(
          state,
          textures,
          command.id,
          device.createTexture({
            label: command.label,
            size: {
              width: extent.width,
              height: extent.height,
              depthOrArrayLayers: extent.depth,
            },
            mipLevelCount: command.mip_level_count ?? 1,
            sampleCount: command.sample_count ?? 1,
            dimension: command.dimension ?? "2d",
            format: textureFormat,
            usage: mapTextureUsage(command.usage),
          }),
          "texture",
          {
            format: textureFormat,
            width: extent.width,
            height: extent.height,
            depth: extent.depth,
            mipLevelCount: command.mip_level_count ?? 1,
            sampleCount: command.sample_count ?? 1,
            dimension: command.dimension ?? "2d",
            usage: new Set(command.usage ?? []),
          },
        );
        break;
      }

      case "CreateTextureView": {
        const textureRecord = requireLiveRecord(state, command.texture_id, "texture");
        const textureViewFormat = command.format === undefined
          ? textureRecord.format
          : mapTextureFormat(command.format);
        const texture = required(
          textures.get(command.texture_id),
          `unknown texture ${command.texture_id}`,
        );
        prepareCreateObject(state, command.id, "texture_view", options);
        const record = registerObject(
          state,
          textureViews,
          command.id,
          texture.createView(makeTextureViewDescriptor(command)),
          "texture_view",
          { format: textureViewFormat },
        );
        retainDependency(record, textureRecord);
        break;
      }

      case "WriteTexture": {
        const textureRecord = requireLiveRecord(state, command.texture_id, "texture");
        requireUsage(textureRecord, "COPY_DST");
        const texture = required(
          textures.get(command.texture_id),
          `unknown texture ${command.texture_id}`,
        );
        const bytes = await decodePayload(command, "WriteTexture");
        const size = required(command.size, "WriteTexture needs size");
        const origin = command.origin ?? { x: 0, y: 0, z: 0 };
        const mipLevel = command.mip_level ?? 0;
        const rowsPerImage = command.rows_per_image ?? size.height;
        const bytesPerRow = required(command.bytes_per_row, "WriteTexture needs bytes_per_row");
        validateTextureRange(textureRecord, mipLevel, origin, size);
        validateTextureLayout(textureRecord, size, bytesPerRow, rowsPerImage, bytes.byteLength);
        device.queue.writeTexture(
          {
            texture,
            mipLevel,
            origin: {
              x: origin.x ?? 0,
              y: origin.y ?? 0,
              z: origin.z ?? 0,
            },
          },
          bytes,
          {
            offset: 0,
            bytesPerRow,
            rowsPerImage,
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
        prepareCreateObject(state, command.id, "sampler", options);
        registerObject(
          state,
          samplers,
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
          "sampler",
        );
        break;

      case "CreateBindGroupLayout":
        prepareCreateObject(state, command.id, "bind_group_layout", options);
        registerObject(
          state,
          bindGroupLayouts,
          command.id,
          {
            id: command.id,
            entries: required(command.entries, "CreateBindGroupLayout needs entries"),
            layout: device.createBindGroupLayout({
              label: command.label,
              entries: command.entries.map((entry) =>
                makeBindGroupLayoutEntry(entry, "read_write", options)),
            }),
          },
          "bind_group_layout",
        );
        break;

      case "CreateBindGroup": {
        prepareCreateObject(state, command.id, "bind_group", options);
        const bindGroupLayoutRecord = requireLiveRecord(
          state,
          command.bind_group_layout_id,
          "bind_group_layout",
        );
        const bindGroupLayout = required(
          bindGroupLayouts.get(command.bind_group_layout_id),
          `unknown bind-group layout ${command.bind_group_layout_id}`,
        );
        const entries = required(command.entries, "CreateBindGroup needs entries");
        if (entries.length !== bindGroupLayout.entries.length) {
          throw new Error("bind-group entry count does not match layout");
        }
        const dependencies = [bindGroupLayoutRecord];
        for (const entry of entries) {
          const layoutEntry = bindGroupLayout.entries.find((item) => item.binding === entry.binding);
          if (layoutEntry === undefined) {
            throw new Error(`bind-group entry binding ${entry.binding} is not in layout`);
          }
          let resourceRecord = null;
          switch (entry.resource_kind) {
            case "buffer":
              resourceRecord = requireLiveRecord(state, entry.resource_id, "buffer");
              break;
            case "texture":
              resourceRecord = requireLiveRecord(state, entry.resource_id, "texture");
              break;
            case "texture_view":
              resourceRecord = requireLiveRecord(state, entry.resource_id, "texture_view");
              break;
            case "sampler":
              resourceRecord = requireLiveRecord(state, entry.resource_id, "sampler");
              break;
            default:
              throw new Error(`unsupported bind-group resource_kind: ${entry.resource_kind}`);
          }
          validateBindGroupEntry(layoutEntry, entry, resourceRecord);
          dependencies.push(resourceRecord);
        }
        const record = registerObject(
          state,
          bindGroups,
          command.id,
          {
            layoutId: command.bind_group_layout_id,
            entries,
            bindGroup: makeBindGroup(
              device,
              specializeBindGroupLayoutForResources(
                device,
                bindGroupLayout,
                { ...command, entries },
                state,
                options,
              ),
              command,
              buffers,
              textures,
              textureViews,
              samplers,
            ),
          },
          "bind_group",
        );
        for (const dependency of dependencies) {
          retainDependency(record, dependency);
        }
        break;
      }

      case "CreateShaderModule": {
        if (
          Array.isArray(capabilities.supported_shader_formats) &&
          !capabilities.supported_shader_formats.includes(command.format)
        ) {
          throw new Error(`shader format ${command.format} is not supported by capabilities`);
        }
        if (command.format !== "wgsl") {
          throw new Error(`unsupported shader format: ${command.format}`);
        }
        if (
          command.required_features?.includes("fp64") &&
          capabilities.supports_fp64 === false
        ) {
          throw new Error("shader module requires feature fp64");
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
        prepareCreateObject(state, command.id, "shader_module", options);
        registerObject(
          state,
          shaders,
          command.id,
          {
            module,
            entryPoint: command.entry_point ?? "main",
            stage: command.stage,
            code: command.code,
          },
          "shader_module",
        );
        break;
      }

      case "CreateRenderPipeline": {
        const dependencies = [
          requireLiveRecord(state, command.vertex_shader_module_id, "shader_module"),
          requireLiveRecord(state, command.fragment_shader_module_id, "shader_module"),
          ...(command.bind_group_layout_ids ?? []).map((id) =>
            requireLiveRecord(state, id, "bind_group_layout"),
          ),
        ];
        prepareCreateObject(state, command.id, "pipeline", options);
        const record = registerObject(
          state,
          pipelines,
          command.id,
          makePipeline(device, canvasFormat, shaders, bindGroupLayouts, command, options),
          "render_pipeline",
        );
        for (const dependency of dependencies) {
          retainDependency(record, dependency);
        }
        break;
      }

      case "CreateComputePipeline": {
        const dependencies = [
          requireLiveRecord(state, command.compute_shader_module_id, "shader_module"),
          ...(command.bind_group_layout_ids ?? []).map((id) =>
            requireLiveRecord(state, id, "bind_group_layout"),
          ),
        ];
        prepareCreateObject(state, command.id, "pipeline", options);
        const record = registerObject(
          state,
          pipelines,
          command.id,
          makeComputePipeline(device, shaders, bindGroupLayouts, command, options),
          "compute_pipeline",
        );
        for (const dependency of dependencies) {
          retainDependency(record, dependency);
        }
        break;
      }

      case "BeginCommandEncoder":
        encoders.set(command.id, device.createCommandEncoder({ label: command.label }));
        encoderRefs.set(command.id, new Set());
        break;

      case "BeginRenderPass": {
        await browserPresentStaleFrameIfNeeded(device, context, canvasFormat, state);
        const attachmentFormats = renderPassAttachmentFormats(state, canvasFormat, command);
        for (const attachment of required(
          command.color_attachments,
          "BeginRenderPass needs color_attachments",
        )) {
          if (attachment.texture_id !== 0) {
            addEncoderRef(state, encoderRefs, command.encoder_id, attachment.texture_id, "texture");
          }
          const resolveTextureId = attachment.resolve_target?.texture_id;
          if (resolveTextureId !== undefined && resolveTextureId !== 0) {
            addEncoderRef(state, encoderRefs, command.encoder_id, resolveTextureId, "texture");
          }
        }
        if (command.depth_stencil_attachment !== undefined) {
          if (command.depth_stencil_attachment.texture_id !== 0) {
            addEncoderRef(
              state,
              encoderRefs,
              command.encoder_id,
              command.depth_stencil_attachment.texture_id,
              "texture",
            );
          }
        }
        passes.set(command.id, {
          id: command.id,
          kind: "render",
          encoderId: command.encoder_id,
          colorAttachmentFormats: attachmentFormats.colorAttachmentFormats,
          depthStencilFormat: attachmentFormats.depthStencilFormat,
          sampleCount: attachmentFormats.sampleCount,
          vertexBuffers: new Set(),
          hasIndexBuffer: false,
          pass: beginRenderPass(device, state, canvasFormat, textures, encoders, command),
        });
        break;
      }

      case "BeginComputePass": {
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        passes.set(command.id, {
          id: command.id,
          kind: "compute",
          encoderId: command.encoder_id,
          pipeline: undefined,
          pass: encoder.beginComputePass({ label: command.label }),
        });
        break;
      }

      case "SetPipeline": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const pipelineKind = passRecord.kind === "render" ? "render_pipeline" : "compute_pipeline";
        requireLiveRecord(state, command.pipeline_id, pipelineKind);
        const pipelineRecord = required(
          pipelines.get(command.pipeline_id),
          `unknown pipeline ${command.pipeline_id}`,
        );
        if (passRecord.kind === "render") {
          validateRenderPipelineForPass(passRecord, pipelineRecord, command.pipeline_id);
        }
        addEncoderRef(state, encoderRefs, passRecord.encoderId, command.pipeline_id, pipelineKind);
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
        requireUsage(requireLiveRecord(state, command.buffer_id, "buffer"), "VERTEX");
        addEncoderRef(state, encoderRefs, passRecord.encoderId, command.buffer_id, "buffer");
        passRecord.pass.setVertexBuffer(command.slot ?? 0, buffer, command.offset ?? 0);
        passRecord.vertexBuffers.add(command.slot ?? 0);
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
        requireUsage(requireLiveRecord(state, command.buffer_id, "buffer"), "INDEX");
        addEncoderRef(state, encoderRefs, passRecord.encoderId, command.buffer_id, "buffer");
        passRecord.pass.setIndexBuffer(buffer, mapIndexFormat(command.index_format), command.offset ?? 0);
        passRecord.hasIndexBuffer = true;
        break;
      }

      case "SetBindGroup": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const pipelineRecord = requireBoundPipeline(passRecord);
        const bindGroupRecord = required(
          bindGroups.get(command.bind_group_id),
          `unknown bind group ${command.bind_group_id}`,
        );
        const slot = command.slot ?? 0;
        const expectedLayoutId = pipelineRecord.bindGroupLayoutIds?.[slot];
        if (expectedLayoutId !== undefined && expectedLayoutId !== bindGroupRecord.layoutId) {
          throw new Error(`bind group layout does not match pipeline slot ${slot}`);
        }
        const layoutRecord = required(
          bindGroupLayouts.get(bindGroupRecord.layoutId),
          `unknown bind-group layout ${bindGroupRecord.layoutId}`,
        );
        validateDynamicOffsets(state, layoutRecord, bindGroupRecord, command.dynamic_offsets ?? []);
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
        addEncoderRef(state, encoderRefs, passRecord.encoderId, command.bind_group_id, "bind_group");
        passRecord.pass.setBindGroup(slot, bindGroup, []);
        break;
      }

      case "SetViewport": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("SetViewport requires a render pass");
        }
        passRecord.pass.setViewport(
          command.x,
          command.y,
          command.width,
          command.height,
          command.min_depth,
          command.max_depth,
        );
        break;
      }

      case "SetScissor": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("SetScissor requires a render pass");
        }
        passRecord.pass.setScissorRect(command.x, command.y, command.width, command.height);
        break;
      }

      case "SetBlendConstant": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("SetBlendConstant requires a render pass");
        }
        passRecord.pass.setBlendConstant(required(command.color, "SetBlendConstant needs color"));
        break;
      }

      case "SetStencilReference": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("SetStencilReference requires a render pass");
        }
        passRecord.pass.setStencilReference(command.reference);
        break;
      }

      case "Draw": {
        const passRecord = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        if (passRecord.kind !== "render") {
          throw new Error("Draw requires a render pass");
        }
        validateRenderDrawState(passRecord);
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
        validateIndexedDrawState(passRecord);
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
        requireBoundPipeline(passRecord);
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

      case "ResourceBarrier": {
        requireNoOpenPass(passes, command.encoder_id);
        addEncoderRef(state, encoderRefs, command.encoder_id, command.buffer_id, "buffer");
        const bufferRecord = requireLiveRecord(state, command.buffer_id, "buffer");
        requireUsage(bufferRecord, "STORAGE");
        if (command.src_stage !== "COMPUTE" || command.src_access !== "STORAGE_WRITE") {
          throw new Error("unsupported ResourceBarrier source");
        }
        if (command.dst_stage === "VERTEX_INPUT" && command.dst_access === "VERTEX_READ") {
          requireUsage(bufferRecord, "VERTEX");
        } else if (command.dst_stage === "COPY" && command.dst_access === "COPY_READ") {
          requireUsage(bufferRecord, "COPY_SRC");
        } else {
          throw new Error("unsupported ResourceBarrier destination");
        }
        const offset = command.offset ?? 0;
        const size = command.size ?? 0;
        if (size > 0 && offset + size > bufferRecord.size) {
          throw new Error("ResourceBarrier range is out of range");
        }
        if (size === 0 && offset > bufferRecord.size) {
          throw new Error("ResourceBarrier range is out of range");
        }
        break;
      }

      case "CopyTextureToBuffer": {
        requireNoOpenPass(passes, command.encoder_id);
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        const textureRecord = addEncoderRef(
          state,
          encoderRefs,
          command.encoder_id,
          command.src_texture_id,
          "texture",
        );
        const bufferRecord = addEncoderRef(
          state,
          encoderRefs,
          command.encoder_id,
          command.dst_buffer_id,
          "buffer",
        );
        requireUsage(textureRecord, "COPY_SRC");
        requireUsage(bufferRecord, "COPY_DST");
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
        const mipLevel = command.src_mip_level ?? 0;
        validateTextureRange(textureRecord, mipLevel, origin, size);
        validateTextureLayout(textureRecord, size, bytesPerRow, rowsPerImage);
        const layoutBytes = (size.depth ?? 1) > 1
          ? ((size.depth ?? 1) - 1) * rowsPerImage * bytesPerRow +
              (size.height - 1) * bytesPerRow +
              size.width * textureFormatBytes(textureRecord.format)
          : (size.height - 1) * bytesPerRow + size.width * textureFormatBytes(textureRecord.format);
        if ((command.dst_offset ?? 0) + layoutBytes > bufferRecord.size) {
          throw new Error("CopyTextureToBuffer destination range is out of range");
        }

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
            mipLevel,
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

      case "CopyBufferToBuffer": {
        requireNoOpenPass(passes, command.encoder_id);
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        const srcRecord = addEncoderRef(
          state,
          encoderRefs,
          command.encoder_id,
          command.src_buffer_id,
          "buffer",
        );
        const dstRecord = addEncoderRef(
          state,
          encoderRefs,
          command.encoder_id,
          command.dst_buffer_id,
          "buffer",
        );
        requireUsage(srcRecord, "COPY_SRC");
        requireUsage(dstRecord, "COPY_DST");
        if ((command.src_offset ?? 0) + command.size > srcRecord.size) {
          throw new Error("CopyBufferToBuffer source range is out of range");
        }
        if ((command.dst_offset ?? 0) + command.size > dstRecord.size) {
          throw new Error("CopyBufferToBuffer destination range is out of range");
        }
        const srcBuffer = required(
          buffers.get(command.src_buffer_id),
          `unknown source buffer ${command.src_buffer_id}`,
        );
        const dstBuffer = required(
          buffers.get(command.dst_buffer_id),
          `unknown destination buffer ${command.dst_buffer_id}`,
        );
        encoder.copyBufferToBuffer(
          srcBuffer,
          command.src_offset ?? 0,
          dstBuffer,
          command.dst_offset ?? 0,
          command.size,
        );
        break;
      }

      case "CopyBufferToTexture": {
        requireNoOpenPass(passes, command.encoder_id);
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        const bufferRecord = addEncoderRef(
          state,
          encoderRefs,
          command.encoder_id,
          command.src_buffer_id,
          "buffer",
        );
        const textureRecord = addEncoderRef(
          state,
          encoderRefs,
          command.encoder_id,
          command.dst_texture_id,
          "texture",
        );
        requireUsage(bufferRecord, "COPY_SRC");
        requireUsage(textureRecord, "COPY_DST");
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
        const bytesPerRow = required(command.bytes_per_row, "CopyBufferToTexture needs bytes_per_row");
        const rowsPerImage = command.rows_per_image ?? size.height;
        const mipLevel = command.dst_mip_level ?? 0;
        validateTextureRange(textureRecord, mipLevel, origin, size);
        validateTextureLayout(textureRecord, size, bytesPerRow, rowsPerImage);
        encoder.copyBufferToTexture(
          {
            buffer,
            offset: command.src_offset ?? 0,
            bytesPerRow,
            rowsPerImage,
          },
          {
            texture,
            mipLevel,
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
        requireNoOpenPass(passes, command.encoder_id);
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        const srcRecord = addEncoderRef(
          state,
          encoderRefs,
          command.encoder_id,
          command.src_texture_id,
          "texture",
        );
        const dstRecord = addEncoderRef(
          state,
          encoderRefs,
          command.encoder_id,
          command.dst_texture_id,
          "texture",
        );
        requireUsage(srcRecord, "COPY_SRC");
        requireUsage(dstRecord, "COPY_DST");
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
        const srcMipLevel = command.src_mip_level ?? 0;
        const dstMipLevel = command.dst_mip_level ?? 0;
        validateTextureRange(srcRecord, srcMipLevel, srcOrigin, size);
        validateTextureRange(dstRecord, dstMipLevel, dstOrigin, size);
        encoder.copyTextureToTexture(
          {
            texture: srcTexture,
            mipLevel: srcMipLevel,
            origin: {
              x: srcOrigin.x ?? 0,
              y: srcOrigin.y ?? 0,
              z: srcOrigin.z ?? 0,
            },
          },
          {
            texture: dstTexture,
            mipLevel: dstMipLevel,
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
        requireNoOpenPass(passes, command.encoder_id);
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        finishEncoderRefs(
          encoderRefs,
          commandBuffers,
          command.encoder_id,
          command.command_buffer_id,
          encoder.finish(),
        );
        encoders.delete(command.encoder_id);
        break;
      }

      case "QueueSubmit": {
        const ids = command.command_buffer_ids ?? [command.command_buffer_id];
        if (new Set(ids).size !== ids.length) {
          throw new Error("duplicate command buffer id in QueueSubmit");
        }
        const submittedRecords = ids.map((id) =>
          required(commandBuffers.get(id), `unknown command buffer ${id}`),
        );
        const submitBuffers = submittedRecords.map((record) => submitCommandBuffer(record));
        device.queue.submit(submitBuffers);
        const readbacks = command.readbacks ?? [];
        const retireSubmitted = options.retireSubmittedRefs === true;
        if ((readbacks.length > 0 || retireSubmitted) && typeof device.queue.onSubmittedWorkDone === "function") {
          await device.queue.onSubmittedWorkDone();
        }
        if (retireSubmitted) {
          for (const record of submittedRecords) {
            retireSubmittedRefs(record);
          }
        }
        await browserPresentPendingFrame(device, context, canvasFormat, state);
        for (const readback of readbacks) {
          const bufferRecord = requireLiveRecord(state, readback.buffer_id, "buffer");
          requireUsage(bufferRecord, "MAP_READ");
          const buffer = required(
            buffers.get(readback.buffer_id),
            `unknown readback buffer ${readback.buffer_id}`,
          );
          const offset = readback.offset ?? 0;
          const size = required(readback.size, "readback needs size");
          if (offset + size > bufferRecord.size) {
            throw new Error("readback range is out of range");
          }
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
        break;

      case "DestroyBuffer":
        destroyObject(buffers, requireLiveRecord(state, command.buffer_id, "buffer"));
        break;

      case "DestroyTexture":
        destroyObject(textures, requireLiveRecord(state, command.texture_id, "texture"));
        break;

      case "DestroyTextureView":
        destroyObject(textureViews, requireLiveRecord(state, command.id, "texture_view"));
        break;

      case "DestroySampler":
        destroyObject(samplers, requireLiveRecord(state, command.id, "sampler"));
        break;

      case "DestroyBindGroupLayout":
        destroyObject(
          bindGroupLayouts,
          requireLiveRecord(state, command.bind_group_layout_id, "bind_group_layout"),
        );
        break;

      case "DestroyBindGroup":
        destroyObject(bindGroups, requireLiveRecord(state, command.bind_group_id, "bind_group"));
        break;

      case "DestroyShaderModule":
        destroyObject(
          shaders,
          requireLiveRecord(state, command.shader_module_id, "shader_module"),
        );
        break;

      case "DestroyRenderPipeline":
        destroyObject(
          pipelines,
          requireLiveRecord(state, command.render_pipeline_id, "render_pipeline"),
        );
        break;

      case "DestroyComputePipeline":
        destroyObject(
          pipelines,
          requireLiveRecord(state, command.compute_pipeline_id, "compute_pipeline"),
        );
        break;

      default:
        throw new Error(`unsupported DRP2 command in WebGPU PoC: ${command.cmd}`);
      }
    } catch (error) {
      throw wrapDrp2WebGpuError(commandIndex, command, error);
    }
  }
  } catch (error) {
    discardLocalExecutionRefs(encoderRefs, commandBuffers);
    throw error;
  }
  discardLocalExecutionRefs(encoderRefs, commandBuffers);

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
