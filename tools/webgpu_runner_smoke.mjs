#!/usr/bin/env node

import { readFile } from 'node:fs/promises';

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

const pass = () => ({
  setPipeline() {},
  setVertexBuffer() {},
  setIndexBuffer() {},
  setBindGroup() {},
  setViewport() {},
  setScissorRect() {},
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
    submit() {},
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
  createTexture() {
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
  createShaderModule() {
    return {
      async getCompilationInfo() {
        return { messages: [] };
      },
    };
  },
  createPipelineLayout() {
    return {};
  },
  createRenderPipeline() {
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
      beginRenderPass() {
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

async function expectFailure(executeDrp2Stream, stream, expectedText, expected = {}) {
  try {
    await executeDrp2Stream(device, context, 'bgra8unorm', stream);
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
    await executeDrp2Stream(device, context, 'bgra8unorm', stream);
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
  const runtime = new Drp2WebGpuRuntime(device, context, 'rgba8unorm', {
    requireExplicitBindGroupLayouts: true,
    requireExplicitPipelineMetadata: true,
  });
  await runtime.load(stream);

  const stableStats = comparableResourceStats(runtime.resourceStats());
  for (let i = 0; i < 10; i++) {
    await runtime.render();
    const stats = runtime.resourceStats();
    assertResourceStatsStable(
      comparableResourceStats(stats),
      stableStats,
      `${path}: resource stats changed after repeated frame ${i + 1}`,
    );
    if (stats.refs.open !== 0 || stats.refs.recorded !== 0) {
      throw new Error(
        `${path}: resource refs leaked after repeated frame ${i + 1}: ` +
          `open=${stats.refs.open} recorded=${stats.refs.recorded}`,
      );
    }
  }
}

async function smokeDemoPath(WebGpuDemoSession) {
  const session = new WebGpuDemoSession(device, context, 'rgba8unorm', {
    supported_shader_formats: ['wgsl'],
    supported_texture_formats: ['rgba8unorm', 'depth32float'],
    supported_sample_counts: [1, 4],
    supports_fp64: false,
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

  const panzoomStream = await loadJson('examples/webgpu/streams/scene_point_wgsl.json');
  await session.loadStreamObject('scene_point_panzoom_wgsl', panzoomStream);
  const panzoomStats = comparableResourceStats(session.resourceStats());
  for (const state of [
    { zoomX: 1.2, zoomY: 1.1, offsetX: 0.05, offsetY: -0.04 },
    { zoomX: 0.7, zoomY: 0.9, offsetX: -0.11, offsetY: 0.08 },
  ]) {
    session.setPanzoom(state);
    await session.render();
    assertResourceStatsStable(
      comparableResourceStats(session.resourceStats()),
      panzoomStats,
      'demo panzoom update changed resource stats',
    );
  }

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

async function main() {
  const { Drp2WebGpuRuntime, WebGpuDemoSession, executeDrp2Stream } = await import(
    '../examples/webgpu/drp2_webgpu.js'
  );

  const manifest = await loadJson('examples/webgpu/fixture_manifest.json');
  for (const entry of manifest.positive) {
    const path = entry.startsWith('/') ? entry.slice(1) : entry;
    const stream = await loadJson(path);
    try {
      await executeDrp2Stream(device, context, 'bgra8unorm', stream, {
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

  await executeDrp2Stream(device, context, 'bgra8unorm', {
    commands: [
      ...header,
      { cmd: 'BeginCommandEncoder', id: 1 },
      {
        cmd: 'BeginRenderPass',
        id: 2,
        encoder_id: 1,
        color_attachments: [{ texture_id: 0 }],
      },
      {
        cmd: 'SetViewport',
        pass_id: 2,
        x: 0,
        y: 0,
        width: 4,
        height: 4,
        min_depth: 0,
        max_depth: 1,
      },
      { cmd: 'SetScissor', pass_id: 2, x: 0, y: 0, width: 4, height: 4 },
      { cmd: 'SetBlendConstant', pass_id: 2, color: { r: 0, g: 0, b: 0, a: 1 } },
      { cmd: 'SetStencilReference', pass_id: 2, reference: 1 },
      { cmd: 'EndRenderPass', pass_id: 2 },
      { cmd: 'FinishCommandEncoder', encoder_id: 1, command_buffer_id: 3 },
      { cmd: 'QueueSubmit', command_buffer_id: 3 },
    ],
  });

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

  await expectCapabilityPreflightFailure(
    executeDrp2Stream,
    {
      commands: [
        ...header,
        { cmd: 'CreateBuffer', id: 1, size: 4, usage: ['COPY_DST'] },
        { cmd: 'ResourceBarrier' },
      ],
    },
    'unsupported DRP2 command in WebGPU PoC: ResourceBarrier',
    {
      commandIndex: 3,
      cmd: 'ResourceBarrier',
      code: 'DRP2_ERR_UNSUPPORTED_CAPABILITY',
    },
  );

  for (const path of negativePaths) {
    await expectNegativeFixtureParity(executeDrp2Stream, path);
  }

  await smokeRepeatedRuntimeFrames(Drp2WebGpuRuntime);
  await smokeDemoPath(WebGpuDemoSession);

  console.log(
    `PASS WebGPU runner smoke fixtures=${manifest.positive.length} ` +
      `streams=${streamPaths.length} negatives=${negativePaths.length}`,
  );
}

await main();
