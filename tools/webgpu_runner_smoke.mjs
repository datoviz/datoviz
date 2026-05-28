#!/usr/bin/env node

import { readFile } from 'node:fs/promises';

globalThis.document = {
  querySelector() {
    return null;
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
  queue: {
    writeBuffer() {},
    writeTexture() {},
    submit() {},
    async onSubmittedWorkDone() {},
  },
  createBuffer(desc = {}) {
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

async function loadJson(path) {
  return JSON.parse(await readFile(path, 'utf8'));
}

async function expectFailure(executeDrp2Stream, stream, expectedText) {
  try {
    await executeDrp2Stream(device, context, 'bgra8unorm', stream);
  } catch (error) {
    const message = String(error.message);
    if (message.includes(expectedText)) {
      return;
    }
    throw new Error(`expected "${expectedText}" failure, got "${message}"`);
  }
  throw new Error(`expected "${expectedText}" failure`);
}

async function main() {
  const { executeDrp2Stream } = await import('../examples/webgpu/drp2_webgpu.js');

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
        { cmd: 'CreateBuffer', id: 1, size: 4, usage: ['COPY_DST'] },
        { cmd: 'DestroyBuffer', buffer_id: 1 },
        { cmd: 'WriteBuffer', buffer_id: 1, offset: 0, data: '', size: 0 },
      ],
    },
    'destroyed',
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

  console.log(`PASS WebGPU runner smoke fixtures=${manifest.positive.length}`);
}

await main();
