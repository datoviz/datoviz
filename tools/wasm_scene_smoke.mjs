#!/usr/bin/env node

import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { decodeDrp2Packet } from "../web/drp2/packet.js";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const modulePath = resolve(root, "build-wasm-scene/wasm/datoviz_wasm_scene.mjs");
const browserWrapperPath = resolve(root, "web/wasm/scene.js");
const output2dPath = resolve(root, "build-wasm-scene/wasm/wasm_api_scene_point_primitive_image_mesh_panzoom.json");
const output3dPath = resolve(root, "build-wasm-scene/wasm/wasm_api_scene_mesh3d_arcball.json");

const DVZ_POINTER_EVENT_PRESS = 1;
const DVZ_POINTER_EVENT_RELEASE = 0;
const DVZ_POINTER_EVENT_MOVE = 2;
const DVZ_POINTER_BUTTON_LEFT = 1;
const DVZ_FORMAT_R8G8B8A8_UNORM = 37;
const DVZ_CONTROLLER_TYPE_PANZOOM = 1;
const DVZ_CONTROLLER_TYPE_ARCBALL = 2;
const DVZ_DIM_MASK_XY = 3;
const DVZ_DIM_MASK_XYZ = 7;
const DVZ_WASM_VISUAL_POINT = 1;
const DVZ_WASM_VISUAL_IMAGE = 6;
const DVZ_WASM_VISUAL_MESH = 7;
const DVZ_WASM_VISUAL_PRIMITIVE = 9;
const DVZ_DRP2_PACKET_SETUP = 1;
const DVZ_DRP2_PACKET_UPDATE = 2;
const DVZ_DRP2_PACKET_FRAME = 3;

function requireOk(condition, message) {
  if (!condition) throw new Error(message);
}

function expectStatus(status, expected, label) {
  requireOk(status === expected, `${label} returned ${status}, expected ${expected}`);
}

function allocArray(Module, typedArray) {
  const ptr = Module._malloc(typedArray.byteLength);
  requireOk(ptr !== 0, "malloc failed");
  Module.HEAPU8.set(new Uint8Array(typedArray.buffer, typedArray.byteOffset, typedArray.byteLength), ptr);
  return ptr;
}

function allocCString(Module, text) {
  const bytes = new TextEncoder().encode(`${text}\0`);
  const ptr = Module._malloc(bytes.byteLength);
  requireOk(ptr !== 0, "malloc failed");
  Module.HEAPU8.set(bytes, ptr);
  return ptr;
}

function diagnostics(Module, scene) {
  const count = Module._dvz_wasm_api_diagnostic_count(scene);
  const messages = [];
  for (let i = 0; i < count; i++) {
    const ptr = Module._dvz_wasm_api_diagnostic(scene, i);
    messages.push(ptr !== 0 ? Module.UTF8ToString(ptr) : "<null diagnostic>");
  }
  return messages;
}

function expectDiagnostics(Module, scene, needle, label) {
  const messages = diagnostics(Module, scene);
  requireOk(messages.length > 0, `${label}: expected a diagnostic`);
  requireOk(
    messages.some((message) => message.includes(needle)),
    `${label}: expected diagnostic containing ${needle}, got ${messages.join("; ")}`,
  );
}

function expectAnyDiagnostics(Module, scene, label) {
  const messages = diagnostics(Module, scene);
  requireOk(messages.length > 0, `${label}: expected diagnostics`);
}

function expectNoDiagnostics(Module, scene, label) {
  const messages = diagnostics(Module, scene);
  requireOk(messages.length === 0, `${label}: unexpected diagnostics: ${messages.join("; ")}`);
}

function expectNoPayload(Module, scene, label) {
  const ptr = Module._dvz_wasm_api_payload_ptr(scene);
  const size = Module._dvz_wasm_api_payload_size(scene);
  requireOk(ptr === 0 && size === 0, `${label}: expected no live payload, got ptr=${ptr} size=${size}`);
}

function readU16LE(bytes, offset) {
  return bytes[offset] | (bytes[offset + 1] << 8);
}

function readU32LE(bytes, offset) {
  return (
    bytes[offset] |
    (bytes[offset + 1] << 8) |
    (bytes[offset + 2] << 16) |
    (bytes[offset + 3] << 24)
  ) >>> 0;
}

function readU64LEAsNumber(bytes, offset) {
  return readU32LE(bytes, offset) + readU32LE(bytes, offset + 4) * 2 ** 32;
}

function packetPayloadRecords(bytes) {
  const commandCount = readU32LE(bytes, 20);
  const records = [];
  let recordOffset = 56;
  for (let i = 0; i < commandCount; i++) {
    const type = readU32LE(bytes, recordOffset);
    const bodySize = readU32LE(bytes, recordOffset + 8);
    const payloadOffsetLow = readU32LE(bytes, recordOffset + 16);
    const payloadOffsetHigh = readU32LE(bytes, recordOffset + 20);
    const payloadSize = readU64LEAsNumber(bytes, recordOffset + 24);
    const bodyPadded = (bodySize + 7) & ~7;
    const hasPayload = payloadOffsetLow !== 0xffffffff || payloadOffsetHigh !== 0xffffffff;
    if (hasPayload && payloadSize > 0) {
      const payloadOffset = payloadOffsetLow + payloadOffsetHigh * 2 ** 32;
      records.push({ index: i, type, offset: payloadOffset, size: payloadSize });
    }
    recordOffset += 32 + bodyPadded;
  }
  return records;
}

function expectPacketPayloadArena(packet, arena, decoded, label) {
  const records = packetPayloadRecords(packet);
  requireOk(records.length > 0, `${label}: expected payload-bearing packet records`);
  for (const record of records) {
    requireOk((record.offset & 7) === 0, `${label}: payload ${record.index} offset is not aligned`);
    requireOk(
      record.offset + record.size <= arena.byteLength,
      `${label}: payload ${record.index} exceeds arena`,
    );
  }
  const payloadEnd = Math.max(...records.map((record) => record.offset + record.size));
  requireOk(payloadEnd <= decoded.arena_size, `${label}: payload end exceeds declared arena size`);
  const writePayloadBytes = records
    .filter((record) => record.type === 18 || record.type === 19)
    .reduce((sum, record) => sum + record.size, 0);
  const decodedWritePayloadBytes = decoded.commands
    .filter((command) => command.cmd === "WriteBuffer" || command.cmd === "WriteTexture")
    .reduce((sum, command) => sum + command.data.byteLength, 0);
  requireOk(
    decodedWritePayloadBytes === writePayloadBytes,
    `${label}: decoded write payload bytes ${decodedWritePayloadBytes} did not match arena records ${writePayloadBytes}`,
  );
  const shaderPayloads = records.filter((record) => record.type === 7).length;
  const decodedShaders = decoded.commands.filter((command) => command.cmd === "CreateShaderModule");
  requireOk(decodedShaders.length >= shaderPayloads, `${label}: missing decoded shader payloads`);
  for (const shader of decodedShaders) {
    requireOk(typeof shader.code === "string" && shader.code.length > 0, `${label}: empty shader code`);
  }
  if (arena.byteLength > 0) {
    const truncatedArena = arena.subarray(0, Math.max(0, payloadEnd - 1));
    let threw = false;
    try {
      decodeDrp2Packet(packet, truncatedArena);
    } catch {
      threw = true;
    }
    requireOk(threw, `${label}: truncated payload arena was accepted`);
  }
}

function emitStream(Module, scene, figure, label) {
  const status = Module._dvz_wasm_api_emit(scene, figure);
  const messages = diagnostics(Module, scene);
  if (status !== 0) {
    requireOk(messages.length > 0, `${label}: no diagnostic was reported`);
    throw new Error(`${label}: ${messages.join("; ")}`);
  }
  requireOk(messages.length === 0, `${label} emit unexpectedly reported diagnostics: ${messages.join("; ")}`);
  const ptr = Module._dvz_wasm_api_payload_ptr(scene);
  const size = Module._dvz_wasm_api_payload_size(scene);
  requireOk(ptr !== 0 && size > 0, `${label} emitted no payload`);
  const payload = new TextDecoder().decode(Module.HEAPU8.subarray(ptr, ptr + size));
  const stream = JSON.parse(payload);
  requireOk(Array.isArray(stream.commands), `${label} stream has no commands array`);
  requireOk(stream.commands.length > 0, `${label} stream has no commands`);
  return { stream, payload, ptr, size };
}

function expectPacket(Module, scene, kind, label, { expectArena = false } = {}) {
  requireOk(typeof Module._dvz_wasm_api_packet_ptr === "function", `${label}: missing packet ABI`);

  const ptr = Module._dvz_wasm_api_packet_ptr(scene, kind);
  const size = Module._dvz_wasm_api_packet_size(scene, kind);
  requireOk(ptr !== 0 && size >= 56, `${label}: missing packet kind ${kind}`);

  const bytes = Module.HEAPU8.subarray(ptr, ptr + size);
  requireOk(
    bytes[0] === 0x44 && bytes[1] === 0x56 && bytes[2] === 0x50 && bytes[3] === 0x32 &&
      bytes[4] === 0x50 && bytes[5] === 0x4b && bytes[6] === 0x54 && bytes[7] === 0,
    `${label}: bad packet magic`,
  );
  requireOk(readU16LE(bytes, 8) === 56, `${label}: bad header size`);
  requireOk(readU16LE(bytes, 10) === 2, `${label}: bad major`);
  requireOk(readU16LE(bytes, 14) === kind, `${label}: bad packet kind`);

  const commandCount = readU32LE(bytes, 20);
  const commandBytes = readU64LEAsNumber(bytes, 24);
  const arenaSize = readU64LEAsNumber(bytes, 32);
  requireOk(commandCount > 0, `${label}: no commands`);
  requireOk(commandBytes + 56 === size, `${label}: command bytes mismatch`);

  const arenaPtr = Module._dvz_wasm_api_packet_arena_ptr(scene, kind);
  const arenaSizeApi = Module._dvz_wasm_api_packet_arena_size(scene, kind);
  requireOk(arenaSizeApi === arenaSize, `${label}: arena size mismatch`);
  if (expectArena) requireOk(arenaPtr !== 0 && arenaSize > 0, `${label}: expected arena`);
  const arena = arenaPtr !== 0 ? Module.HEAPU8.subarray(arenaPtr, arenaPtr + arenaSizeApi) : new Uint8Array();
  const decoded = decodeDrp2Packet(bytes, arena);
  requireOk(decoded.kind_id === kind, `${label}: decoded wrong packet kind`);
  requireOk(decoded.commands.length === commandCount, `${label}: decoded command count mismatch`);
  if (packetPayloadRecords(bytes).length > 0) {
    requireOk(arenaPtr !== 0 && arenaSize > 0, `${label}: payload records require arena`);
    expectPacketPayloadArena(bytes, arena, decoded, label);
  }
  return { ptr, size, commandCount, arenaPtr, arenaSize, decoded };
}

function emitPacketStream(Module, scene, figure, label) {
  requireOk(typeof Module._dvz_wasm_api_emit_packets === "function", `${label}: missing packet emit ABI`);

  const status = Module._dvz_wasm_api_emit_packets(scene, figure);
  const messages = diagnostics(Module, scene);
  if (status !== 0) {
    requireOk(messages.length > 0, `${label}: no diagnostic was reported`);
    throw new Error(`${label}: ${messages.join("; ")}`);
  }
  requireOk(Module._dvz_wasm_api_packet_status(scene) === 0, `${label}: packet status not OK`);
  requireOk(messages.length === 0, `${label}: diagnostics: ${messages.join("; ")}`);
  requireOk(Module._dvz_wasm_api_resource_version(scene) > 0, `${label}: missing resource version`);
  requireOk(Module._dvz_wasm_api_frame_index(scene) > 0, `${label}: missing frame index`);

  return {
    setup: expectPacket(Module, scene, DVZ_DRP2_PACKET_SETUP, `${label} setup`),
    update: expectPacket(Module, scene, DVZ_DRP2_PACKET_UPDATE, `${label} update`, { expectArena: true }),
    frame: expectPacket(Module, scene, DVZ_DRP2_PACKET_FRAME, `${label} frame`),
  };
}

async function expectBrowserWrapperPacketRuntime() {
  const source = await readFile(browserWrapperPath, "utf8");
  const renderInitial = source.match(/async renderInitial\(\) \{[\s\S]*?\n  \}/)?.[0] ?? "";
  const renderIncremental = source.match(/async renderIncremental\(\) \{[\s\S]*?\n  \}/)?.[0] ?? "";
  requireOk(renderInitial.includes("this.emitPackets()"), "renderInitial does not emit packets");
  requireOk(renderInitial.includes("executePacketSet"), "renderInitial does not execute packet sets");
  requireOk(renderIncremental.includes("this.emitPackets()"), "renderIncremental does not emit packets");
  requireOk(
    renderIncremental.includes("executePacketSet"),
    "renderIncremental does not execute packet sets",
  );
  requireOk(!renderInitial.includes("emitDebugJson"), "renderInitial uses debug JSON");
  requireOk(!renderIncremental.includes("emitDebugJson"), "renderIncremental uses debug JSON");
  requireOk(!renderInitial.includes("_dvz_wasm_api_emit("), "renderInitial uses JSON ABI");
  requireOk(!renderIncremental.includes("_dvz_wasm_api_emit("), "renderIncremental uses JSON ABI");
}

function expectNoLegacyDirectAbi(Module) {
  for (const name of [
    "_dvz_wasm_api_emit_direct",
    "_dvz_wasm_api_payload_count",
    "_dvz_wasm_api_payload_command_index",
    "_dvz_wasm_api_payload_data_ptr",
    "_dvz_wasm_api_payload_data_size",
  ]) {
    requireOk(typeof Module[name] === "undefined", `legacy direct-payload ABI still exported: ${name}`);
  }
}

function commandsOf(stream, cmd) {
  return stream.commands.filter((command) => command.cmd === cmd);
}

function countCommands(stream) {
  const counts = new Map();
  for (const command of stream.commands) {
    counts.set(command.cmd, (counts.get(command.cmd) ?? 0) + 1);
  }
  return counts;
}

function expectCommandCount(stream, cmd, expected, label) {
  const actual = countCommands(stream).get(cmd) ?? 0;
  requireOk(actual === expected, `${label}: expected ${expected} ${cmd}, got ${actual}`);
}

function expectNoSetupCommands(stream, label) {
  for (const cmd of [
    "CreateBuffer",
    "CreateTexture",
    "CreateSampler",
    "CreateBindGroupLayout",
    "CreateBindGroup",
    "CreateShaderModule",
    "CreateRenderPipeline",
  ]) {
    expectCommandCount(stream, cmd, 0, label);
  }
}

function expectSetupCommands(stream, label) {
  const setup = [
    "CreateBuffer",
    "CreateTexture",
    "CreateSampler",
    "CreateBindGroupLayout",
    "CreateBindGroup",
    "CreateShaderModule",
    "CreateRenderPipeline",
  ];
  const found = setup.filter((cmd) => (countCommands(stream).get(cmd) ?? 0) > 0);
  requireOk(found.length > 0, `${label}: expected setup commands`);
}

function expectWriteCommands(stream, label) {
  const writes = commandsOf(stream, "WriteBuffer").length + commandsOf(stream, "WriteTexture").length;
  requireOk(writes > 0, `${label}: expected buffer or texture writes`);
}

function expectAllShadersWgsl(stream, label) {
  const shaders = commandsOf(stream, "CreateShaderModule");
  for (const shader of shaders) {
    requireOk(shader.format === "wgsl", `${label}: shader ${shader.id} is ${shader.format}, not wgsl`);
    requireOk(
      shader.stage === "VERTEX" || shader.stage === "FRAGMENT" || shader.stage === "COMPUTE",
      `${label}: shader ${shader.id} has invalid stage ${shader.stage}`,
    );
  }
}

function expectPipelineMetadata(stream, label) {
  for (const pipeline of commandsOf(stream, "CreateRenderPipeline")) {
    requireOk(
      Number.isInteger(pipeline.vertex_buffer_slots) && pipeline.vertex_buffer_slots >= 0,
      `${label}: render pipeline ${pipeline.id} needs explicit vertex_buffer_slots`,
    );
    requireOk(
      Array.isArray(pipeline.vertex_buffers) &&
        pipeline.vertex_buffers.length === pipeline.vertex_buffer_slots,
      `${label}: render pipeline ${pipeline.id} needs explicit matching vertex_buffers`,
    );
    for (const vertexBuffer of pipeline.vertex_buffers) {
      requireOk(
        vertexBuffer.step_mode === "vertex" || vertexBuffer.step_mode === "instance",
        `${label}: render pipeline ${pipeline.id} has invalid step_mode ${vertexBuffer.step_mode}`,
      );
      requireOk(
        Array.isArray(vertexBuffer.attributes) && vertexBuffer.attributes.length > 0,
        `${label}: render pipeline ${pipeline.id} vertex buffer has no attributes`,
      );
    }
    requireOk(
      Array.isArray(pipeline.bind_group_layout_ids) && pipeline.bind_group_layout_ids.length > 0,
      `${label}: render pipeline ${pipeline.id} needs explicit bind_group_layout_ids`,
    );
    requireOk(
      Array.isArray(pipeline.color_targets) && pipeline.color_targets.length > 0,
      `${label}: render pipeline ${pipeline.id} needs explicit color_targets`,
    );
    for (const target of pipeline.color_targets) {
      requireOk(
        target.format === "rgba8unorm" || target.format === "bgra8unorm" || target.format === "canvas",
        `${label}: render pipeline ${pipeline.id} has unexpected color target format ${target.format}`,
      );
    }
  }
}

function pipelineAttributeFormats(pipeline) {
  return pipeline.vertex_buffers.map((vertexBuffer) => ({
    stepMode: vertexBuffer.step_mode,
    formats: vertexBuffer.attributes.map((attribute) => attribute.format),
  }));
}

function expectPipeline(stream, label, predicate) {
  const pipeline = commandsOf(stream, "CreateRenderPipeline").find(predicate);
  requireOk(pipeline !== undefined, `${label}: missing expected render pipeline`);
  return pipeline;
}

function expectDepthPipeline(pipeline, label, writeEnabled = true, compare = "less-equal") {
  requireOk(pipeline.depth_stencil?.format === "depth32float", `${label}: missing depth32float state`);
  requireOk(
    pipeline.depth_stencil.depth_write_enabled === writeEnabled,
    `${label}: unexpected depth write state`,
  );
  requireOk(
    pipeline.depth_stencil.depth_compare === compare,
    `${label}: unexpected depth compare ${pipeline.depth_stencil.depth_compare}`,
  );
}

function expectCanvasRenderPass(stream, label) {
  const passes = commandsOf(stream, "BeginRenderPass");
  requireOk(passes.length === 1, `${label}: expected one render pass, got ${passes.length}`);
  const pass = passes[0];
  requireOk(
    Array.isArray(pass.color_attachments) && pass.color_attachments.length === 1,
    `${label}: expected one color attachment`,
  );
  requireOk(
    pass.color_attachments[0].texture_id === 0,
    `${label}: expected browser canvas color target texture_id 0`,
  );
  requireOk(
    pass.depth_stencil_attachment?.texture_id === 0,
    `${label}: expected browser canvas depth target texture_id 0`,
  );
  expectCommandCount(stream, "SetViewport", 1, label);
  expectCommandCount(stream, "SetScissor", 1, label);
}

function expectDraw(stream, vertexCount, instanceCount, label) {
  const found = commandsOf(stream, "Draw").some(
    (draw) => draw.vertex_count === vertexCount && draw.instance_count === instanceCount,
  );
  requireOk(
    found,
    `${label}: missing Draw vertex_count=${vertexCount} instance_count=${instanceCount}`,
  );
}

function expectFrameCommandShape(stream, label) {
  expectAllShadersWgsl(stream, label);
  expectPipelineMetadata(stream, label);
  expectCanvasRenderPass(stream, label);
  expectCommandCount(stream, "BeginCommandEncoder", 1, label);
  expectCommandCount(stream, "EndRenderPass", 1, label);
  expectCommandCount(stream, "FinishCommandEncoder", 1, label);
  expectCommandCount(stream, "QueueSubmit", 1, label);
}

function expectCommonSceneStreamShape(stream, label) {
  expectFrameCommandShape(stream, label);
  expectCommandCount(stream, "HelloRenderer", 1, label);
  expectCommandCount(stream, "RendererHelloReply", 1, label);
}

function expect2DSceneStreamShape(stream, label) {
  expectCommonSceneStreamShape(stream, label);
  expectDraw(stream, 6, 5, `${label} point`);
  expectDraw(stream, 3, 1, `${label} primitive`);
  expectDraw(stream, 4, 1, `${label} image`);
  expectDraw(stream, 6, 1, `${label} mesh`);
  const pointPipeline = expectPipeline(
    stream,
    `${label} point`,
    (pipeline) => pipeline.builtin_pipeline === "scene.point",
  );
  requireOk(pointPipeline.topology === "triangle-list", `${label} point: unexpected topology`);
  requireOk(
    JSON.stringify(pipelineAttributeFormats(pointPipeline)) ===
      JSON.stringify([
        { stepMode: "instance", formats: ["float32x3"] },
        { stepMode: "instance", formats: ["unorm8x4"] },
        { stepMode: "instance", formats: ["float32"] },
      ]),
    `${label} point: unexpected vertex attributes`,
  );
  expectDepthPipeline(pointPipeline, `${label} point`);
  const primitivePipeline = expectPipeline(
    stream,
    `${label} primitive`,
    (pipeline) =>
      pipeline.builtin_pipeline === "scene.primitive" &&
      pipeline.vertex_buffer_slots === 2 &&
      pipeline.topology === "triangle-list",
  );
  requireOk(
    JSON.stringify(pipelineAttributeFormats(primitivePipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["unorm8x4"] },
      ]),
    `${label} primitive: unexpected vertex attributes`,
  );
  expectDepthPipeline(primitivePipeline, `${label} primitive`);
  const imagePipeline = expectPipeline(
    stream,
    `${label} image`,
    (pipeline) => pipeline.builtin_pipeline === "scene.image",
  );
  requireOk(imagePipeline.topology === "triangle-strip", `${label} image: unexpected topology`);
  requireOk(
    JSON.stringify(pipelineAttributeFormats(imagePipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["float32x2"] },
      ]),
    `${label} image: unexpected vertex attributes`,
  );
  expectDepthPipeline(imagePipeline, `${label} image`, false, "always");
  const meshPipeline = expectPipeline(
    stream,
    `${label} mesh`,
    (pipeline) =>
      pipeline.builtin_pipeline === "scene.primitive" &&
      pipeline.vertex_buffer_slots === 3 &&
      pipeline.topology === "triangle-list",
  );
  requireOk(
    JSON.stringify(pipelineAttributeFormats(meshPipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["unorm8x4"] },
        { stepMode: "vertex", formats: ["float32x3"] },
      ]),
    `${label} mesh: unexpected vertex attributes`,
  );
  expectDepthPipeline(meshPipeline, `${label} mesh`);
  const textures = commandsOf(stream, "CreateTexture").filter((command) => command.format === "rgba8unorm");
  const writes = commandsOf(stream, "WriteTexture");
  requireOk(textures.length === 1, `${label}: expected one RGBA8 image texture, got ${textures.length}`);
  requireOk(textures[0].width === 8 && textures[0].height === 8, `${label}: unexpected image texture size`);
  requireOk(
    textures[0].usage.includes("COPY_DST") && textures[0].usage.includes("TEXTURE_BINDING"),
    `${label}: image texture needs COPY_DST and TEXTURE_BINDING usage`,
  );
  requireOk(writes.length === 1, `${label}: expected one image texture upload, got ${writes.length}`);
  requireOk(
    writes[0].texture_id === textures[0].id &&
      writes[0].size?.width === 8 &&
      writes[0].size?.height === 8,
    `${label}: image texture upload does not match texture resource`,
  );
  requireOk(writes[0].bytes_per_row === 32, `${label}: unexpected image upload row pitch`);
  requireOk(
    commandsOf(stream, "CreateRenderPipeline").length === 4,
    `${label}: expected point, primitive, image, and mesh pipelines`,
  );
}

function expect2DUpdateStreamShape(stream, label, pointInstances = 5) {
  expectFrameCommandShape(stream, label);
  expectNoSetupCommands(stream, label);
  expectDraw(stream, 6, pointInstances, `${label} point`);
  expectDraw(stream, 3, 1, `${label} primitive`);
  expectDraw(stream, 4, 1, `${label} image`);
  expectDraw(stream, 6, 1, `${label} mesh`);
}

function expect3DSceneStreamShape(stream, label) {
  expectCommonSceneStreamShape(stream, label);
  expectDraw(stream, 36, 1, `${label} mesh`);
  const meshPipeline = expectPipeline(
    stream,
    `${label} mesh`,
    (pipeline) =>
      pipeline.builtin_pipeline === "scene.primitive" &&
      pipeline.vertex_buffer_slots === 3 &&
      pipeline.topology === "triangle-list",
  );
  requireOk(
    JSON.stringify(pipelineAttributeFormats(meshPipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["unorm8x4"] },
        { stepMode: "vertex", formats: ["float32x3"] },
      ]),
    `${label} mesh: unexpected vertex attributes`,
  );
  expectDepthPipeline(meshPipeline, `${label} mesh`);
  requireOk(
    commandsOf(stream, "CreateRenderPipeline").length === 1,
    `${label}: expected one 3D mesh pipeline`,
  );
}

function expect3DUpdateStreamShape(stream, label) {
  expectFrameCommandShape(stream, label);
  expectNoSetupCommands(stream, label);
  expectDraw(stream, 36, 1, `${label} mesh`);
}

function makeCubeMesh(size) {
  const s = size / 2;
  const faces = [
    { n: [0, 0, 1], c: [90, 170, 255, 255], v: [[-s, -s, s], [s, -s, s], [-s, s, s], [s, -s, s], [s, s, s], [-s, s, s]] },
    { n: [0, 0, -1], c: [255, 135, 210, 255], v: [[s, -s, -s], [-s, -s, -s], [s, s, -s], [-s, -s, -s], [-s, s, -s], [s, s, -s]] },
    { n: [1, 0, 0], c: [85, 230, 190, 255], v: [[s, -s, s], [s, -s, -s], [s, s, s], [s, -s, -s], [s, s, -s], [s, s, s]] },
    { n: [-1, 0, 0], c: [255, 190, 90, 255], v: [[-s, -s, -s], [-s, -s, s], [-s, s, -s], [-s, -s, s], [-s, s, s], [-s, s, -s]] },
    { n: [0, 1, 0], c: [170, 125, 255, 255], v: [[-s, s, s], [s, s, s], [-s, s, -s], [s, s, s], [s, s, -s], [-s, s, -s]] },
    { n: [0, -1, 0], c: [245, 115, 95, 255], v: [[-s, -s, -s], [s, -s, -s], [-s, -s, s], [s, -s, -s], [s, -s, s], [-s, -s, s]] },
  ];
  const positions = [];
  const colors = [];
  const normals = [];
  for (const face of faces) {
    for (const vertex of face.v) {
      positions.push(...vertex);
      colors.push(...face.c);
      normals.push(...face.n);
    }
  }
  return { positions: new Float32Array(positions), colors: new Uint8Array(colors), normals: new Float32Array(normals) };
}

function setF32(Module, visual, attrPtr, dataPtr, count, label) {
  expectStatus(Module._dvz_wasm_api_visual_set_f32(visual, attrPtr, dataPtr, count), 0, label);
}

function setRGBA8(Module, visual, attrPtr, dataPtr, count, label) {
  expectStatus(Module._dvz_wasm_api_visual_set_rgba8(visual, attrPtr, dataPtr, count), 0, label);
}

function setCapabilities(
  Module,
  scene,
  maxTextureDimension2d,
  maxBindGroups,
  maxVertexBuffers,
  maxBufferSize,
  minTextureCopyBytesPerRowAlignment,
  maxSampleCount,
  label,
) {
  expectStatus(
    Module._dvz_wasm_api_set_capabilities(
      scene,
      maxTextureDimension2d,
      maxBindGroups,
      maxVertexBuffers,
      maxBufferSize,
      minTextureCopyBytesPerRowAlignment,
      maxSampleCount,
    ),
    0,
    label,
  );
}

const { default: createModule } = await import(pathToFileURL(modulePath).href);
const Module = await createModule({ locateFile: (path) => join(dirname(modulePath), path) });
const smokeSize = 64;

await expectBrowserWrapperPacketRuntime();
expectNoLegacyDirectAbi(Module);

const positionNamePtr = allocCString(Module, "position");
const colorNamePtr = allocCString(Module, "color");
const diameterNamePtr = allocCString(Module, "diameter");
const normalNamePtr = allocCString(Module, "normal");
const texcoordsNamePtr = allocCString(Module, "texcoords");

try {
  const diagnosticScene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(diagnosticScene !== 0, "diagnostic scene creation failed");
  try {
    expectStatus(
      Module._dvz_wasm_api_set_canvas_format(diagnosticScene, 9999),
      -1,
      "unsupported canvas format",
    );
    expectDiagnostics(Module, diagnosticScene, "unsupported WASM canvas format", "unsupported format");
    expectStatus(
      Module._dvz_wasm_api_set_canvas_format(diagnosticScene, DVZ_FORMAT_R8G8B8A8_UNORM),
      0,
      "reset canvas format",
    );
    expectNoDiagnostics(Module, diagnosticScene, "successful format reset");
    expectStatus(
      Module._dvz_wasm_api_set_capabilities(diagnosticScene, 0, 4, 8, 1024, 256, 1),
      -1,
      "invalid capabilities",
    );
    expectDiagnostics(
      Module, diagnosticScene, "invalid WASM capability snapshot", "invalid capabilities");
    setCapabilities(Module, diagnosticScene, 4096, 4, 8, 1024 * 1024, 256, 1, "reset capabilities");
    expectNoDiagnostics(Module, diagnosticScene, "successful capability reset");

    expectStatus(Module._dvz_wasm_api_emit(diagnosticScene, 0), -1, "invalid emit");
    expectDiagnostics(Module, diagnosticScene, "invalid WASM emit request", "invalid emit");

    expectStatus(Module._dvz_wasm_api_emit_packets(diagnosticScene, 0), -1, "invalid packet emit");
    expectStatus(Module._dvz_wasm_api_packet_status(diagnosticScene), -1, "invalid packet status");
    expectDiagnostics(
      Module, diagnosticScene, "invalid WASM packet emit request", "invalid packet emit");

    const badVisual = Module._dvz_wasm_api_visual(diagnosticScene, 9999, 0);
    requireOk(badVisual === 0, "unsupported visual type unexpectedly succeeded");
    expectDiagnostics(Module, diagnosticScene, "unsupported WASM visual type", "unsupported visual");

    const pointForDiagnostics = Module._dvz_wasm_api_visual(diagnosticScene, DVZ_WASM_VISUAL_POINT, 0);
    requireOk(pointForDiagnostics !== 0, "diagnostic point creation failed");
    expectNoDiagnostics(Module, diagnosticScene, "successful visual creation clears diagnostics");

    const badAttrNamePtr = allocCString(Module, "not_an_attr");
    const onePositionPtr = allocArray(Module, new Float32Array([0, 0, 0]));
    try {
      expectStatus(
        Module._dvz_wasm_api_visual_set_f32(pointForDiagnostics, badAttrNamePtr, onePositionPtr, 1),
        -1,
        "invalid visual attribute",
      );
      expectDiagnostics(Module, diagnosticScene, "WASM f32 visual upload failed", "invalid attr");
    } finally {
      Module._free(badAttrNamePtr);
      Module._free(onePositionPtr);
    }

    expectStatus(
      Module._dvz_wasm_api_resize(diagnosticScene, 0, smokeSize, smokeSize, 1),
      -1,
      "invalid resize",
    );
    expectDiagnostics(Module, diagnosticScene, "invalid WASM resize request", "invalid resize");

    const badController = Module._dvz_wasm_api_controller(diagnosticScene, 9999);
    requireOk(badController === 0, "unsupported controller type unexpectedly succeeded");
    expectDiagnostics(
      Module, diagnosticScene, "unsupported WASM controller type", "unsupported controller");

    const panzoomForDiagnostics =
      Module._dvz_wasm_api_controller(diagnosticScene, DVZ_CONTROLLER_TYPE_PANZOOM);
    requireOk(panzoomForDiagnostics !== 0, "diagnostic panzoom creation failed");
    expectNoDiagnostics(Module, diagnosticScene, "successful controller creation clears diagnostics");
    expectStatus(
      Module._dvz_wasm_api_arcball_initial(panzoomForDiagnostics, 0, 0, 0),
      -1,
      "arcball initial on panzoom",
    );
    expectDiagnostics(
      Module, diagnosticScene, "WASM controller is not an arcball", "arcball on panzoom");

    const diagnosticFigure = Module._dvz_wasm_api_figure(diagnosticScene, smokeSize, smokeSize);
    const diagnosticPanel = Module._dvz_wasm_api_panel_full(diagnosticFigure);
    requireOk(diagnosticFigure !== 0 && diagnosticPanel !== 0, "diagnostic figure/panel failed");
    expectStatus(
      Module._dvz_wasm_api_panel_add_visual(diagnosticPanel, 0),
      -1,
      "null visual handle",
    );
    expectDiagnostics(Module, diagnosticScene, "invalid WASM panel/visual handle", "null visual");

    const otherScene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
    requireOk(otherScene !== 0, "cross-scene diagnostic scene creation failed");
    try {
      const otherPoint = Module._dvz_wasm_api_visual(otherScene, DVZ_WASM_VISUAL_POINT, 0);
      requireOk(otherPoint !== 0, "cross-scene diagnostic point creation failed");
      expectStatus(
        Module._dvz_wasm_api_panel_add_visual(diagnosticPanel, otherPoint),
        -1,
        "cross-scene visual handle",
      );
      expectDiagnostics(
        Module, diagnosticScene, "invalid WASM panel/visual handle", "cross-scene visual");
    } finally {
      Module._dvz_wasm_api_scene_destroy(otherScene);
    }
  } finally {
    Module._dvz_wasm_api_scene_destroy(diagnosticScene);
  }

  const positions = new Float32Array([-0.75, -0.45, 0, -0.35, 0.35, 0, 0.05, -0.1, 0, 0.42, 0.5, 0, 0.72, -0.35, 0]);
  const colors = new Uint8Array([231, 77, 60, 255, 46, 204, 113, 255, 52, 152, 219, 255, 241, 196, 15, 255, 155, 89, 182, 255]);
  const sizes = new Float32Array([32, 44, 36, 48, 40]);
  const primitivePositions = new Float32Array([-0.85, -0.7, 0.15, -0.15, -0.7, 0.15, -0.5, 0.1, 0.15]);
  const primitiveColors = new Uint8Array([255, 120, 90, 220, 255, 180, 90, 220, 255, 90, 150, 220]);
  const imageWidth = 8;
  const imageHeight = 8;
  const imagePixels = new Uint8Array(imageWidth * imageHeight * 4);
  for (let y = 0; y < imageHeight; y++) {
    for (let x = 0; x < imageWidth; x++) {
      const i = (y * imageWidth + x) * 4;
      const checker = (x + y) % 2;
      imagePixels[i + 0] = checker ? 60 : 235;
      imagePixels[i + 1] = checker ? 125 : 245;
      imagePixels[i + 2] = checker ? 210 : 120;
      imagePixels[i + 3] = 255;
    }
  }
  const imagePositions = new Float32Array([0.18, -0.78, 0.05, 0.18, -0.12, 0.05, 0.86, -0.78, 0.05, 0.86, -0.12, 0.05]);
  const imageTexcoords = new Float32Array([0, 0, 0, 1, 1, 0, 1, 1]);
  const meshPositions = new Float32Array([0.18, 0.18, 0.22, 0.86, 0.18, 0.22, 0.18, 0.78, 0.22, 0.86, 0.18, 0.22, 0.86, 0.78, 0.22, 0.18, 0.78, 0.22]);
  const meshColors = new Uint8Array([90, 170, 255, 240, 85, 230, 190, 240, 160, 120, 255, 240, 85, 230, 190, 240, 255, 135, 210, 240, 160, 120, 255, 240]);
  const meshNormals = new Float32Array([0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1]);

  const scene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(scene !== 0, "dvz_wasm_api_scene failed");
  const ptrs = [
    allocArray(Module, positions), allocArray(Module, colors), allocArray(Module, sizes),
    allocArray(Module, primitivePositions), allocArray(Module, primitiveColors),
    allocArray(Module, imagePositions), allocArray(Module, imageTexcoords), allocArray(Module, imagePixels),
    allocArray(Module, meshPositions), allocArray(Module, meshColors), allocArray(Module, meshNormals),
  ];
  try {
    expectStatus(Module._dvz_wasm_api_set_canvas_format(scene, DVZ_FORMAT_R8G8B8A8_UNORM), 0, "api 2D canvas format");
    const figure = Module._dvz_wasm_api_figure(scene, smokeSize, smokeSize);
    const panel = Module._dvz_wasm_api_panel_full(figure);
    requireOk(figure !== 0 && panel !== 0, "api 2D figure/panel failed");

    const point = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_POINT, 0);
    setF32(Module, point, positionNamePtr, ptrs[0], positions.length / 3, "api point position");
    setRGBA8(Module, point, colorNamePtr, ptrs[1], colors.length / 4, "api point color");
    setF32(Module, point, diameterNamePtr, ptrs[2], sizes.length, "api point diameter");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, point), 0, "api add point");

    const primitive = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_PRIMITIVE, 0);
    setF32(Module, primitive, positionNamePtr, ptrs[3], primitivePositions.length / 3, "api primitive position");
    setRGBA8(Module, primitive, colorNamePtr, ptrs[4], primitiveColors.length / 4, "api primitive color");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, primitive), 0, "api add primitive");

    const image = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_IMAGE, 0);
    setF32(Module, image, positionNamePtr, ptrs[5], imagePositions.length / 3, "api image position");
    setF32(Module, image, texcoordsNamePtr, ptrs[6], imageTexcoords.length / 2, "api image texcoords");
    expectStatus(Module._dvz_wasm_api_visual_set_texture_rgba8(image, ptrs[7], imageWidth, imageHeight), 0, "api image texture");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, image), 0, "api add image");

    const mesh = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_MESH, 0);
    setF32(Module, mesh, positionNamePtr, ptrs[8], meshPositions.length / 3, "api mesh position");
    setRGBA8(Module, mesh, colorNamePtr, ptrs[9], meshColors.length / 4, "api mesh color");
    setF32(Module, mesh, normalNamePtr, ptrs[10], meshNormals.length / 3, "api mesh normal");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, mesh), 0, "api add mesh");

    const panzoom = Module._dvz_wasm_api_controller(scene, DVZ_CONTROLLER_TYPE_PANZOOM);
    expectStatus(Module._dvz_wasm_api_panel_bind_controller(panel, panzoom, DVZ_DIM_MASK_XY), 0, "api bind panzoom");
    setCapabilities(Module, scene, 4096, 4, 8, 128, 256, 1, "restrict buffer capability");
    expectStatus(Module._dvz_wasm_api_emit(scene, figure), -1, "buffer capability emit rejection");
    expectAnyDiagnostics(Module, scene, "buffer capability emit rejection");
    setCapabilities(Module, scene, 4096, 4, 8, 256 * 1024 * 1024, 256, 1, "restore buffer capability");
    const initial = emitStream(Module, scene, figure, "generic 2D initial");
    expect2DSceneStreamShape(initial.stream, "generic 2D initial");
    const updatedColors = new Uint8Array(colors);
    updatedColors[0] = 32;
    updatedColors[1] = 220;
    updatedColors[2] = 180;
    const updatedColorsPtr = allocArray(Module, updatedColors);
    try {
      setRGBA8(
        Module, point, colorNamePtr, updatedColorsPtr, updatedColors.length / 4,
        "api point color update");
      const visualUpdate = emitStream(Module, scene, figure, "generic 2D visual update");
      expect2DUpdateStreamShape(visualUpdate.stream, "generic 2D visual update");
      expectWriteCommands(visualUpdate.stream, "generic 2D visual update");
    } finally {
      Module._free(updatedColorsPtr);
    }
    const grownPositionPtr = allocArray(Module, new Float32Array([...positions, -0.1, 0.75, 0]));
    try {
      expectStatus(
        Module._dvz_wasm_api_visual_set_f32(
          point, positionNamePtr, grownPositionPtr, positions.length / 3 + 1),
        -1,
        "api point count growth",
      );
    expectDiagnostics(
      Module, scene, "WASM f32 visual upload failed: attr=position item_count=6",
      "point count growth");
    } finally {
      Module._free(grownPositionPtr);
    }
    const largerImageWidth = 16;
    const largerImageHeight = 16;
    const largerImagePixels = new Uint8Array(largerImageWidth * largerImageHeight * 4);
    for (let i = 0; i < largerImagePixels.length; i += 4) {
      largerImagePixels[i + 0] = 80;
      largerImagePixels[i + 1] = 180;
      largerImagePixels[i + 2] = 240;
      largerImagePixels[i + 3] = 255;
    }
    const largerImagePtr = allocArray(Module, largerImagePixels);
    try {
      expectStatus(
        Module._dvz_wasm_api_visual_set_texture_rgba8(
          image, largerImagePtr, largerImageWidth, largerImageHeight),
        0,
        "api image texture resize",
      );
      const visualReload = emitStream(Module, scene, figure, "generic 2D visual reload");
      expectFrameCommandShape(visualReload.stream, "generic 2D visual reload");
      expectSetupCommands(visualReload.stream, "generic 2D visual reload");
      expectDraw(visualReload.stream, 6, 5, "generic 2D visual reload point");
      expectDraw(visualReload.stream, 3, 1, "generic 2D visual reload primitive");
      expectDraw(visualReload.stream, 4, 1, "generic 2D visual reload image");
      expectDraw(visualReload.stream, 6, 1, "generic 2D visual reload mesh");
      expectStatus(
        Module._dvz_wasm_api_visual_set_texture_rgba8(image, ptrs[7], imageWidth, imageHeight),
        0,
        "api image texture restore for split packet",
      );
      const splitReload = emitPacketStream(Module, scene, figure, "generic 2D split packet reload");
      requireOk(
        splitReload.setup.commandCount > 0 &&
          splitReload.update.commandCount > 0 &&
          splitReload.frame.commandCount > 0,
        "generic 2D split packet reload missing commands",
      );
      expectNoPayload(Module, scene, "split packet emit invalidates JSON payload");
    } finally {
      Module._free(largerImagePtr);
    }
    expectStatus(Module._dvz_wasm_api_pointer(scene, DVZ_POINTER_EVENT_PRESS, 32, 32, DVZ_POINTER_BUTTON_LEFT, 0, 1, 200), 0, "api pointer press");
    expectNoPayload(Module, scene, "pointer press invalidates 2D payload");
    expectStatus(Module._dvz_wasm_api_pointer(scene, DVZ_POINTER_EVENT_MOVE, 38, 30, DVZ_POINTER_BUTTON_LEFT, 0, 1, 216), 0, "api pointer move");
    expectStatus(Module._dvz_wasm_api_pointer(scene, DVZ_POINTER_EVENT_RELEASE, 38, 30, DVZ_POINTER_BUTTON_LEFT, 0, 1, 232), 0, "api pointer release");
    const interactive = emitStream(Module, scene, figure, "generic 2D interactive");
    expect2DUpdateStreamShape(interactive.stream, "generic 2D interactive");
    requireOk(
      interactive.payload !== initial.payload && interactive.size !== initial.size,
      "generic 2D interactive payload did not replace initial payload",
    );
    expectStatus(Module._dvz_wasm_api_resize(scene, figure, smokeSize * 2, smokeSize + 8, 2), 0, "api resize");
    expectNoPayload(Module, scene, "resize invalidates 2D payload");
    const resized = emitStream(Module, scene, figure, "generic 2D resized");
    expect2DUpdateStreamShape(resized.stream, "generic 2D resized");
    requireOk(
      resized.payload !== interactive.payload,
      "generic 2D resized payload did not replace interactive payload",
    );
    await mkdir(dirname(output2dPath), { recursive: true });
    await writeFile(output2dPath, `${JSON.stringify(initial.stream, null, 2)}\n`, "utf8");
    console.log(`Wrote ${output2dPath}`);
    console.log(`commands_api2d=initial:${initial.stream.commands.length} interactive:${interactive.stream.commands.length} resize:${resized.stream.commands.length}`);
  } finally {
    ptrs.forEach((ptr) => Module._free(ptr));
    Module._dvz_wasm_api_scene_destroy(scene);
  }

  const cube = makeCubeMesh(1.25);
  const scene3d = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(scene3d !== 0, "dvz_wasm_api_scene 3D failed");
  const cubePtrs = [allocArray(Module, cube.positions), allocArray(Module, cube.colors), allocArray(Module, cube.normals)];
  try {
    expectStatus(Module._dvz_wasm_api_set_canvas_format(scene3d, DVZ_FORMAT_R8G8B8A8_UNORM), 0, "api 3D canvas format");
    const figure3d = Module._dvz_wasm_api_figure(scene3d, smokeSize, smokeSize);
    const panel3d = Module._dvz_wasm_api_panel_full(figure3d);
    requireOk(figure3d !== 0 && panel3d !== 0, "api 3D figure/panel failed");
    expectStatus(Module._dvz_wasm_api_panel_set_camera(panel3d, 0, 0, 3, 0, 0, 0, Math.PI / 4, 0.1, 100), 0, "api 3D camera");
    const mesh3d = Module._dvz_wasm_api_visual(scene3d, DVZ_WASM_VISUAL_MESH, 0);
    setF32(Module, mesh3d, positionNamePtr, cubePtrs[0], cube.positions.length / 3, "api 3D mesh position");
    setRGBA8(Module, mesh3d, colorNamePtr, cubePtrs[1], cube.colors.length / 4, "api 3D mesh color");
    setF32(Module, mesh3d, normalNamePtr, cubePtrs[2], cube.normals.length / 3, "api 3D mesh normal");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel3d, mesh3d), 0, "api 3D add mesh");
    const arcball = Module._dvz_wasm_api_controller(scene3d, DVZ_CONTROLLER_TYPE_ARCBALL);
    expectStatus(Module._dvz_wasm_api_panel_bind_controller(panel3d, arcball, DVZ_DIM_MASK_XYZ), 0, "api bind arcball");
    expectStatus(Module._dvz_wasm_api_arcball_initial(arcball, 0.45, -0.65, 0.2), 0, "api arcball initial");
    const initial3d = emitStream(Module, scene3d, figure3d, "generic 3D initial");
    expect3DSceneStreamShape(initial3d.stream, "generic 3D initial");
    expectStatus(Module._dvz_wasm_api_pointer(scene3d, DVZ_POINTER_EVENT_PRESS, 32, 32, DVZ_POINTER_BUTTON_LEFT, 0, 1, 300), 0, "api 3D pointer press");
    expectNoPayload(Module, scene3d, "pointer press invalidates 3D payload");
    expectStatus(Module._dvz_wasm_api_pointer(scene3d, DVZ_POINTER_EVENT_MOVE, 40, 38, DVZ_POINTER_BUTTON_LEFT, 0, 1, 316), 0, "api 3D pointer move");
    expectStatus(Module._dvz_wasm_api_pointer(scene3d, DVZ_POINTER_EVENT_RELEASE, 40, 38, DVZ_POINTER_BUTTON_LEFT, 0, 1, 332), 0, "api 3D pointer release");
    const interactive3d = emitStream(Module, scene3d, figure3d, "generic 3D interactive");
    expect3DUpdateStreamShape(interactive3d.stream, "generic 3D interactive");
    requireOk(
      interactive3d.payload !== initial3d.payload && interactive3d.size !== initial3d.size,
      "generic 3D interactive payload did not replace initial payload",
    );
    await writeFile(output3dPath, `${JSON.stringify(initial3d.stream, null, 2)}\n`, "utf8");
    console.log(`Wrote ${output3dPath}`);
    console.log(`commands_api3d=initial:${initial3d.stream.commands.length} interactive:${interactive3d.stream.commands.length}`);
  } finally {
    cubePtrs.forEach((ptr) => Module._free(ptr));
    Module._dvz_wasm_api_scene_destroy(scene3d);
  }
} finally {
  Module._free(positionNamePtr);
  Module._free(colorNamePtr);
  Module._free(diameterNamePtr);
  Module._free(normalNamePtr);
  Module._free(texcoordsNamePtr);
}
