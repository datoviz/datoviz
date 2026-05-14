const statusEl = document.querySelector("#status");
const canvas = document.querySelector("#viewport");
const streamNameEl = document.querySelector("#stream-name");
const streamSelectEl = document.querySelector("#stream-select");

export const STREAMS = [
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
  if (topology === undefined || topology === "triangle-list") {
    return "triangle-list";
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
        flags |= GPUBufferUsage.MAP_WRITE;
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
    case "rgba8unorm":
    case "bgra8unorm":
    case "depth32float":
      return format;
    default:
      throw new Error(`unsupported texture format: ${format}`);
  }
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



function makeVertexBuffers(command) {
  const vertexBuffers = command.vertex_buffers ?? [];
  if (vertexBuffers.length === 0) {
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



function makeBindGroupLayoutEntry(entry) {
  const binding = required(entry.binding, "bind-group layout entry needs binding");
  switch (entry.binding_type) {
    case "sampled_texture":
      return {
        binding,
        visibility: GPUShaderStage.FRAGMENT,
        texture: { sampleType: "float" },
      };
    case "sampler":
      return {
        binding,
        visibility: GPUShaderStage.FRAGMENT,
        sampler: { type: "filtering" },
      };
    default:
      throw new Error(`unsupported bind-group layout binding_type: ${entry.binding_type}`);
  }
}



function makeBindGroupEntry(entry, textures, samplers) {
  const binding = required(entry.binding, "bind-group entry needs binding");
  switch (entry.resource_kind) {
    case "texture":
      return {
        binding,
        resource: required(textures.get(entry.resource_id), `unknown texture ${entry.resource_id}`)
          .createView(),
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



function makePipeline(device, canvasFormat, shaders, bindGroupLayouts, command) {
  const vertexShader = required(
    shaders.get(command.vertex_shader_module_id),
    `unknown vertex shader module ${command.vertex_shader_module_id}`,
  );
  const fragmentShader = required(
    shaders.get(command.fragment_shader_module_id),
    `unknown fragment shader module ${command.fragment_shader_module_id}`,
  );

  const colorTargets = required(command.color_targets, "CreateRenderPipeline needs color_targets");
  if (colorTargets.length !== 1) {
    throw new Error("only one color target is supported by this PoC");
  }

  const streamFormat = colorTargets[0].format;
  const targetFormat = streamFormat === "canvas" ? canvasFormat : mapTextureFormat(streamFormat);
  const bindGroupLayoutIds = command.bind_group_layout_ids ?? [];
  const layout = bindGroupLayoutIds.length > 0
    ? device.createPipelineLayout({
        bindGroupLayouts: bindGroupLayoutIds.map((id) =>
          required(bindGroupLayouts.get(id), `unknown bind-group layout ${id}`),
        ),
      })
    : "auto";

  return device.createRenderPipeline({
    label: command.label,
    layout,
    vertex: {
      module: vertexShader.module,
      entryPoint: vertexShader.entryPoint,
      buffers: makeVertexBuffers(command),
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
  });
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



export async function executeDrp2Stream(device, context, canvasFormat, stream) {
  const buffers = new Map();
  const textures = new Map();
  const samplers = new Map();
  const bindGroupLayouts = new Map();
  const bindGroups = new Map();
  const shaders = new Map();
  const pipelines = new Map();
  const encoders = new Map();
  const passes = new Map();
  const commandBuffers = new Map();
  const readbackReplies = [];

  for (const command of stream.commands) {
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
        const bytes = decodeBase64(required(command.data, "WriteBuffer needs data"));
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

      case "WriteTexture": {
        const texture = required(
          textures.get(command.texture_id),
          `unknown texture ${command.texture_id}`,
        );
        const bytes = decodeBase64(required(command.data, "WriteTexture needs data"));
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
          device.createBindGroupLayout({
            label: command.label,
            entries: required(command.entries, "CreateBindGroupLayout needs entries").map((entry) =>
              makeBindGroupLayoutEntry(entry),
            ),
          }),
        );
        break;

      case "CreateBindGroup":
        bindGroups.set(
          command.id,
          device.createBindGroup({
            label: command.label,
            layout: required(
              bindGroupLayouts.get(command.bind_group_layout_id),
              `unknown bind-group layout ${command.bind_group_layout_id}`,
            ),
            entries: required(command.entries, "CreateBindGroup needs entries").map((entry) =>
              makeBindGroupEntry(entry, textures, samplers),
            ),
          }),
        );
        break;

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
        });
        break;
      }

      case "CreateRenderPipeline":
        pipelines.set(
          command.id,
          makePipeline(device, canvasFormat, shaders, bindGroupLayouts, command),
        );
        break;

      case "BeginCommandEncoder":
        encoders.set(command.id, device.createCommandEncoder({ label: command.label }));
        break;

      case "BeginRenderPass":
        passes.set(command.id, beginRenderPass(context, textures, encoders, command));
        break;

      case "SetPipeline": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const pipeline = required(
          pipelines.get(command.pipeline_id),
          `unknown pipeline ${command.pipeline_id}`,
        );
        pass.setPipeline(pipeline);
        break;
      }

      case "SetVertexBuffer": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const buffer = required(
          buffers.get(command.buffer_id),
          `unknown buffer ${command.buffer_id}`,
        );
        pass.setVertexBuffer(command.slot ?? 0, buffer, command.offset ?? 0);
        break;
      }

      case "SetIndexBuffer": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const buffer = required(
          buffers.get(command.buffer_id),
          `unknown buffer ${command.buffer_id}`,
        );
        pass.setIndexBuffer(buffer, mapIndexFormat(command.index_format), command.offset ?? 0);
        break;
      }

      case "SetBindGroup": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const bindGroup = required(
          bindGroups.get(command.bind_group_id),
          `unknown bind group ${command.bind_group_id}`,
        );
        pass.setBindGroup(command.slot ?? 0, bindGroup, command.dynamic_offsets ?? []);
        break;
      }

      case "Draw": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        pass.draw(
          command.vertex_count,
          command.instance_count ?? 1,
          command.first_vertex ?? 0,
          command.first_instance ?? 0,
        );
        break;
      }

      case "DrawIndexed": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        pass.drawIndexed(
          command.index_count,
          command.instance_count ?? 1,
          command.first_index ?? 0,
          command.base_vertex ?? 0,
          command.first_instance ?? 0,
        );
        break;
      }

      case "EndRenderPass": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        pass.end();
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
            buffer,
            offset: command.dst_offset ?? 0,
            bytesPerRow: required(command.bytes_per_row, "CopyTextureToBuffer needs bytes_per_row"),
            rowsPerImage: command.rows_per_image ?? size.height,
          },
          {
            width: required(size.width, "CopyTextureToBuffer size needs width"),
            height: required(size.height, "CopyTextureToBuffer size needs height"),
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
        break;

      default:
        throw new Error(`unsupported DRP2 command in WebGPU PoC: ${command.cmd}`);
    }
  }

  return { readbacks: readbackReplies };
}



export async function executeDrp2StreamChecked(device, context, canvasFormat, stream) {
  const scopes = ["validation", "out-of-memory", "internal"];
  for (const scope of scopes) {
    device.pushErrorScope(scope);
  }

  let result = null;
  let thrown = null;
  try {
    result = await executeDrp2Stream(device, context, canvasFormat, stream);
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



async function main() {
  try {
    const { device, context, format } = await initWebGPU();
    const params = new URLSearchParams(window.location.search);
    let streamName = params.get("stream") ?? "indexed_quad_wgsl";
    let stream = null;

    for (const item of STREAMS) {
      const option = document.createElement("option");
      option.value = item.name;
      option.textContent = item.label;
      streamSelectEl.appendChild(option);
    }
    streamSelectEl.value = streamName;

    const loadStream = async (name) => {
      const streamPath = `./streams/${name}.json`;
      streamNameEl.textContent = streamPath.slice(2);
      const response = await fetch(streamPath, { cache: "no-cache" });
      if (!response.ok) {
        throw new Error(`failed to load stream: ${response.status} ${response.statusText}`);
      }
      streamName = name;
      stream = await response.json();
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
          resizeCanvasToDisplaySize(device, context, format);
          if (stream === null) {
            await loadStream(streamName);
          }
          const result = await executeDrp2StreamChecked(device, context, format, stream);
          if (result.readbacks.length > 0) {
            const readback = result.readbacks[0];
            setStatus(
              `Rendered ${stream.name}; readback nonzero=${readback.summary.nonzero}`,
            );
          } else {
            setStatus(`Rendered ${stream.name}; readbacks=0`);
          }
        } while (rerenderRequested);
      } finally {
        rendering = false;
      }
    };

    streamSelectEl.addEventListener("change", () => {
      loadStream(streamSelectEl.value)
        .then(render)
        .catch((error) => setStatus(error.message, true));
    });

    await loadStream(streamName);
    await render();
    new ResizeObserver(() => {
      render().catch((error) => setStatus(error.message, true));
    }).observe(canvas);
  } catch (error) {
    setStatus(error.message, true);
    console.error(error);
  }
}



if (canvas !== null && statusEl !== null && streamNameEl !== null && streamSelectEl !== null) {
  main();
}
