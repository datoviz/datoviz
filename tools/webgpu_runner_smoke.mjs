#!/usr/bin/env node

import { readFile } from 'node:fs/promises';
import { decodeDrp2Packet, decodeDrp2PacketSet } from '../web/drp2/packet.js';

const COLOR_GREEN = '\x1b[32m';
const COLOR_RESET = '\x1b[0m';

function useColor() {
  if (process.env.FORCE_COLOR !== undefined && process.env.FORCE_COLOR !== '0') return true;
  if (process.env.NO_COLOR !== undefined) return false;
  return Boolean(process.stdout.isTTY);
}

function color(text, colorCode) {
  return useColor() ? `${colorCode}${text}${COLOR_RESET}` : text;
}

function passLine(text) {
  return `${color('PASS', COLOR_GREEN)} ${text}`;
}

const fakeCanvas = {
  width: 640,
  height: 480,
  clientWidth: 640,
  clientHeight: 480,
  style: {
    setProperty() {},
    removeProperty() {},
  },
};

globalThis.document = {
  querySelector(selector) {
    return selector === '#viewport' ? fakeCanvas : null;
  },
  createElement() {
    return {};
  },
};
globalThis.window = {
  location: { search: '', href: 'http://localhost/' },
  history: { replaceState() {} },
};

globalThis.GPUBufferUsage = {
  COPY_SRC: 1,
  COPY_DST: 2,
  VERTEX: 4,
  INDEX: 8,
  UNIFORM: 16,
  STORAGE: 32,
  MAP_READ: 64,
  MAP_WRITE: 128,
};
globalThis.GPUTextureUsage = {
  COPY_SRC: 1,
  COPY_DST: 2,
  TEXTURE_BINDING: 4,
  STORAGE_BINDING: 8,
  RENDER_ATTACHMENT: 16,
};
globalThis.GPUShaderStage = { VERTEX: 1, FRAGMENT: 2, COMPUTE: 4 };
globalThis.GPUColorWrite = { RED: 1, GREEN: 2, BLUE: 4, ALPHA: 8, ALL: 15 };
globalThis.GPUMapMode = { READ: 1 };
globalThis.atob = (value) => Buffer.from(value, 'base64').toString('binary');
globalThis.btoa = (value) => Buffer.from(value, 'binary').toString('base64');

let createdBufferCount = 0;
let queueSubmitCount = 0;
let observedViewports = [];
let observedScissors = [];
let observedShaderModules = [];
let observedTextureDescriptors = [];
let observedRenderPipelineDescriptors = [];
let observedRenderPassDescriptors = [];

const pass = () => ({
  setPipeline() {},
  setVertexBuffer() {},
  setIndexBuffer() {},
  setBindGroup() {},
  setViewport(x, y, width, height, minDepth, maxDepth) {
    observedViewports.push({ x, y, width, height, minDepth, maxDepth });
  },
  setScissorRect(x, y, width, height) {
    observedScissors.push({ x, y, width, height });
  },
  setBlendConstant() {},
  setStencilReference() {},
  draw() {},
  drawIndexed() {},
  dispatchWorkgroups() {},
  end() {},
});

const device = {
  limits: {
    maxTextureDimension2D: 8192,
  },
  queue: {
    writeBuffer() {},
    writeTexture() {},
    submit() {
      queueSubmitCount++;
    },
    async onSubmittedWorkDone() {},
  },
  createBuffer(desc = {}) {
    createdBufferCount++;
    return {
      size: desc.size ?? 0,
      destroy() {},
      async mapAsync() {},
      getMappedRange(_offset = 0, size = desc.size ?? 0) {
        return new ArrayBuffer(size);
      },
      unmap() {},
    };
  },
  createTexture(desc = {}) {
    observedTextureDescriptors.push(desc);
    return {
      createView() {
        return {};
      },
      destroy() {},
    };
  },
  createSampler() {
    return {};
  },
  createBindGroupLayout() {
    return {};
  },
  createBindGroup() {
    return {};
  },
  createShaderModule(desc = {}) {
    observedShaderModules.push(desc);
    return {
      async getCompilationInfo() {
        return { messages: [] };
      },
    };
  },
  createPipelineLayout() {
    return {};
  },
  createRenderPipeline(desc = {}) {
    observedRenderPipelineDescriptors.push(desc);
    return {};
  },
  createComputePipeline() {
    return {};
  },
  pushErrorScope() {},
  async popErrorScope() {
    return null;
  },
  createCommandEncoder() {
    return {
      beginRenderPass(desc = {}) {
        observedRenderPassDescriptors.push(desc);
        return pass();
      },
      beginComputePass() {
        return pass();
      },
      copyBufferToBuffer() {},
      copyBufferToTexture() {},
      copyTextureToBuffer() {},
      copyTextureToTexture() {},
      finish() {
        return {};
      },
    };
  },
};

const context = {
  configure() {},
  getCurrentTexture() {
    return {
      createView() {
        return {};
      },
    };
  },
};

const header = [
  { cmd: 'HelloRenderer', version: { major: 2, minor: 0 }, client_name: 'smoke' },
  {
    cmd: 'RendererHelloReply',
    version: { major: 2, minor: 0 },
    status: 'ok',
    renderer_name: 'fake',
  },
];

const exampleStreams = [
  'examples/webgpu/streams/attachment_multi_color_wgsl.json',
  'examples/webgpu/streams/attachment_depth_wgsl.json',
];

const negativeFixtureParity = [
  'spec/drp2/fixtures/negative/invalid_bind_group_sampler_unknown_id.json',
  'spec/drp2/fixtures/negative/invalid_capability_shader_module_format.json',
  'spec/drp2/fixtures/negative/invalid_create_sampler_duplicate.json',
  'spec/drp2/fixtures/negative/invalid_duplicate_buffer_id.json',
  'spec/drp2/fixtures/negative/invalid_end_wrong_pass_kind.json',
  'spec/drp2/fixtures/negative/invalid_pipeline_vertex_buffers_slot_mismatch.json',
  'spec/drp2/fixtures/negative/invalid_queue_submit_readback_unknown_buffer.json',
  'spec/drp2/fixtures/negative/invalid_queue_submit_reused_command_buffer.json',
  'spec/drp2/fixtures/negative/invalid_queue_submit_unknown_command_buffer.json',
  'spec/drp2/fixtures/negative/invalid_set_blend_constant_in_compute_pass.json',
  'spec/drp2/fixtures/negative/invalid_set_scissor_in_compute_pass.json',
  'spec/drp2/fixtures/negative/invalid_set_stencil_reference_in_compute_pass.json',
  'spec/drp2/fixtures/negative/invalid_set_viewport_in_compute_pass.json',
  'spec/drp2/fixtures/negative/invalid_texture_view_unknown_texture.json',
  'spec/drp2/fixtures/negative/invalid_unknown_buffer_id_write.json',
  'spec/drp2/fixtures/negative/invalid_wrong_object_type_destroy.json',
];

const triangleShaders = [
  {
    cmd: 'CreateShaderModule',
    id: 9000,
    stage: 'VERTEX',
    format: 'wgsl',
    entry_point: 'main',
    code: '@vertex fn main(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4f { var pos = array<vec2f, 3>(vec2f(-0.8, -0.8), vec2f(0.8, -0.8), vec2f(0.0, 0.8)); return vec4f(pos[idx], 0.0, 1.0); }',
  },
  {
    cmd: 'CreateShaderModule',
    id: 9001,
    stage: 'FRAGMENT',
    format: 'wgsl',
    entry_point: 'main',
    code: '@fragment fn main() -> @location(0) vec4f { return vec4f(1.0, 0.0, 0.0, 1.0); }',
  },
];

function texture(id, format) {
  return {
    cmd: 'CreateTexture',
    id,
    dimension: '2d',
    width: 4,
    height: 4,
    depth: 1,
    format,
    usage: ['RENDER_ATTACHMENT'],
    mip_level_count: 1,
    sample_count: 1,
  };
}

function renderPipeline(colorTargets, depthStencil = undefined) {
  const command = {
    cmd: 'CreateRenderPipeline',
    id: 10,
    vertex_buffer_slots: 0,
    vertex_buffers: [],
    vertex_shader_module_id: 9000,
    fragment_shader_module_id: 9001,
    topology: 'triangle-list',
    color_targets: colorTargets,
  };
  if (depthStencil !== undefined) {
    command.depth_stencil = depthStencil;
  }
  return command;
}

function renderPass(colorAttachments, depthStencilAttachment = undefined) {
  const command = {
    cmd: 'BeginRenderPass',
    id: 21,
    encoder_id: 20,
    color_attachments: colorAttachments,
  };
  if (depthStencilAttachment !== undefined) {
    command.depth_stencil_attachment = depthStencilAttachment;
  }
  return command;
}

function colorAttachment(textureId, loadOp = 'clear') {
  return {
    texture_id: textureId,
    load_op: loadOp,
    store_op: 'store',
    clear_value: { r: 0, g: 0, b: 0, a: 1 },
  };
}

async function loadJson(path) {
  return JSON.parse(await readFile(path, 'utf8'));
}

function emptyPacket(kind, resourceVersion, frameIndex) {
  const packet = new Uint8Array(56);
  packet.set([0x44, 0x56, 0x50, 0x32, 0x50, 0x4b, 0x54, 0]);
  const view = new DataView(packet.buffer);
  view.setUint16(8, 56, true);
  view.setUint16(10, 2, true);
  view.setUint16(14, kind, true);
  view.setBigUint64(40, BigInt(resourceVersion), true);
  view.setBigUint64(48, BigInt(frameIndex), true);
  return packet;
}

function packetWithRecords(kind, resourceVersion, frameIndex, records, arenaSize = 0n) {
  const commandBytes = records.reduce((sum, record) => sum + 32 + ((record.body.length + 7) & ~7), 0);
  const packet = new Uint8Array(56 + commandBytes);
  packet.set(emptyPacket(kind, resourceVersion, frameIndex));
  const view = new DataView(packet.buffer);
  view.setUint32(20, records.length, true);
  view.setBigUint64(24, BigInt(commandBytes), true);
  view.setBigUint64(32, arenaSize, true);
  let offset = 56;
  for (const record of records) {
    const body = record.body;
    const bodyPadded = (body.length + 7) & ~7;
    view.setUint32(offset + 0, record.type, true);
    view.setUint32(offset + 4, 0, true);
    view.setUint32(offset + 8, body.length, true);
    view.setBigUint64(offset + 16, record.payloadOffset ?? 0xffffffffffffffffn, true);
    view.setBigUint64(offset + 24, record.payloadSize ?? 0n, true);
    packet.set(body, offset + 32);
    offset += 32 + bodyPadded;
  }
  return packet;
}

function createBufferPacket(resourceVersion, frameIndex, id = 7n, size = 8n) {
  const body = new Uint8Array(24);
  const view = new DataView(body.buffer);
  view.setBigUint64(0, id, true);
  view.setBigUint64(8, size, true);
  view.setUint32(16, GPUBufferUsage.COPY_DST | GPUBufferUsage.COPY_SRC, true);
  return packetWithRecords(1, resourceVersion, frameIndex, [{ type: 3, body }]);
}

function writeBufferPacket({ payloadOffset = 0n, payloadSize = 4n, arenaSize = 8n } = {}) {
  const packet = new Uint8Array(56 + 32 + 24);
  packet.set(emptyPacket(2, 1, 1));
  const view = new DataView(packet.buffer);
  view.setUint32(20, 1, true);
  view.setBigUint64(24, 56n, true);
  view.setBigUint64(32, arenaSize, true);
  view.setUint32(56, 18, true);
  view.setUint32(64, 24, true);
  view.setBigUint64(72, payloadOffset, true);
  view.setBigUint64(80, payloadSize, true);
  view.setBigUint64(88, 7n, true);
  view.setBigUint64(96, 0n, true);
  view.setBigUint64(104, payloadSize, true);
  return packet;
}

function writeBufferPacketForVersion(resourceVersion, frameIndex, id = 7n, payloadSize = 4n) {
  const body = new Uint8Array(24);
  const view = new DataView(body.buffer);
  view.setBigUint64(0, id, true);
  view.setBigUint64(8, 0n, true);
  view.setBigUint64(16, payloadSize, true);
  return packetWithRecords(
    2,
    resourceVersion,
    frameIndex,
    [{ type: 18, body, payloadOffset: 0n, payloadSize }],
    8n,
  );
}

function bufferPacketSet(resourceVersion, frameIndex, id = 7n, includeSetup = false) {
  const packetSet = {
    update: {
      packet: writeBufferPacketForVersion(resourceVersion, frameIndex, id),
      arena: new Uint8Array([1, 2, 3, 4, 0, 0, 0, 0]),
    },
    frame: { packet: emptyPacket(3, resourceVersion, frameIndex) },
  };
  if (includeSetup) {
    packetSet.setup = { packet: createBufferPacket(resourceVersion, frameIndex, id) };
  }
  return packetSet;
}

function expectThrows(fn, expectedText) {
  try {
    fn();
  } catch (error) {
    if (!String(error.message).includes(expectedText)) {
      throw new Error(`expected "${expectedText}" failure, got "${error.message}"`);
    }
    return;
  }
  throw new Error(`expected "${expectedText}" failure`);
}

async function expectAsyncFailure(fn, expectedText) {
  try {
    await fn();
  } catch (error) {
    if (!String(error.message).includes(expectedText)) {
      throw new Error(`expected "${expectedText}" failure, got "${error.message}"`);
    }
    return;
  }
  throw new Error(`expected "${expectedText}" failure`);
}

async function expectFailure(executeDrp2Stream, stream, expectedText, expected = {}) {
  try {
    await executeDrp2Stream(device, context, 'bgra8unorm', stream, { canvas: fakeCanvas });
  } catch (error) {
    const message = String(error.message);
    if (!message.includes(expectedText)) {
      throw new Error(`expected "${expectedText}" failure, got "${message}"`);
    }
    for (const [key, value] of Object.entries(expected)) {
      if (error[key] !== value) {
        throw new Error(`expected error.${key}=${value}, got ${error[key]}`);
      }
    }
    return;
  }
  throw new Error(`expected "${expectedText}" failure`);
}

async function expectCapabilityPreflightFailure(executeDrp2Stream, stream, expectedText, expected) {
  const before = createdBufferCount;
  await expectFailure(executeDrp2Stream, stream, expectedText, expected);
  if (createdBufferCount !== before) {
    throw new Error(
      `capability preflight created GPU buffers before failing: ` +
        `${before} -> ${createdBufferCount}`,
    );
  }
}

async function expectNegativeFixtureParity(executeDrp2Stream, path) {
  const stream = await loadJson(path);
  const expected = stream.expected;
  if (expected?.outcome !== 'error') {
    throw new Error(`${path}: negative fixture needs expected.outcome=error`);
  }

  try {
    await executeDrp2Stream(device, context, 'bgra8unorm', stream, { canvas: fakeCanvas });
  } catch (error) {
    const expectedCmd = stream.commands[expected.command_index]?.cmd;
    const checks = {
      commandIndex: expected.command_index,
      cmd: expectedCmd,
      code: expected.code,
    };
    for (const [key, value] of Object.entries(checks)) {
      if (error[key] !== value) {
        throw new Error(`${path}: expected error.${key}=${value}, got ${error[key]}`);
      }
    }
    return;
  }

  throw new Error(`${path}: expected WebGPU runner failure`);
}

function comparableResourceStats(stats) {
  const { refs: _refs, ...stable } = stats;
  return stable;
}

function assertResourceStatsStable(actual, expected, label) {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) {
    throw new Error(`${label}: expected ${expectedText}, got ${actualText}`);
  }
}

const RUNTIME_STRESS_STREAMS = [
  'examples/webgpu/streams/scene_point_wgsl.json',
  'examples/webgpu/streams/scene_primitive_wgsl.json',
  'examples/webgpu/streams/texture_sampling_wgsl.json',
  'examples/webgpu/streams/attachment_depth_wgsl.json',
];

async function smokeRepeatedRuntimeFrames(Drp2WebGpuRuntime) {
  for (const path of RUNTIME_STRESS_STREAMS) {
    await smokeRepeatedRuntimeStream(Drp2WebGpuRuntime, path);
  }
}

async function smokeRepeatedRuntimeStream(Drp2WebGpuRuntime, path) {
  const stream = await loadJson(path);
  await smokeRepeatedRuntimeStreamObject(Drp2WebGpuRuntime, stream, path);
}

async function smokeRepeatedRuntimeStreamObject(Drp2WebGpuRuntime, stream, label) {
  const runtime = new Drp2WebGpuRuntime(device, context, 'rgba8unorm', {
    canvas: fakeCanvas,
    requireExplicitBindGroupLayouts: true,
    requireExplicitPipelineMetadata: true,
  });
  await runtime.load(stream);
  await runtime.render();
  const firstStats = runtime.resourceStats();
  if (firstStats.refs.open !== 0 || firstStats.refs.recorded !== 0 || firstStats.refs.submitted !== 0) {
    throw new Error(
      `${label}: resource refs leaked after repeated frame 1: ` +
        `open=${firstStats.refs.open} recorded=${firstStats.refs.recorded} ` +
        `submitted=${firstStats.refs.submitted}`,
    );
  }

  const stableStats = comparableResourceStats(runtime.resourceStats());
  for (let i = 1; i < 10; i++) {
    await runtime.render();
    const stats = runtime.resourceStats();
    assertResourceStatsStable(
      comparableResourceStats(stats),
      stableStats,
      `${label}: resource stats changed after repeated frame ${i + 1}`,
    );
    if (stats.refs.open !== 0 || stats.refs.recorded !== 0 || stats.refs.submitted !== 0) {
      throw new Error(
        `${label}: resource refs leaked after repeated frame ${i + 1}: ` +
          `open=${stats.refs.open} recorded=${stats.refs.recorded} ` +
          `submitted=${stats.refs.submitted}`,
      );
    }
  }
}

async function smokeDestroyAfterSubmittedWork(executeDrp2Stream) {
  await executeDrp2Stream(
    device,
    context,
    'bgra8unorm',
    {
      commands: [
        ...header,
        { cmd: 'CreateBuffer', id: 1, size: 4, usage: ['COPY_SRC'] },
        { cmd: 'CreateBuffer', id: 2, size: 4, usage: ['COPY_DST'] },
        { cmd: 'BeginCommandEncoder', id: 10 },
        {
          cmd: 'CopyBufferToBuffer',
          encoder_id: 10,
          src_buffer_id: 1,
          src_offset: 0,
          dst_buffer_id: 2,
          dst_offset: 0,
          size: 4,
        },
        { cmd: 'FinishCommandEncoder', encoder_id: 10, command_buffer_id: 11 },
        { cmd: 'QueueSubmit', command_buffer_id: 11 },
        { cmd: 'DestroyBuffer', buffer_id: 1 },
        { cmd: 'DestroyBuffer', buffer_id: 2 },
      ],
    },
    { canvas: fakeCanvas, retireSubmittedRefs: true },
  );
}

async function smokeBrowserCanvasDepthCache(Drp2WebGpuRuntime) {
  const runtime = new Drp2WebGpuRuntime(device, context, 'rgba8unorm', {
    canvas: fakeCanvas,
    requireExplicitBindGroupLayouts: true,
    requireExplicitPipelineMetadata: true,
  });
  await runtime.load({
    commands: [
      ...header,
      ...triangleShaders,
      renderPipeline(
        [{ format: 'rgba8unorm', write_mask: ['all'] }],
        { format: 'depth32float', depth_write_enabled: true, depth_compare: 'less' },
      ),
      { cmd: 'BeginCommandEncoder', id: 20 },
      renderPass(
        [colorAttachment(0)],
        {
          texture_id: 0,
          depth_load_op: 'clear',
          depth_store_op: 'store',
          depth_clear_value: 1,
        },
      ),
      { cmd: 'SetPipeline', pass_id: 21, pipeline_id: 10 },
      { cmd: 'Draw', pass_id: 21, vertex_count: 3, instance_count: 1 },
      { cmd: 'EndRenderPass', pass_id: 21 },
      { cmd: 'FinishCommandEncoder', encoder_id: 20, command_buffer_id: 22 },
      { cmd: 'QueueSubmit', command_buffer_id: 22 },
    ],
  });
  await runtime.render();
  const firstStats = comparableResourceStats(runtime.resourceStats());
  if (firstStats.browserCanvasDepthTextures !== 1) {
    throw new Error(
      `browser canvas depth cache expected one texture, got ` +
        `${firstStats.browserCanvasDepthTextures}`,
    );
  }
  await runtime.render();
  assertResourceStatsStable(
    comparableResourceStats(runtime.resourceStats()),
    firstStats,
    'browser canvas depth cache changed across retained renders',
  );
}

async function smokeBrowserPresentResizeRetention(Drp2WebGpuRuntime) {
  observedShaderModules = [];
  observedTextureDescriptors = [];
  observedRenderPipelineDescriptors = [];
  fakeCanvas.width = 640;
  fakeCanvas.height = 480;
  fakeCanvas.clientWidth = 640;
  fakeCanvas.clientHeight = 480;
  const runtime = new Drp2WebGpuRuntime(device, context, 'rgba8unorm', {
    canvas: fakeCanvas,
    browserPresentFormat: 'rgba16float',
    requireExplicitBindGroupLayouts: true,
    requireExplicitPipelineMetadata: true,
  });
  await runtime.load({
    commands: [
      ...header,
      ...triangleShaders,
      renderPipeline([{ format: 'rgba16float', write_mask: ['all'] }]),
      { cmd: 'BeginCommandEncoder', id: 20 },
      renderPass([colorAttachment(0)]),
      { cmd: 'SetPipeline', pass_id: 21, pipeline_id: 10 },
      { cmd: 'Draw', pass_id: 21, vertex_count: 3, instance_count: 1 },
      { cmd: 'EndRenderPass', pass_id: 21 },
      { cmd: 'FinishCommandEncoder', encoder_id: 20, command_buffer_id: 22 },
      { cmd: 'QueueSubmit', command_buffer_id: 22 },
    ],
  });

  const beforeFirstRender = queueSubmitCount;
  await runtime.render();
  const presentShader = observedShaderModules.find(
    (module) => module.label === 'browser-present-shader',
  );
  if (
    presentShader === undefined ||
    !presentShader.code.includes('linear_to_srgb(linear.rgb)') ||
    !presentShader.code.includes('linear.a')
  ) {
    throw new Error('browser presentation must encode linear RGB to sRGB and preserve alpha');
  }
  const presentTexture = observedTextureDescriptors.find(
    (texture) => texture.label?.includes('browser-present-color'),
  );
  if (presentTexture?.format !== 'rgba16float') {
    throw new Error(
      `browser scene intermediate must be rgba16float, got ${presentTexture?.format}`,
    );
  }
  const presentPipeline = observedRenderPipelineDescriptors.find(
    (pipeline) => pipeline.label === 'browser-present-pipeline',
  );
  if (presentPipeline?.fragment?.targets?.[0]?.format !== 'rgba8unorm') {
    throw new Error('browser presentation pipeline must target the configured canvas format');
  }
  const firstStats = runtime.resourceStats();
  if (firstStats.browserPresentTextures !== 1) {
    throw new Error(`browser present cache expected one texture, got ${firstStats.browserPresentTextures}`);
  }
  if (queueSubmitCount - beforeFirstRender !== 2) {
    throw new Error(`browser present first frame expected 2 submits, got ${queueSubmitCount - beforeFirstRender}`);
  }

  fakeCanvas.width = 800;
  fakeCanvas.height = 360;
  fakeCanvas.clientWidth = 800;
  fakeCanvas.clientHeight = 360;
  const beforeResizeRender = queueSubmitCount;
  await runtime.render();
  const resizeStats = runtime.resourceStats();
  if (resizeStats.browserPresentTextures !== 1) {
    throw new Error(`browser present resize kept ${resizeStats.browserPresentTextures} textures after present`);
  }
  if (queueSubmitCount - beforeResizeRender !== 3) {
    throw new Error(
      `browser present resize expected stale+scene+present submits, got ` +
        `${queueSubmitCount - beforeResizeRender}`,
    );
  }

  fakeCanvas.width = 640;
  fakeCanvas.height = 480;
  fakeCanvas.clientWidth = 640;
  fakeCanvas.clientHeight = 480;
}



async function smokeBrowserPresentMsaaResolve(Drp2WebGpuRuntime) {
  observedRenderPassDescriptors = [];
  observedRenderPipelineDescriptors = [];
  const runtime = new Drp2WebGpuRuntime(device, context, 'rgba8unorm', {
    canvas: fakeCanvas,
    browserPresentFormat: 'rgba16float',
    requireExplicitBindGroupLayouts: true,
    requireExplicitPipelineMetadata: true,
  });
  const msaaTexture = texture(30, 'rgba16float');
  msaaTexture.sample_count = 4;
  const pipeline = renderPipeline([{ format: 'rgba16float', write_mask: ['all'] }]);
  pipeline.multisample = { sample_count: 4, alpha_to_coverage_enabled: false };
  const attachment = colorAttachment(30);
  attachment.resolve_target = { texture_id: 0, mode: 1 };
  await runtime.load({
    capabilities: {
      supported_texture_formats: ['rgba16float'],
      supported_sample_counts: [1, 4],
    },
    commands: [
      ...header,
      ...triangleShaders,
      msaaTexture,
      pipeline,
      { cmd: 'BeginCommandEncoder', id: 20 },
      renderPass([attachment]),
      { cmd: 'SetPipeline', pass_id: 21, pipeline_id: 10 },
      { cmd: 'Draw', pass_id: 21, vertex_count: 3, instance_count: 1 },
      { cmd: 'EndRenderPass', pass_id: 21 },
      { cmd: 'FinishCommandEncoder', encoder_id: 20, command_buffer_id: 22 },
      { cmd: 'QueueSubmit', command_buffer_id: 22 },
    ],
  });
  await runtime.render();
  const scenePass = observedRenderPassDescriptors.find(
    (desc) => desc.label !== 'browser-present-pass',
  );
  if (scenePass?.colorAttachments?.[0]?.resolveTarget === undefined) {
    throw new Error('browser MSAA pass did not receive a presentation resolveTarget');
  }
  const scenePipeline = observedRenderPipelineDescriptors.find(
    (desc) => desc.label !== 'browser-present-pipeline',
  );
  if (scenePipeline?.multisample?.count !== 4) {
    throw new Error(
      `browser MSAA pipeline expected sample count 4, got ${scenePipeline?.multisample?.count}`,
    );
  }
}

async function smokeStreamPathsOnly(Drp2WebGpuRuntime, executeDrp2Stream, paths) {
  if (paths.length === 0) {
    throw new Error('--streams-only needs at least one stream path');
  }

  for (const path of paths) {
    const stream = await loadJson(path);
    try {
      await executeDrp2Stream(device, context, 'rgba8unorm', stream, {
        canvas: fakeCanvas,
        requireExplicitBindGroupLayouts: true,
        requireExplicitPipelineMetadata: true,
      });
      await smokeRepeatedRuntimeStreamObject(Drp2WebGpuRuntime, stream, path);
    } catch (error) {
      throw new Error(`${path}: ${error.message}`);
    }
  }

  console.log(passLine(`WebGPU runner smoke generated_streams=${paths.length}`));
}

async function smokeDemoPath(WebGpuDemoSession) {
  const session = new WebGpuDemoSession(device, context, 'rgba8unorm', {
    supported_shader_formats: ['wgsl'],
    supported_texture_formats: ['rgba8unorm', 'depth32float'],
    supported_sample_counts: [1, 4],
    supports_fp64: false,
  }, {
    canvas: fakeCanvas,
  });

  const pointStream = await loadJson('examples/webgpu/streams/scene_point_wgsl.json');
  await session.loadStreamObject('scene_point_wgsl', pointStream);
  await session.render();
  const pointStats = comparableResourceStats(session.resourceStats());

  fakeCanvas.width = 1;
  fakeCanvas.height = 1;
  await session.render();
  assertResourceStatsStable(
    comparableResourceStats(session.resourceStats()),
    pointStats,
    'demo resize reload changed point resource stats',
  );

  const textureStream = await loadJson('examples/webgpu/streams/texture_sampling_wgsl.json');
  await session.loadStreamObject('texture_sampling_wgsl', textureStream);
  await session.render();
  if (session.resourceStats().refs.open !== 0 || session.resourceStats().refs.recorded !== 0) {
    throw new Error('demo texture reload leaked open or recorded refs');
  }

  await session.loadStreamObject('scene_point_wgsl', pointStream);
  await session.render();
  assertResourceStatsStable(
    comparableResourceStats(session.resourceStats()),
    pointStats,
    'demo stream reload changed point resource stats',
  );
}

async function smokePacketSessionValidation(Drp2WebGpuRuntime, executeDrp2Stream) {
  const arena = new Uint8Array([1, 2, 3, 4, 0, 0, 0, 0]);
  const decoded = decodeDrp2Packet(writeBufferPacket(), arena);
  if (
    decoded.kind !== 'update' ||
    decoded.commands.length !== 1 ||
    decoded.commands[0].cmd !== 'WriteBuffer' ||
    decoded.commands[0].data.byteLength !== 4
  ) {
    throw new Error('DRP2 packet decoder failed WriteBuffer smoke');
  }
  const badMagic = writeBufferPacket();
  badMagic[0] = 0;
  expectThrows(() => decodeDrp2Packet(badMagic, arena), 'magic');
  expectThrows(
    () => decodeDrp2Packet(
      writeBufferPacket({ payloadOffset: 8n, payloadSize: 4n, arenaSize: 10n }),
      new Uint8Array(10),
    ),
    'payload span',
  );
  expectThrows(
    () => decodeDrp2PacketSet({
      setup: { packet: emptyPacket(3, 1, 1) },
      frame: { packet: emptyPacket(3, 1, 1) },
    }),
    'phase setup contains frame packet',
  );

  const runtime = new Drp2WebGpuRuntime(device, context, 'bgra8unorm', { canvas: fakeCanvas });
  await expectAsyncFailure(
    () => runtime.executePacketSet({}),
    'needs a frame packet',
  );
  await runtime.executePacketSet({ frame: { packet: emptyPacket(3, 1, 0) } });
  await expectAsyncFailure(
    () => runtime.executePacketSet({ frame: { packet: emptyPacket(3, 1, 0) } }),
    'stale DRP2 packet frame_index',
  );
  await expectAsyncFailure(
    () => runtime.executePacketSet({ frame: { packet: emptyPacket(3, 0, 1) } }),
    'stale DRP2 packet resource_version',
  );
  await expectAsyncFailure(
    () => runtime.executePacketSet({
      setup: { packet: emptyPacket(1, 2, 3) },
      frame: { packet: emptyPacket(3, 2, 4) },
    }),
    'inconsistent version counters',
  );
  await runtime.executePacketSet({ frame: { packet: emptyPacket(3, 1, 0) } }, { reset: true });

  const realistic = new Drp2WebGpuRuntime(device, context, 'bgra8unorm', { canvas: fakeCanvas });
  const beforeBuffers = createdBufferCount;
  await realistic.executePacketSet(bufferPacketSet(10, 1, 7n, true), { reset: true });
  const firstStats = realistic.resourceStats();
  if (firstStats.buffers !== 1 || firstStats.destroyedObjects !== 0) {
    throw new Error(`realistic packet setup did not create one live buffer: ${JSON.stringify(firstStats)}`);
  }
  if (createdBufferCount !== beforeBuffers + 1) {
    throw new Error('realistic packet setup did not create a GPU buffer');
  }
  await realistic.executePacketSet(bufferPacketSet(11, 2));
  const secondStats = realistic.resourceStats();
  if (secondStats.buffers !== 1 || secondStats.destroyedObjects !== 0) {
    throw new Error(`realistic packet update did not preserve one live buffer: ${JSON.stringify(secondStats)}`);
  }
  await expectAsyncFailure(
    () => realistic.executePacketSet(bufferPacketSet(10, 1)),
    'stale DRP2 packet resource_version',
  );
  await realistic.executePacketSet(bufferPacketSet(1, 1, 7n, true), { reset: true });
  const resetStats = realistic.resourceStats();
  if (resetStats.buffers !== 1 || resetStats.destroyedObjects !== 0) {
    throw new Error(`realistic packet reset did not rebuild one live buffer: ${JSON.stringify(resetStats)}`);
  }
  await realistic.executePacketSet(bufferPacketSet(2, 2));
  await expectAsyncFailure(
    () => realistic.executePacketSet(bufferPacketSet(1, 1)),
    'stale DRP2 packet resource_version',
  );

  const cleanupRuntime = new Drp2WebGpuRuntime(device, context, 'bgra8unorm', { canvas: fakeCanvas });
  await cleanupRuntime.load({ commands: [texture(5000, 'rgba8unorm')] });
  cleanupRuntime.frameCommands = [
    { cmd: 'BeginCommandEncoder', id: 20 },
    renderPass([colorAttachment(5000)]),
    { cmd: 'SetPipeline', pass_id: 21, pipeline_id: 999 },
  ];
  await expectAsyncFailure(
    () => cleanupRuntime.render(),
    'unknown render pipeline 999',
  );
  const failedStats = cleanupRuntime.resourceStats();
  if (failedStats.refs.open !== 0 || failedStats.refs.recorded !== 0) {
    throw new Error(`failed render leaked local execution refs: ${JSON.stringify(failedStats.refs)}`);
  }
  const replacement = { commands: [texture(5000, 'rgba8unorm')] };
  await executeDrp2Stream(device, context, 'bgra8unorm', replacement, {
    canvas: fakeCanvas,
    commands: replacement.commands,
    replaceExistingResources: true,
    state: cleanupRuntime.state,
  });
}

async function main() {
  const { Drp2WebGpuRuntime, WebGpuDemoSession, executeDrp2Stream } = await import(
    '../web/drp2/webgpu.js'
  );

  const args = process.argv.slice(2);
  if (args[0] === '--streams-only') {
    await smokeStreamPathsOnly(Drp2WebGpuRuntime, executeDrp2Stream, args.slice(1));
    return;
  }

  await smokePacketSessionValidation(Drp2WebGpuRuntime, executeDrp2Stream);

  const manifest = await loadJson('examples/webgpu/fixture_manifest.json');
  for (const entry of manifest.positive) {
    const path = entry.startsWith('/') ? entry.slice(1) : entry;
    const stream = await loadJson(path);
    try {
      await executeDrp2Stream(device, context, 'bgra8unorm', stream, {
        canvas: fakeCanvas,
        requireExplicitBindGroupLayouts: true,
        requireExplicitPipelineMetadata: true,
      });
    } catch (error) {
      throw new Error(`${path}: ${error.message}`);
    }
  }

  const streamPaths = (manifest.webgpu_streams ?? exampleStreams).map((entry) =>
    entry.startsWith('/') ? entry.slice(1) : entry,
  );
  for (const path of streamPaths) {
    const stream = await loadJson(path);
    try {
      await executeDrp2Stream(device, context, 'bgra8unorm', stream, {
        canvas: fakeCanvas,
        requireExplicitBindGroupLayouts: true,
        requireExplicitPipelineMetadata: true,
      });
    } catch (error) {
      throw new Error(`${path}: ${error.message}`);
    }
  }

  const negativePaths = (manifest.negative_parity ?? negativeFixtureParity).map((entry) =>
    entry.startsWith('/') ? entry.slice(1) : entry,
  );

  observedViewports = [];
  observedScissors = [];
  await executeDrp2Stream(device, context, 'bgra8unorm', {
    commands: [
      ...header,
      { cmd: 'BeginCommandEncoder', id: 1 },
      {
        cmd: 'BeginRenderPass',
        id: 2,
        encoder_id: 1,
        color_attachments: [{ texture_id: 0 }],
        viewport: { x: 0, y: 1, width: 2, height: 3, min_depth: 0, max_depth: 1 },
        scissor: { x: 0, y: 1, width: 2, height: 3 },
      },
      {
        cmd: 'SetViewport',
        pass_id: 2,
        x: 1,
        y: 2,
        width: 4,
        height: 5,
        min_depth: 0.25,
        max_depth: 0.75,
      },
      { cmd: 'SetScissor', pass_id: 2, x: 2, y: 3, width: 5, height: 6 },
      { cmd: 'SetBlendConstant', pass_id: 2, color: { r: 0, g: 0, b: 0, a: 1 } },
      { cmd: 'SetStencilReference', pass_id: 2, reference: 1 },
      { cmd: 'EndRenderPass', pass_id: 2 },
      { cmd: 'FinishCommandEncoder', encoder_id: 1, command_buffer_id: 3 },
      { cmd: 'QueueSubmit', command_buffer_id: 3 },
    ],
  }, { canvas: fakeCanvas });
  if (
    observedViewports.length !== 2 ||
    observedViewports[0].x !== 0 ||
    observedViewports[0].y !== 1 ||
    observedViewports[0].width !== 2 ||
    observedViewports[0].height !== 3 ||
    observedViewports[1].x !== 1 ||
    observedViewports[1].y !== 2 ||
    observedViewports[1].width !== 4 ||
    observedViewports[1].height !== 5 ||
    observedViewports[1].minDepth !== 0.25 ||
    observedViewports[1].maxDepth !== 0.75
  ) {
    throw new Error(`SetViewport arguments were not forwarded: ${JSON.stringify(observedViewports)}`);
  }
  if (
    observedScissors.length !== 2 ||
    observedScissors[0].x !== 0 ||
    observedScissors[0].y !== 1 ||
    observedScissors[0].width !== 2 ||
    observedScissors[0].height !== 3 ||
    observedScissors[1].x !== 2 ||
    observedScissors[1].y !== 3 ||
    observedScissors[1].width !== 5 ||
    observedScissors[1].height !== 6
  ) {
    throw new Error(`SetScissor arguments were not forwarded: ${JSON.stringify(observedScissors)}`);
  }

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        {
          cmd: 'CreateShaderModule',
          id: 9000,
          stage: 'VERTEX',
          format: 'wgsl',
          entry_point: 'main',
          code:
            'struct In { @location(0) pos: vec3f }; ' +
            '@vertex fn main(input: In) -> @builtin(position) vec4f { ' +
            'return vec4f(input.pos, 1.0); }',
        },
        triangleShaders[1],
        {
          cmd: 'CreateRenderPipeline',
          id: 10,
          vertex_buffer_slots: 1,
          vertex_shader_module_id: 9000,
          fragment_shader_module_id: 9001,
          topology: 'triangle-list',
          color_targets: [{ format: 'rgba8unorm' }],
        },
      ],
    },
    'needs explicit vertex_buffers',
    {
      commandIndex: 4,
      cmd: 'CreateRenderPipeline',
      code: 'DRP2_ERR_INVALID_ARGUMENT',
    },
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        {
          cmd: 'CreateTexture',
          id: 1,
          dimension: '2d',
          width: 'canvas',
          height: 4,
          depth: 1,
          format: 'rgba8unorm',
          usage: ['RENDER_ATTACHMENT'],
        },
      ],
    },
    'canvas extent alias requires both width and height',
    {
      commandIndex: 2,
      cmd: 'CreateTexture',
      code: 'DRP2_ERR_INVALID_ARGUMENT',
    },
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        { cmd: 'CreateBuffer', id: 1, size: 4, usage: ['COPY_DST'] },
        { cmd: 'DestroyBuffer', buffer_id: 1 },
        { cmd: 'WriteBuffer', buffer_id: 1, offset: 0, data: '', size: 0 },
      ],
    },
    'destroyed',
    { commandIndex: 4, cmd: 'WriteBuffer', code: 'DRP2_ERR_INVALID_STATE' },
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        { cmd: 'CreateBuffer', id: 1, size: 4, usage: ['COPY_SRC'] },
        { cmd: 'CreateBuffer', id: 2, size: 4, usage: ['COPY_DST'] },
        { cmd: 'BeginCommandEncoder', id: 10 },
        {
          cmd: 'CopyBufferToBuffer',
          encoder_id: 10,
          src_buffer_id: 1,
          src_offset: 0,
          dst_buffer_id: 2,
          dst_offset: 0,
          size: 4,
        },
        { cmd: 'FinishCommandEncoder', encoder_id: 10, command_buffer_id: 11 },
        { cmd: 'DestroyBuffer', buffer_id: 1 },
      ],
    },
    'recorded work',
  );
  await smokeDestroyAfterSubmittedWork(executeDrp2Stream);

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        texture(1, 'rgba8unorm'),
        texture(2, 'rgba8unorm'),
        ...triangleShaders,
        renderPipeline([{ format: 'rgba8unorm', write_mask: ['all'] }]),
        { cmd: 'BeginCommandEncoder', id: 20 },
        renderPass([colorAttachment(1), colorAttachment(2)]),
        { cmd: 'SetPipeline', pass_id: 21, pipeline_id: 10 },
      ],
    },
    'color target count',
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        texture(1, 'r16float'),
        ...triangleShaders,
        renderPipeline([{ format: 'rgba8unorm', write_mask: ['all'] }]),
        { cmd: 'BeginCommandEncoder', id: 20 },
        renderPass([colorAttachment(1)]),
        { cmd: 'SetPipeline', pass_id: 21, pipeline_id: 10 },
      ],
    },
    'does not match render pass',
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        texture(1, 'rgba8unorm'),
        ...triangleShaders,
        renderPipeline(
          [{ format: 'rgba8unorm', write_mask: ['all'] }],
          { format: 'depth32float', depth_write_enabled: true, depth_compare: 'less' },
        ),
        { cmd: 'BeginCommandEncoder', id: 20 },
        renderPass([colorAttachment(1)]),
        { cmd: 'SetPipeline', pass_id: 21, pipeline_id: 10 },
      ],
    },
    'depth_stencil format',
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        ...triangleShaders,
        renderPipeline([
          {
            format: 'r32uint',
            blend: {
              color: { src_factor: 'one', dst_factor: 'zero', operation: 'add' },
              alpha: { src_factor: 'one', dst_factor: 'zero', operation: 'add' },
            },
          },
        ]),
      ],
    },
    'does not support blending',
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        ...triangleShaders,
        renderPipeline([{ format: 'rgba8unorm', write_mask: ['all', 'red'] }]),
      ],
    },
    'write_mask cannot combine all',
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        texture(1, 'rgba8unorm'),
        { cmd: 'BeginCommandEncoder', id: 20 },
        renderPass([colorAttachment(1, 'preserve')]),
      ],
    },
    'unsupported load_op',
  );

  await expectCapabilityPreflightFailure(
    executeDrp2Stream,
    {
      capabilities: { supported_texture_formats: ['bgra8unorm'] },
      commands: [
        ...header,
        { cmd: 'CreateBuffer', id: 1, size: 4, usage: ['COPY_DST'] },
        ...triangleShaders,
        renderPipeline([{ format: 'rgba8unorm', write_mask: ['all'] }]),
      ],
    },
    'color target format rgba8unorm is not supported by capabilities',
    {
      commandIndex: 5,
      cmd: 'CreateRenderPipeline',
      code: 'DRP2_ERR_UNSUPPORTED_CAPABILITY',
    },
  );

  await expectFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        { cmd: 'CreateBuffer', id: 1, size: 4, usage: ['COPY_DST'] },
        { cmd: 'ResourceBarrier' },
      ],
    },
    'unknown buffer undefined',
    {
      commandIndex: 3,
      cmd: 'ResourceBarrier',
      code: 'DRP2_ERR_INVALID_ID',
    },
  );

  for (const path of negativePaths) {
    await expectNegativeFixtureParity(executeDrp2Stream, path);
  }

  await smokeRepeatedRuntimeFrames(Drp2WebGpuRuntime);
  await smokeBrowserCanvasDepthCache(Drp2WebGpuRuntime);
  await smokeBrowserPresentResizeRetention(Drp2WebGpuRuntime);
  await smokeBrowserPresentMsaaResolve(Drp2WebGpuRuntime);
  await smokeDemoPath(WebGpuDemoSession);

  console.log(
    passLine(
      `WebGPU runner smoke fixtures=${manifest.positive.length} ` +
        `streams=${streamPaths.length} negatives=${negativePaths.length}`,
    ),
  );
}

await main();
