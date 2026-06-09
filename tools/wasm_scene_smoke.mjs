#!/usr/bin/env node

import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { decodeDrp2Packet } from "../web/drp2/packet.js";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const modulePath = resolve(root, "build-wasm-scene/wasm/datoviz_wasm_scene.mjs");
const browserWrapperPath = resolve(root, "web/wasm/scene.js");
const output2dPath = resolve(root, "build-wasm-scene/wasm/wasm_api_scene_point_pixel_marker_segment_path_primitive_image_mesh_panzoom.json");
const output3dPath = resolve(root, "build-wasm-scene/wasm/wasm_api_scene_sphere_textured_mesh3d_arcball.json");
const outputScenarioPath = resolve(root, "build-wasm-scene/wasm/wasm_api_scenario_timer_animation.json");

const DVZ_POINTER_EVENT_PRESS = 1;
const DVZ_POINTER_EVENT_RELEASE = 0;
const DVZ_POINTER_EVENT_MOVE = 2;
const DVZ_POINTER_BUTTON_LEFT = 1;
const DVZ_FORMAT_R8G8B8A8_UNORM = 37;
const DVZ_CONTROLLER_TYPE_PANZOOM = 1;
const DVZ_CONTROLLER_TYPE_ARCBALL = 2;
const DVZ_DIM_X = 0;
const DVZ_DIM_Y = 1;
const DVZ_DIM_MASK_XY = 3;
const DVZ_DIM_MASK_XYZ = 7;
const DVZ_WASM_VISUAL_POINT = 1;
const DVZ_WASM_VISUAL_PIXEL = 2;
const DVZ_WASM_VISUAL_MARKER = 3;
const DVZ_WASM_VISUAL_SEGMENT = 4;
const DVZ_WASM_VISUAL_PATH = 5;
const DVZ_WASM_VISUAL_IMAGE = 6;
const DVZ_WASM_VISUAL_MESH = 7;
const DVZ_WASM_VISUAL_GLYPH = 8;
const DVZ_WASM_VISUAL_PRIMITIVE = 9;
const DVZ_WASM_VISUAL_SPHERE = 10;
const DVZ_WASM_VISUAL_TEXT = 11;
const DVZ_WASM_VISUAL_LABELS = 12;
const DVZ_MATERIAL_MODEL_STANDARD = 2;
const DVZ_SEGMENT_CAP_ROUND = 1;
const DVZ_SEGMENT_CAP_TRIANGLE_OUT = 3;
const DVZ_SEGMENT_CAP_SQUARE = 4;
const DVZ_SEGMENT_CAP_BUTT = 5;
const DVZ_PATH_JOIN_MITER = 0;
const DVZ_PATH_JOIN_BEVEL = 2;
const DVZ_SCENE_BUFFER_USAGE_VERTEX = 1;
const DVZ_DRP2_PACKET_SETUP = 1;
const DVZ_DRP2_PACKET_UPDATE = 2;
const DVZ_DRP2_PACKET_FRAME = 3;

function requireOk(condition, message) {
  if (!condition) throw new Error(message);
}

function expectStatus(status, expected, label) {
  requireOk(status === expected, `${label} returned ${status}, expected ${expected}`);
}

const stderrCaptures = [];

function wasmPrintErr(text) {
  const activeCapture = stderrCaptures[stderrCaptures.length - 1];
  if (activeCapture !== undefined) {
    activeCapture.push(String(text));
    return;
  }
  console.error(text);
}

function captureExpectedStderr(label, fn) {
  const captured = [];
  stderrCaptures.push(captured);
  try {
    fn();
  } catch (error) {
    if (captured.length > 0) {
      error.message = `${error.message}\n${label}: captured stderr: ${captured.join("; ")}`;
    }
    throw error;
  } finally {
    stderrCaptures.pop();
  }
  return captured;
}

function expectCapturedStderr(captured, needle, label) {
  requireOk(captured.length > 0, `${label}: expected captured stderr`);
  requireOk(
    captured.some((line) => line.includes(needle)),
    `${label}: expected stderr containing ${needle}, got ${captured.join("; ")}`,
  );
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

function allocCStringArray(Module, values) {
  requireOk(values.length > 0, "empty string arrays are not supported");
  const stringPtrs = values.map((value) => allocCString(Module, value));
  const ptr = Module._malloc(values.length * 4);
  if (ptr === 0) {
    for (const stringPtr of stringPtrs) Module._free(stringPtr);
    throw new Error("malloc failed");
  }
  Module.HEAPU32.set(stringPtrs, ptr / 4);
  return { ptr, stringPtrs };
}

function freeCStringArray(Module, array) {
  Module._free(array.ptr);
  for (const ptr of array.stringPtrs) Module._free(ptr);
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
    requireOk(typeof shader.stage === "string" && shader.stage.length > 0, `${label}: empty shader stage`);
    requireOk(typeof shader.format === "string" && shader.format.length > 0, `${label}: empty shader format`);
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
  const resourceVersion = Module._dvz_wasm_api_resource_version(scene);
  const frameIndex = Module._dvz_wasm_api_frame_index(scene);
  requireOk(resourceVersion > 0, `${label}: missing JSON artifact resource version`);
  requireOk(frameIndex > 0, `${label}: missing JSON artifact frame index`);
  requireOk(Module._dvz_wasm_api_packet_status(scene) === 0, `${label}: JSON artifact packet status not OK`);
  return { stream, payload, ptr, size, resourceVersion, frameIndex };
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
  const resourceVersion = Module._dvz_wasm_api_resource_version(scene);
  const frameIndex = Module._dvz_wasm_api_frame_index(scene);
  requireOk(resourceVersion > 0, `${label}: missing resource version`);
  requireOk(frameIndex > 0, `${label}: missing frame index`);

  const packetStream = {
    resourceVersion,
    frameIndex,
    setup: expectPacket(Module, scene, DVZ_DRP2_PACKET_SETUP, `${label} setup`),
    update: expectPacket(Module, scene, DVZ_DRP2_PACKET_UPDATE, `${label} update`, { expectArena: true }),
    frame: expectPacket(Module, scene, DVZ_DRP2_PACKET_FRAME, `${label} frame`),
  };
  for (const [kind, packet] of Object.entries(packetStream)) {
    if (kind === "resourceVersion" || kind === "frameIndex") continue;
    requireOk(packet.decoded.resource_version === resourceVersion, `${label} ${kind}: resource counter mismatch`);
    requireOk(packet.decoded.frame_index === frameIndex, `${label} ${kind}: frame counter mismatch`);
  }
  return packetStream;
}

function expectReleasedPackets(Module, scene, label, resourceVersion, frameIndex) {
  expectStatus(Module._dvz_wasm_api_release_packets(scene), 0, `${label}: release frame artifact`);
  requireOk(Module._dvz_wasm_api_packet_status(scene) === 0, `${label}: released packet status not OK`);
  for (const kind of [DVZ_DRP2_PACKET_SETUP, DVZ_DRP2_PACKET_UPDATE, DVZ_DRP2_PACKET_FRAME]) {
    requireOk(Module._dvz_wasm_api_packet_ptr(scene, kind) === 0, `${label}: packet ${kind} ptr survived release`);
    requireOk(Module._dvz_wasm_api_packet_size(scene, kind) === 0, `${label}: packet ${kind} size survived release`);
    requireOk(Module._dvz_wasm_api_packet_arena_ptr(scene, kind) === 0, `${label}: packet ${kind} arena ptr survived release`);
    requireOk(Module._dvz_wasm_api_packet_arena_size(scene, kind) === 0, `${label}: packet ${kind} arena size survived release`);
  }
  requireOk(
    Module._dvz_wasm_api_resource_version(scene) === resourceVersion,
    `${label}: resource counter changed on release`,
  );
  requireOk(
    Module._dvz_wasm_api_frame_index(scene) === frameIndex,
    `${label}: frame counter changed on release`,
  );
}

function emitIncrementalPacketStream(Module, scene, figure, label) {
  requireOk(typeof Module._dvz_wasm_api_emit_packets === "function", `${label}: missing packet emit ABI`);

  const status = Module._dvz_wasm_api_emit_packets(scene, figure);
  const messages = diagnostics(Module, scene);
  if (status !== 0) {
    requireOk(messages.length > 0, `${label}: no diagnostic was reported`);
    throw new Error(`${label}: ${messages.join("; ")}`);
  }
  requireOk(Module._dvz_wasm_api_packet_status(scene) === 0, `${label}: packet status not OK`);
  requireOk(messages.length === 0, `${label}: diagnostics: ${messages.join("; ")}`);

  return {
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
  requireOk(source.includes("_dvz_wasm_api_packet_status"), "browser wrapper ignores packet status");
  requireOk(source.includes("_dvz_wasm_api_buffer"), "browser wrapper cannot create scene buffers");
  requireOk(source.includes("_dvz_wasm_api_buffer_set_data"), "browser wrapper cannot upload scene buffers");
  requireOk(source.includes("createScenario"), "browser wrapper cannot create portable scenarios");
  requireOk(source.includes("_dvz_wasm_api_scenario_create"), "browser wrapper cannot instantiate C scenarios");
  requireOk(source.includes("_dvz_wasm_api_scenario_frame"), "browser wrapper cannot advance C scenarios");
  requireOk(source.includes("_dvz_wasm_api_scenario_post_frame"), "browser wrapper cannot run scenario post-frame hooks");
  requireOk(source.includes("_dvz_wasm_api_scenario_pointer"), "browser wrapper cannot route scenario pointer events");
  requireOk(source.includes("_dvz_wasm_api_scenario_wheel"), "browser wrapper cannot route scenario wheel events");
  requireOk(source.includes("_dvz_wasm_api_emit_query_packets"), "browser wrapper cannot emit scenario query packets");
  requireOk(source.includes("_dvz_wasm_api_query_resolve"), "browser wrapper cannot resolve scenario query readbacks");
  requireOk(source.includes("_dvz_wasm_api_visual_set_attr_buffer"), "browser wrapper cannot bind attr buffers");
  requireOk(source.includes("_dvz_wasm_api_visual_set_u32"), "browser wrapper cannot upload u32 attrs");
  requireOk(source.includes("_dvz_wasm_api_visual_set_strings"), "browser wrapper cannot upload text strings");
  requireOk(source.includes("_dvz_wasm_api_visual_set_labels_s32"), "browser wrapper cannot upload S32 labels");
  requireOk(source.includes("_dvz_wasm_api_panel_set_domain"), "browser wrapper cannot set panel domains");
  requireOk(source.includes("_dvz_wasm_api_panel_axis"), "browser wrapper cannot create panel axes");
  requireOk(source.includes("_dvz_wasm_api_axis_set_grid"), "browser wrapper cannot enable axis grids");
  requireOk(source.includes("_dvz_wasm_api_axis_set_label"), "browser wrapper cannot set axis labels");
  requireOk(source.includes("_dvz_wasm_api_visual_set_material"), "browser wrapper cannot set materials");
  requireOk(source.includes("_dvz_wasm_api_visual_set_segment_caps"), "browser wrapper cannot set segment caps");
  requireOk(source.includes("_dvz_wasm_api_visual_set_path_caps"), "browser wrapper cannot set path caps");
  requireOk(source.includes("_dvz_wasm_api_visual_set_path_join"), "browser wrapper cannot set path joins");
  requireOk(source.includes(".slice()"), "browser wrapper retains borrowed WASM packet views");
  requireOk(source.includes("wasm_frame_artifact"), "browser wrapper does not tag frame artifact packet sets");
  requireOk(source.includes("artifact_spans_copied: true"), "browser wrapper does not mark copied artifact spans");
  requireOk(source.includes("artifact_released = true"), "browser wrapper does not release frame artifacts");
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

function scenarioIndex(Module, id) {
  const count = Module._dvz_wasm_api_scenario_count();
  for (let i = 0; i < count; i++) {
    const ptr = Module._dvz_wasm_api_scenario_id(i);
    if (ptr !== 0 && Module.UTF8ToString(ptr) === id) return i;
  }
  throw new Error(`missing WASM scenario ${id}`);
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

function expectCanvasRenderPass(stream, label, expectedCount = 1) {
  const passes = commandsOf(stream, "BeginRenderPass");
  requireOk(
    passes.length === expectedCount,
    `${label}: expected ${expectedCount} render pass(es), got ${passes.length}`,
  );
  for (const pass of passes) {
    requireOk(
      Array.isArray(pass.color_attachments) && pass.color_attachments.length === 1,
      `${label}: expected one color attachment`,
    );
    requireOk(
      pass.color_attachments[0].texture_id === 0,
      `${label}: expected browser canvas color target texture_id 0`,
    );
  }
  expectCommandCount(stream, "SetViewport", expectedCount, label);
  expectCommandCount(stream, "SetScissor", expectedCount, label);
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

function expectDrawIndexed(stream, indexCount, instanceCount, label) {
  const found = commandsOf(stream, "DrawIndexed").some(
    (draw) => draw.index_count === indexCount && draw.instance_count === instanceCount,
  );
  requireOk(
    found,
    `${label}: missing DrawIndexed index_count=${indexCount} instance_count=${instanceCount}`,
  );
}

function expectFrameCommandShape(stream, label, expectedRenderPassCount = 1) {
  expectAllShadersWgsl(stream, label);
  expectPipelineMetadata(stream, label);
  expectCanvasRenderPass(stream, label, expectedRenderPassCount);
  expectCommandCount(stream, "BeginCommandEncoder", 1, label);
  expectCommandCount(stream, "EndRenderPass", expectedRenderPassCount, label);
  expectCommandCount(stream, "FinishCommandEncoder", 1, label);
  expectCommandCount(stream, "QueueSubmit", 1, label);
}

function expectCommonSceneStreamShape(stream, label, expectedRenderPassCount = 1) {
  expectFrameCommandShape(stream, label, expectedRenderPassCount);
  expectCommandCount(stream, "HelloRenderer", 1, label);
  expectCommandCount(stream, "RendererHelloReply", 1, label);
}

function expect2DSceneStreamShape(stream, label) {
  expectCommonSceneStreamShape(stream, label, 2);
  expectDraw(stream, 6, 5, `${label} point`);
  expectDraw(stream, 6, 6, `${label} pixel`);
  expectDraw(stream, 6, 4, `${label} marker`);
  expectDrawIndexed(stream, 18, 1, `${label} segment`);
  expectDrawIndexed(stream, 24, 1, `${label} path`);
  expectDraw(stream, 3, 1, `${label} primitive`);
  expectDraw(stream, 4, 1, `${label} image`);
  expectDraw(stream, 6, 1, `${label} labels`);
  expectDraw(stream, 18, 1, `${label} glyph`);
  expectDraw(stream, 24, 1, `${label} text`);
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
  const pixelPipeline = expectPipeline(
    stream,
    `${label} pixel`,
    (pipeline) => pipeline.builtin_pipeline === "scene.pixel",
  );
  requireOk(pixelPipeline.topology === "triangle-list", `${label} pixel: unexpected topology`);
  requireOk(
    JSON.stringify(pipelineAttributeFormats(pixelPipeline)) ===
      JSON.stringify([
        { stepMode: "instance", formats: ["float32x3"] },
        { stepMode: "instance", formats: ["unorm8x4"] },
        { stepMode: "instance", formats: ["float32"] },
      ]),
    `${label} pixel: unexpected vertex attributes`,
  );
  expectDepthPipeline(pixelPipeline, `${label} pixel`);
  const markerPipeline = expectPipeline(
    stream,
    `${label} marker`,
    (pipeline) => pipeline.builtin_pipeline === "scene.marker",
  );
  requireOk(markerPipeline.topology === "triangle-list", `${label} marker: unexpected topology`);
  requireOk(
    JSON.stringify(pipelineAttributeFormats(markerPipeline)) ===
      JSON.stringify([
        { stepMode: "instance", formats: ["float32x3"] },
        { stepMode: "instance", formats: ["unorm8x4"] },
        { stepMode: "instance", formats: ["float32"] },
        { stepMode: "instance", formats: ["float32"] },
        { stepMode: "instance", formats: ["uint32"] },
      ]),
    `${label} marker: unexpected vertex attributes`,
  );
  expectDepthPipeline(markerPipeline, `${label} marker`);
  const segmentPipeline = expectPipeline(
    stream,
    `${label} segment`,
    (pipeline) => pipeline.builtin_pipeline === "scene.segment",
  );
  requireOk(segmentPipeline.topology === "triangle-list", `${label} segment: unexpected topology`);
  requireOk(
    JSON.stringify(pipelineAttributeFormats(segmentPipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["unorm8x4"] },
        { stepMode: "vertex", formats: ["float32"] },
      ]),
    `${label} segment: unexpected vertex attributes`,
  );
  const pathPipeline = expectPipeline(
    stream,
    `${label} path`,
    (pipeline) => pipeline.builtin_pipeline === "scene.path",
  );
  requireOk(pathPipeline.topology === "triangle-list", `${label} path: unexpected topology`);
  requireOk(
    JSON.stringify(pipelineAttributeFormats(pathPipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["unorm8x4"] },
        { stepMode: "vertex", formats: ["float32"] },
        { stepMode: "vertex", formats: ["uint32"] },
        { stepMode: "vertex", formats: ["float32"] },
      ]),
    `${label} path: unexpected vertex attributes`,
  );
  const primitivePipeline = expectPipeline(
    stream,
    `${label} primitive`,
    (pipeline) =>
      pipeline.builtin_pipeline === "scene.primitive" &&
      pipeline.vertex_buffer_slots === 2 &&
      pipeline.topology === "triangle-list" &&
      pipeline.depth_stencil?.depth_write_enabled === true,
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
  const axisPrimitivePipeline = expectPipeline(
    stream,
    `${label} axis primitive`,
    (pipeline) =>
      pipeline.builtin_pipeline === "scene.primitive" &&
      pipeline.vertex_buffer_slots === 2 &&
      pipeline.topology === "triangle-list" &&
      pipeline.depth_stencil?.depth_write_enabled === false,
  );
  expectDepthPipeline(axisPrimitivePipeline, `${label} axis primitive`, false, "always");
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
  const glyphPipeline = expectPipeline(
    stream,
    `${label} glyph`,
    (pipeline) => pipeline.builtin_pipeline === "scene.glyph",
  );
  requireOk(glyphPipeline.topology === "triangle-list", `${label} glyph: unexpected topology`);
  requireOk(
    JSON.stringify(pipelineAttributeFormats(glyphPipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["float32x4"] },
        { stepMode: "vertex", formats: ["float32x4"] },
        { stepMode: "vertex", formats: ["unorm8x4"] },
        { stepMode: "vertex", formats: ["float32"] },
      ]),
    `${label} glyph: unexpected vertex attributes`,
  );
  expectDepthPipeline(glyphPipeline, `${label} glyph`, false, "always");
  const labelsPipeline = expectPipeline(
    stream,
    `${label} labels`,
    (pipeline) => pipeline.builtin_pipeline === "scene.labels",
  );
  requireOk(labelsPipeline.topology === "triangle-list", `${label} labels: unexpected topology`);
  requireOk(
    JSON.stringify(pipelineAttributeFormats(labelsPipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["float32x2"] },
      ]),
    `${label} labels: unexpected vertex attributes`,
  );
  requireOk(labelsPipeline.depth_stencil === undefined, `${label} labels: unexpected depth state`);
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
  const imageTexture = textures.find((texture) => texture.width === 8 && texture.height === 8);
  const glyphTexture = textures.find((texture) => texture.width === 48 && texture.height === 16);
  const textTexture = textures.find((texture) => texture.width === 128 && texture.height === 60);
  const labelsTexture = commandsOf(stream, "CreateTexture").find(
    (texture) => texture.format === "r32sint" && texture.width === 6 && texture.height === 5,
  );
  requireOk(textures.length >= 3, `${label}: expected image, glyph, and text RGBA8 textures, got ${textures.length}`);
  requireOk(imageTexture !== undefined, `${label}: missing image texture`);
  requireOk(glyphTexture !== undefined, `${label}: missing glyph texture`);
  requireOk(textTexture !== undefined, `${label}: missing text atlas texture`);
  requireOk(labelsTexture !== undefined, `${label}: missing S32 labels texture`);
  for (const texture of [imageTexture, glyphTexture, textTexture]) {
    requireOk(
      texture.usage.includes("COPY_DST") && texture.usage.includes("TEXTURE_BINDING"),
      `${label}: texture ${texture.id} needs COPY_DST and TEXTURE_BINDING usage`,
    );
  }
  requireOk(writes.length >= 4, `${label}: expected image, labels, glyph, and text texture uploads, got ${writes.length}`);
  const imageWrite = writes.find((write) => write.texture_id === imageTexture.id);
  const glyphWrite = writes.find((write) => write.texture_id === glyphTexture.id);
  const textWrite = writes.find((write) => write.texture_id === textTexture.id);
  const labelsWrite = writes.find((write) => write.texture_id === labelsTexture.id);
  requireOk(
    imageWrite?.size?.width === 8 && imageWrite?.size?.height === 8,
    `${label}: image texture upload does not match texture resource`,
  );
  requireOk(
    glyphWrite?.size?.width === 48 && glyphWrite?.size?.height === 16,
    `${label}: glyph texture upload does not match texture resource`,
  );
  requireOk(
    textWrite?.size?.width === 128 && textWrite?.size?.height === 60,
    `${label}: text atlas upload does not match texture resource`,
  );
  requireOk(
    labelsWrite?.size?.width === 6 && labelsWrite?.size?.height === 5,
    `${label}: labels texture upload does not match texture resource`,
  );
  requireOk(imageWrite.bytes_per_row === 32, `${label}: unexpected image upload row pitch`);
  requireOk(labelsWrite.bytes_per_row === 24, `${label}: unexpected labels upload row pitch`);
  requireOk(glyphWrite.bytes_per_row === 192, `${label}: unexpected glyph upload row pitch`);
  requireOk(textWrite.bytes_per_row === 512, `${label}: unexpected text atlas upload row pitch`);
  requireOk(
    commandsOf(stream, "CreateRenderPipeline").length >= 11,
    `${label}: expected at least point, pixel, marker, segment, path, primitive, image, labels, glyph, text, and mesh pipelines`,
  );
}

function expect2DUpdateStreamShape(
  stream,
  label,
  pointInstances = 5,
  { allowSetupCommands = false } = {},
) {
  expectFrameCommandShape(stream, label, 2);
  if (!allowSetupCommands) {
    expectNoSetupCommands(stream, label);
  }
  expectDraw(stream, 6, pointInstances, `${label} point`);
  expectDraw(stream, 6, 6, `${label} pixel`);
  expectDraw(stream, 6, 4, `${label} marker`);
  expectDrawIndexed(stream, 18, 1, `${label} segment`);
  expectDrawIndexed(stream, 24, 1, `${label} path`);
  expectDraw(stream, 3, 1, `${label} primitive`);
  expectDraw(stream, 4, 1, `${label} image`);
  expectDraw(stream, 6, 1, `${label} labels`);
  expectDraw(stream, 18, 1, `${label} glyph`);
  expectDraw(stream, 24, 1, `${label} text`);
  expectDraw(stream, 6, 1, `${label} mesh`);
}

function expect3DSceneStreamShape(stream, label) {
  expectCommonSceneStreamShape(stream, label);
  expectDraw(stream, 6, 3, `${label} sphere`);
  expectDraw(stream, 36, 1, `${label} mesh`);
  const spherePipeline = expectPipeline(
    stream,
    `${label} sphere`,
    (pipeline) =>
      pipeline.builtin_pipeline === "scene.sphere" &&
      pipeline.vertex_buffer_slots === 3 &&
      pipeline.topology === "triangle-list",
  );
  requireOk(
    JSON.stringify(pipelineAttributeFormats(spherePipeline)) ===
      JSON.stringify([
        { stepMode: "instance", formats: ["float32x3"] },
        { stepMode: "instance", formats: ["unorm8x4"] },
        { stepMode: "instance", formats: ["float32"] },
      ]),
    `${label} sphere: unexpected vertex attributes`,
  );
  expectDepthPipeline(spherePipeline, `${label} sphere`);
  const meshPipeline = expectPipeline(
    stream,
    `${label} mesh`,
    (pipeline) =>
      pipeline.builtin_pipeline === "scene.mesh" &&
      pipeline.vertex_buffer_slots === 4 &&
      pipeline.topology === "triangle-list",
  );
  requireOk(
    JSON.stringify(pipelineAttributeFormats(meshPipeline)) ===
      JSON.stringify([
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["unorm8x4"] },
        { stepMode: "vertex", formats: ["float32x3"] },
        { stepMode: "vertex", formats: ["float32x2"] },
      ]),
    `${label} mesh: unexpected vertex attributes`,
  );
  expectDepthPipeline(meshPipeline, `${label} mesh`);
  requireOk(
    commandsOf(stream, "CreateBindGroup").some((command) =>
      command.entries?.some((entry) => entry.binding_type === "sampled_texture") &&
      command.entries?.some((entry) => entry.binding_type === "sampler")),
    `${label} mesh: missing sampled texture bind group`,
  );
  requireOk(
    commandsOf(stream, "WriteTexture").some(
      (command) => command.size?.width === 16 && command.size?.height === 16,
    ),
    `${label} mesh: missing texture upload`,
  );
  requireOk(
    commandsOf(stream, "CreateRenderPipeline").length === 2,
    `${label}: expected sphere and textured mesh pipelines`,
  );
}

function expect3DUpdateStreamShape(stream, label) {
  expectFrameCommandShape(stream, label);
  expectNoSetupCommands(stream, label);
  expectDraw(stream, 6, 3, `${label} sphere`);
  expectDraw(stream, 36, 1, `${label} mesh`);
}

function expectTimerScenarioStreamShape(stream, label, { allowSetupCommands = true } = {}) {
  expectFrameCommandShape(stream, label);
  if (!allowSetupCommands) {
    expectNoSetupCommands(stream, label);
  }
  expectDraw(stream, 6, 8, `${label} point`);
  if (!allowSetupCommands) {
    return;
  }
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
  requireOk(
    commandsOf(stream, "CreateRenderPipeline").filter(
      (pipeline) => pipeline.builtin_pipeline === "scene.point",
    ).length === 1,
    `${label}: expected one point pipeline`,
  );
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
  const texcoords = [];
  const faceUv = [[0, 0], [1, 0], [0, 1], [1, 0], [1, 1], [0, 1]];
  for (const face of faces) {
    for (let i = 0; i < face.v.length; i++) {
      const vertex = face.v[i];
      positions.push(...vertex);
      colors.push(255, 255, 255, 255);
      normals.push(...face.n);
      texcoords.push(...faceUv[i]);
    }
  }
  return {
    positions: new Float32Array(positions),
    colors: new Uint8Array(colors),
    normals: new Float32Array(normals),
    texcoords: new Float32Array(texcoords),
  };
}

function makeCheckerTexture(width, height) {
  const pixels = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      const checker = ((x >> 2) ^ (y >> 2)) & 1;
      pixels[i + 0] = checker ? 65 : 245;
      pixels[i + 1] = checker ? 180 : 115;
      pixels[i + 2] = checker ? 230 : 80;
      pixels[i + 3] = 255;
    }
  }
  return pixels;
}

function makeGlyphAtlas(width, height) {
  const pixels = new Uint8Array(width * height * 4);
  const cell = Math.floor(width / 3);
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const letter = Math.min(2, Math.floor(x / cell));
      const lx = x - letter * cell;
      const top = y <= 2;
      const bottom = y >= height - 3;
      const left = lx <= 2;
      const right = lx >= cell - 4;
      const fill =
        (letter === 0 && (left || top || bottom || (right && y > 2 && y < height - 3))) ||
        (letter === 1 && y >= height / 2 && Math.abs(lx - cell / 2) <= Math.max(1, y - height / 2)) ||
        (letter === 2 && (top || bottom || Math.abs(lx - (cell - 1 - y)) <= 1));
      if (!fill) continue;
      const i = (y * width + x) * 4;
      pixels[i + 0] = 255;
      pixels[i + 1] = 255;
      pixels[i + 2] = 255;
      pixels[i + 3] = 255;
    }
  }
  return pixels;
}

function setF32(Module, visual, attrPtr, dataPtr, count, label) {
  expectStatus(Module._dvz_wasm_api_visual_set_f32(visual, attrPtr, dataPtr, count), 0, label);
}

function setRGBA8(Module, visual, attrPtr, dataPtr, count, label) {
  expectStatus(Module._dvz_wasm_api_visual_set_rgba8(visual, attrPtr, dataPtr, count), 0, label);
}

function setU32(Module, visual, attrPtr, dataPtr, count, label) {
  expectStatus(Module._dvz_wasm_api_visual_set_u32(visual, attrPtr, dataPtr, count), 0, label);
}

function createBuffer(Module, scene, usage, stride, byteSize, label) {
  const buffer = Module._dvz_wasm_api_buffer(scene, usage, stride, byteSize);
  requireOk(buffer !== 0, `${label}: scene buffer creation failed`);
  return buffer;
}

function setBufferData(Module, buffer, dataPtr, byteSize, label) {
  expectStatus(Module._dvz_wasm_api_buffer_set_data(buffer, dataPtr, byteSize), 0, label);
}

function setAttrBuffer(Module, visual, attrPtr, buffer, byteOffset, count, label) {
  expectStatus(
    Module._dvz_wasm_api_visual_set_attr_buffer(visual, attrPtr, buffer, byteOffset, count),
    0,
    label,
  );
}

function setStandardMaterial(Module, visual, label, roughness = 0.42, metallic = 0.04) {
  expectStatus(
    Module._dvz_wasm_api_visual_set_material(
      visual,
      DVZ_MATERIAL_MODEL_STANDARD,
      1.0,
      1.05, 0.96, 1.0, 1.0,
      -0.45, -0.38, 0.8,
      0.24, 0.82, 0.24, 26.0,
      roughness, 0.52, metallic,
      0.0, 0.0, 0.0, 0.16,
    ),
    0,
    label,
  );
}

function setSegmentCaps(Module, visual, startCap, endCap, label) {
  expectStatus(Module._dvz_wasm_api_visual_set_segment_caps(visual, startCap, endCap), 0, label);
}

function setPathCaps(Module, visual, startCap, endCap, label) {
  expectStatus(Module._dvz_wasm_api_visual_set_path_caps(visual, startCap, endCap), 0, label);
}

function setPathJoin(Module, visual, join, miterLimit, label) {
  expectStatus(Module._dvz_wasm_api_visual_set_path_join(visual, join, miterLimit), 0, label);
}

function setLabelsS32(
  Module,
  visual,
  valuesPtr,
  width,
  height,
  categoryIdsPtr,
  colorsPtr,
  categoryCount,
  label,
) {
  expectStatus(
    Module._dvz_wasm_api_visual_set_labels_s32(
      visual,
      valuesPtr,
      width,
      height,
      categoryIdsPtr,
      colorsPtr,
      categoryCount,
    ),
    0,
    label,
  );
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
  { supportsColorBlending = true } = {},
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
      supportsColorBlending ? 1 : 0,
    ),
    0,
    label,
  );
}

const { default: createModule } = await import(pathToFileURL(modulePath).href);
const Module = await createModule({
  locateFile: (path) => join(dirname(modulePath), path),
  printErr: wasmPrintErr,
});
const smokeSize = 64;

await expectBrowserWrapperPacketRuntime();
expectNoLegacyDirectAbi(Module);

const positionNamePtr = allocCString(Module, "position");
const colorNamePtr = allocCString(Module, "color");
const diameterNamePtr = allocCString(Module, "diameter");
const pixelSizeNamePtr = allocCString(Module, "pixel_size");
const angleNamePtr = allocCString(Module, "angle");
const symbolNamePtr = allocCString(Module, "symbol");
const positionStartNamePtr = allocCString(Module, "position_start");
const positionEndNamePtr = allocCString(Module, "position_end");
const strokeWidthNamePtr = allocCString(Module, "stroke_width");
const normalNamePtr = allocCString(Module, "normal");
const radiusNamePtr = allocCString(Module, "radius");
const texcoordsNamePtr = allocCString(Module, "texcoords");
const boundsNamePtr = allocCString(Module, "bounds");
const textNamePtr = allocCString(Module, "text");
const anchorNamePtr = allocCString(Module, "anchor");
const sizeNamePtr = allocCString(Module, "size");
const extentNamePtr = allocCString(Module, "extent");
const xAxisLabelPtr = allocCString(Module, "x");
const yAxisLabelPtr = allocCString(Module, "y");

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
      Module._dvz_wasm_api_set_capabilities(diagnosticScene, 0, 4, 8, 1024, 256, 1, 1),
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
    const badBuffer = Module._dvz_wasm_api_buffer(diagnosticScene, 0, 12, 12);
    requireOk(badBuffer === 0, "invalid scene buffer descriptor unexpectedly succeeded");
    expectDiagnostics(
      Module, diagnosticScene, "invalid WASM scene buffer descriptor", "invalid scene buffer");

    const pointForDiagnostics = Module._dvz_wasm_api_visual(diagnosticScene, DVZ_WASM_VISUAL_POINT, 0);
    requireOk(pointForDiagnostics !== 0, "diagnostic point creation failed");
    expectNoDiagnostics(Module, diagnosticScene, "successful visual creation clears diagnostics");
    const materialStderr = captureExpectedStderr("point material rejection", () => {
      expectStatus(
        Module._dvz_wasm_api_visual_set_material(
          pointForDiagnostics,
          DVZ_MATERIAL_MODEL_STANDARD,
          1.0,
          1.0, 1.0, 1.0, 1.0,
          -0.45, -0.35, 0.82,
          0.24, 0.82, 0.24, 26.0,
          0.62, 0.34, 0.0,
          0.0, 0.0, 0.0, 0.1,
        ),
        -1,
        "unsupported material visual",
      );
    });
    expectCapturedStderr(
      materialStderr, "materials are only supported", "point material rejection");
    expectDiagnostics(Module, diagnosticScene, "WASM visual material update failed", "point material rejection");
    const segmentCapStderr = captureExpectedStderr("point segment cap rejection", () => {
      expectStatus(
        Module._dvz_wasm_api_visual_set_segment_caps(
          pointForDiagnostics, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_SQUARE),
        -1,
        "unsupported segment cap visual",
      );
    });
    expectCapturedStderr(
      segmentCapStderr, "dvz_segment_set_caps requires a segment visual",
      "point segment cap rejection");
    expectDiagnostics(Module, diagnosticScene, "WASM segment cap update failed", "point segment cap rejection");
    const pathJoinStderr = captureExpectedStderr("point path join rejection", () => {
      expectStatus(
        Module._dvz_wasm_api_visual_set_path_join(pointForDiagnostics, DVZ_PATH_JOIN_BEVEL, 4.0),
        -1,
        "unsupported path join visual",
      );
    });
    expectCapturedStderr(
      pathJoinStderr, "dvz_path_set_join requires a path visual", "point path join rejection");
    expectDiagnostics(Module, diagnosticScene, "WASM path join update failed", "point path join rejection");

    const badAttrNamePtr = allocCString(Module, "not_an_attr");
    const onePositionPtr = allocArray(Module, new Float32Array([0, 0, 0]));
    try {
      const diagnosticBuffer =
        createBuffer(Module, diagnosticScene, DVZ_SCENE_BUFFER_USAGE_VERTEX, 12, 12, "diagnostic position buffer");
      setBufferData(Module, diagnosticBuffer, onePositionPtr, 12, "diagnostic position buffer upload");
      const attrBufferStderr = captureExpectedStderr("invalid attr buffer", () => {
        expectStatus(
          Module._dvz_wasm_api_visual_set_attr_buffer(
            pointForDiagnostics, badAttrNamePtr, diagnosticBuffer, 0, 1),
          -1,
          "invalid visual attribute buffer",
        );
      });
      expectCapturedStderr(
        attrBufferStderr, "unsupported point visual attribute 'not_an_attr'",
        "invalid attr buffer");
      expectDiagnostics(
        Module, diagnosticScene, "WASM visual attribute buffer bind failed",
        "invalid attr buffer");
      const attrUploadStderr = captureExpectedStderr("invalid attr", () => {
        expectStatus(
          Module._dvz_wasm_api_visual_set_f32(pointForDiagnostics, badAttrNamePtr, onePositionPtr, 1),
          -1,
          "invalid visual attribute",
        );
      });
      expectCapturedStderr(
        attrUploadStderr, "unsupported point visual attribute 'not_an_attr'", "invalid attr");
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

  requireOk(Module._dvz_wasm_api_scenario_count() >= 3, "expected gallery-live WASM scenarios");
  const expectedScenarioIds = [
    "feature_basic_scene",
    "feature_timer_animation",
    "feature_triangulation_polygon",
    "feature_builtin_shapes_2d",
    "feature_builtin_shapes_3d",
    "feature_isolines",
    "feature_animation_tracks",
    "feature_obj_loading",
    "feature_picking",
    "feature_selection_pixel",
    "feature_selection_sphere",
    "feature_image_probe",
  ];
  for (let i = 0; i < expectedScenarioIds.length; i++) {
    const ptr = Module._dvz_wasm_api_scenario_id(i);
    requireOk(ptr !== 0, `WASM scenario ${i} has no id`);
    const id = Module.UTF8ToString(ptr);
    requireOk(id === expectedScenarioIds[i], `unexpected scenario ${i} id ${id}`);
    if (
      id === "feature_picking" ||
      id === "feature_selection_sphere" ||
      id === "feature_image_probe"
    ) {
      requireOk(
        (Module._dvz_wasm_api_scenario_requirements(i) & (1 << 8)) !== 0,
        `${id} did not declare query readback`,
      );
    }
  }
  const timerScenarioIndex = scenarioIndex(Module, "feature_timer_animation");
  const scenarioIdPtr = Module._dvz_wasm_api_scenario_id(timerScenarioIndex);
  requireOk(scenarioIdPtr !== 0, "WASM scenario 0 has no id");
  const scenarioId = Module.UTF8ToString(scenarioIdPtr);
  requireOk(scenarioId === "feature_timer_animation", `unexpected scenario id ${scenarioId}`);
  const scenarioTitlePtr = Module._dvz_wasm_api_scenario_title(timerScenarioIndex);
  requireOk(scenarioTitlePtr !== 0, "WASM scenario 0 has no title");
  requireOk(Module.UTF8ToString(scenarioTitlePtr) === "timer_animation", "unexpected scenario title");
  requireOk(Module._dvz_wasm_api_scenario_width(timerScenarioIndex) === 1600, "unexpected scenario width");
  requireOk(Module._dvz_wasm_api_scenario_height(timerScenarioIndex) === 1200, "unexpected scenario height");
  requireOk(Module._dvz_wasm_api_scenario_fps(timerScenarioIndex) === 60, "unexpected scenario fps");
  requireOk(
    (Module._dvz_wasm_api_scenario_requirements(timerScenarioIndex) & (1 << 9)) !== 0,
    "timer scenario did not declare frame callbacks",
  );

  const scenarioScene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(scenarioScene !== 0, "scenario scene creation failed");
  try {
    expectStatus(
      Module._dvz_wasm_api_set_canvas_format(scenarioScene, DVZ_FORMAT_R8G8B8A8_UNORM),
      0,
      "scenario canvas format",
    );
    expectStatus(
      Module._dvz_wasm_api_scenario_create(scenarioScene, timerScenarioIndex),
      0,
      "timer scenario create",
    );
    expectNoDiagnostics(Module, scenarioScene, "timer scenario create diagnostics");
    const scenarioFigure = Module._dvz_wasm_api_scenario_figure(scenarioScene);
    requireOk(scenarioFigure !== 0, "timer scenario has no figure");
    const initialScenario = emitStream(Module, scenarioScene, scenarioFigure, "timer scenario initial");
    expectTimerScenarioStreamShape(initialScenario.stream, "timer scenario initial");

    expectStatus(
      Module._dvz_wasm_api_scenario_pointer(
        scenarioScene, DVZ_POINTER_EVENT_MOVE, 128, 96, DVZ_POINTER_BUTTON_LEFT, 0, 1, 25),
      0,
      "timer scenario pointer event",
    );
    expectStatus(
      Module._dvz_wasm_api_scenario_wheel(scenarioScene, 128, 96, 0, -1, 0, 1, 26),
      0,
      "timer scenario wheel event",
    );
    expectNoDiagnostics(Module, scenarioScene, "timer scenario event diagnostics");

    expectStatus(
      Module._dvz_wasm_api_scenario_frame(scenarioScene, 0.25, 1 / 60),
      0,
      "timer scenario first frame",
    );
    expectStatus(
      Module._dvz_wasm_api_scenario_post_frame(scenarioScene),
      0,
      "timer scenario post-frame",
    );
    requireOk(
      typeof Module._dvz_wasm_api_query_pending_count === "function",
      "missing WASM scenario query pending ABI",
    );
    requireOk(
      typeof Module._dvz_wasm_api_emit_query_packets === "function",
      "missing WASM scenario query packet ABI",
    );
    requireOk(
      typeof Module._dvz_wasm_api_query_active === "function",
      "missing WASM scenario query active ABI",
    );
    requireOk(
      typeof Module._dvz_wasm_api_query_readback_size === "function",
      "missing WASM scenario query readback size ABI",
    );
    requireOk(
      typeof Module._dvz_wasm_api_query_resolve === "function",
      "missing WASM scenario query resolve ABI",
    );
    requireOk(
      Module._dvz_wasm_api_query_pending_count(scenarioScene) === 0,
      "timer scenario should not queue query requests",
    );
    expectNoPayload(Module, scenarioScene, "scenario frame invalidates payload");
    const frameScenario = emitStream(Module, scenarioScene, scenarioFigure, "timer scenario frame");
    expectTimerScenarioStreamShape(frameScenario.stream, "timer scenario frame", {
      allowSetupCommands: false,
    });
    expectWriteCommands(frameScenario.stream, "timer scenario frame");
    requireOk(
      frameScenario.payload !== initialScenario.payload,
      "timer scenario frame payload did not replace initial payload",
    );

    expectStatus(
      Module._dvz_wasm_api_scenario_frame(scenarioScene, 0.5, 1 / 60),
      0,
      "timer scenario second frame",
    );
    const splitScenario = emitIncrementalPacketStream(
      Module, scenarioScene, scenarioFigure, "timer scenario split packet frame");
    requireOk(
      splitScenario.update.commandCount > 0 && splitScenario.frame.commandCount > 0,
      "timer scenario split packet frame missing update/frame commands",
    );
    await writeFile(outputScenarioPath, `${JSON.stringify(initialScenario.stream, null, 2)}\n`, "utf8");
    console.log(`Wrote ${outputScenarioPath}`);
    console.log(`commands_scenario_timer=initial:${initialScenario.stream.commands.length} frame:${frameScenario.stream.commands.length}`);
  } finally {
    Module._dvz_wasm_api_scene_destroy(scenarioScene);
  }

  const animationScene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(animationScene !== 0, "animation scenario scene creation failed");
  try {
    expectStatus(
      Module._dvz_wasm_api_set_canvas_format(animationScene, DVZ_FORMAT_R8G8B8A8_UNORM),
      0,
      "animation scenario canvas format",
    );
    expectStatus(
      Module._dvz_wasm_api_scenario_create(animationScene, 6),
      0,
      "animation scenario create",
    );
    expectNoDiagnostics(Module, animationScene, "animation scenario create diagnostics");
    const animationFigure = Module._dvz_wasm_api_scenario_figure(animationScene);
    requireOk(animationFigure !== 0, "animation scenario has no figure");
    const initialAnimation =
      emitStream(Module, animationScene, animationFigure, "animation scenario initial");
    expectWriteCommands(initialAnimation.stream, "animation scenario initial");
    expectStatus(
      Module._dvz_wasm_api_scenario_frame(animationScene, 0.25, 1 / 60),
      0,
      "animation scenario frame",
    );
    const frameAnimation =
      emitStream(Module, animationScene, animationFigure, "animation scenario frame");
    expectWriteCommands(frameAnimation.stream, "animation scenario frame");
  } finally {
    Module._dvz_wasm_api_scene_destroy(animationScene);
  }

  const pixelSelectionScene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(pixelSelectionScene !== 0, "pixel selection scenario scene creation failed");
  try {
    expectStatus(
      Module._dvz_wasm_api_set_canvas_format(pixelSelectionScene, DVZ_FORMAT_R8G8B8A8_UNORM),
      0,
      "pixel selection scenario canvas format",
    );
    expectStatus(
      Module._dvz_wasm_api_scenario_create(pixelSelectionScene, 9),
      0,
      "pixel selection scenario create",
    );
    expectNoDiagnostics(Module, pixelSelectionScene, "pixel selection scenario create diagnostics");
    const pixelSelectionFigure = Module._dvz_wasm_api_scenario_figure(pixelSelectionScene);
    requireOk(pixelSelectionFigure !== 0, "pixel selection scenario has no figure");
    const initialPixelSelection =
      emitStream(Module, pixelSelectionScene, pixelSelectionFigure, "pixel selection initial");
    expectWriteCommands(initialPixelSelection.stream, "pixel selection initial");
    expectStatus(
      Module._dvz_wasm_api_scenario_pointer(
        pixelSelectionScene, DVZ_POINTER_EVENT_MOVE, smokeSize / 2, smokeSize / 2, 0, 0, 1, 40),
      0,
      "pixel selection pointer move",
    );
    expectStatus(
      Module._dvz_wasm_api_scenario_post_frame(pixelSelectionScene),
      0,
      "pixel selection post-frame",
    );
    requireOk(
      Module._dvz_wasm_api_query_pending_count(pixelSelectionScene) > 0,
      "pixel selection scenario did not queue a query",
    );
    expectStatus(
      Module._dvz_wasm_api_emit_query_packets(pixelSelectionScene, pixelSelectionFigure),
      0,
      "pixel selection query packet emit",
    );
    expectNoDiagnostics(Module, pixelSelectionScene, "pixel selection query packet diagnostics");
    requireOk(
      Module._dvz_wasm_api_query_active(pixelSelectionScene) === 1,
      "pixel selection query did not become active",
    );
    requireOk(
      Module._dvz_wasm_api_query_readback_size(pixelSelectionScene) > 0,
      "pixel selection query has no readback size",
    );
    expectPacket(
      Module, pixelSelectionScene, DVZ_DRP2_PACKET_SETUP, "pixel selection query setup");
    expectPacket(
      Module, pixelSelectionScene, DVZ_DRP2_PACKET_UPDATE, "pixel selection query update", {
        expectArena: true,
      });
    expectPacket(Module, pixelSelectionScene, DVZ_DRP2_PACKET_FRAME, "pixel selection query frame");
  } finally {
    Module._dvz_wasm_api_scene_destroy(pixelSelectionScene);
  }

  const sphereSelectionIndex = scenarioIndex(Module, "feature_selection_sphere");
  const sphereSelectionScene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(sphereSelectionScene !== 0, "sphere selection scenario scene creation failed");
  try {
    expectStatus(
      Module._dvz_wasm_api_set_canvas_format(sphereSelectionScene, DVZ_FORMAT_R8G8B8A8_UNORM),
      0,
      "sphere selection scenario canvas format",
    );
    expectStatus(
      Module._dvz_wasm_api_scenario_create(sphereSelectionScene, sphereSelectionIndex),
      0,
      "sphere selection scenario create",
    );
    expectNoDiagnostics(Module, sphereSelectionScene, "sphere selection scenario create diagnostics");
    const sphereSelectionFigure = Module._dvz_wasm_api_scenario_figure(sphereSelectionScene);
    requireOk(sphereSelectionFigure !== 0, "sphere selection scenario has no figure");
    const initialSphereSelection =
      emitStream(Module, sphereSelectionScene, sphereSelectionFigure, "sphere selection initial");
    expectWriteCommands(initialSphereSelection.stream, "sphere selection initial");
    expectStatus(
      Module._dvz_wasm_api_scenario_pointer(
        sphereSelectionScene, DVZ_POINTER_EVENT_MOVE, smokeSize / 2, smokeSize / 2, 0, 0, 1, 41),
      0,
      "sphere selection pointer move",
    );
    expectStatus(
      Module._dvz_wasm_api_scenario_post_frame(sphereSelectionScene),
      0,
      "sphere selection post-frame",
    );
    requireOk(
      Module._dvz_wasm_api_query_pending_count(sphereSelectionScene) > 0,
      "sphere selection scenario did not queue a query",
    );
    expectStatus(
      Module._dvz_wasm_api_emit_query_packets(sphereSelectionScene, sphereSelectionFigure),
      0,
      "sphere selection query packet emit",
    );
    expectNoDiagnostics(Module, sphereSelectionScene, "sphere selection query packet diagnostics");
    requireOk(
      Module._dvz_wasm_api_query_active(sphereSelectionScene) === 1,
      "sphere selection query did not become active",
    );
    requireOk(
      Module._dvz_wasm_api_query_readback_size(sphereSelectionScene) === 4,
      "sphere selection query readback size was not 4 bytes",
    );
    const sphereQuerySetup = expectPacket(
      Module, sphereSelectionScene, DVZ_DRP2_PACKET_SETUP, "sphere selection query setup");
    requireOk(
      sphereQuerySetup.decoded.commands.some(
        (command) => command.cmd === "CreateTexture" && command.format === "r32uint",
      ),
      "sphere selection query setup did not create an r32uint target",
    );
    expectPacket(
      Module, sphereSelectionScene, DVZ_DRP2_PACKET_UPDATE, "sphere selection query update", {
        expectArena: true,
      });
    expectPacket(Module, sphereSelectionScene, DVZ_DRP2_PACKET_FRAME, "sphere selection query frame");
  } finally {
    Module._dvz_wasm_api_scene_destroy(sphereSelectionScene);
  }

  const positions = new Float32Array([-0.75, -0.45, 0, -0.35, 0.35, 0, 0.05, -0.1, 0, 0.42, 0.5, 0, 0.72, -0.35, 0]);
  const colors = new Uint8Array([231, 77, 60, 255, 46, 204, 113, 255, 52, 152, 219, 255, 241, 196, 15, 255, 155, 89, 182, 255]);
  const sizes = new Float32Array([32, 44, 36, 48, 40]);
  const pixelPositions = new Float32Array([-0.72, 0.12, 0.02, -0.52, 0.42, 0.02, -0.32, 0.12, 0.02, -0.12, 0.42, 0.02, 0.08, 0.12, 0.02, 0.28, 0.42, 0.02]);
  const pixelColors = new Uint8Array([60, 190, 245, 255, 120, 225, 170, 255, 245, 175, 85, 255, 210, 105, 220, 255, 80, 210, 195, 255, 235, 95, 125, 255]);
  const pixelSizes = new Float32Array([7, 9, 8, 10, 9, 7]);
  const markerPositions = new Float32Array([0.42, 0.08, 0.04, 0.62, 0.36, 0.04, 0.82, 0.08, 0.04, 0.62, 0.64, 0.04]);
  const markerColors = new Uint8Array([245, 125, 90, 235, 80, 210, 195, 235, 170, 130, 245, 235, 245, 215, 90, 235]);
  const markerDiameters = new Float32Array([12, 14, 13, 15]);
  const markerAngles = new Float32Array([0, 0.35, 0.7, 1.05]);
  const markerSymbols = new Uint32Array([0, 1, 2, 3]);
  const segmentStarts = new Float32Array([-0.78, -0.72, 0.08, -0.48, -0.72, 0.08, -0.18, -0.72, 0.08]);
  const segmentEnds = new Float32Array([-0.58, -0.38, 0.08, -0.28, -0.50, 0.08, -0.02, -0.30, 0.08]);
  const segmentColors = new Uint8Array([80, 205, 245, 230, 245, 165, 75, 230, 135, 225, 150, 230]);
  const segmentWidths = new Float32Array([4, 7, 5]);
  const pathPositions = new Float32Array([
    -0.84, 0.12, 0.12,
    -0.66, 0.32, 0.12,
    -0.42, 0.18, 0.12,
    -0.22, 0.46, 0.12,
    -0.02, 0.22, 0.12,
  ]);
  const pathColors = new Uint8Array([
    70, 235, 180, 230,
    100, 210, 245, 230,
    170, 150, 245, 230,
    245, 125, 175, 230,
    245, 180, 90, 230,
  ]);
  const pathWidths = new Float32Array([5, 7, 6, 8, 5]);
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
  const labelsWidth = 6;
  const labelsHeight = 5;
  const labelsValues = new Int32Array([
    1, 1, 2, 2, 3, 3,
    1, 1, 2, 2, 3, 3,
    4, 4, 0, 0, 3, 3,
    4, 4, 1, 1, 2, 2,
    4, 4, 1, 1, 2, 2,
  ]);
  const labelsCategoryIds = new Int32Array([1, 2, 3, 4]);
  const labelsCategoryColors = new Uint8Array([
    245, 94, 92, 220,
    83, 203, 168, 220,
    86, 156, 244, 220,
    246, 207, 95, 180,
  ]);
  const labelsPosition = new Float32Array([0.52, -0.45, 0.09]);
  const labelsExtent = new Float32Array([0.58, 0.42]);
  const glyphWidth = 48;
  const glyphHeight = 16;
  const glyphPixels = makeGlyphAtlas(glyphWidth, glyphHeight);
  const glyphAnchors = [
    [-0.70, 0.78, 0.18],
    [-0.50, 0.78, 0.18],
    [-0.30, 0.78, 0.18],
  ];
  const glyphUvBounds = [
    [0, 0, 1 / 3, 1],
    [1 / 3, 0, 2 / 3, 1],
    [2 / 3, 0, 1, 1],
  ];
  const glyphPositions = [];
  const glyphBounds = [];
  const glyphTexcoords = [];
  const glyphColors = [];
  const glyphAngles = [];
  for (let i = 0; i < glyphAnchors.length; i++) {
    for (let j = 0; j < 6; j++) {
      glyphPositions.push(...glyphAnchors[i]);
      glyphBounds.push(-18, -14, 18, 14);
      glyphTexcoords.push(...glyphUvBounds[i]);
      glyphColors.push(250, 250, 255, 245);
      glyphAngles.push(0.0);
    }
  }
  const glyphPositionData = new Float32Array(glyphPositions);
  const glyphBoundsData = new Float32Array(glyphBounds);
  const glyphTexcoordsData = new Float32Array(glyphTexcoords);
  const glyphColorData = new Uint8Array(glyphColors);
  const glyphAngleData = new Float32Array(glyphAngles);
  const textStrings = allocCStringArray(Module, ["WASM"]);
  const textPositions = new Float32Array([0.02, 0.78, 0.24]);
  const textAnchors = new Float32Array([0, 0.5]);
  const textSizes = new Float32Array([12]);
  const textColors = new Uint8Array([255, 255, 255, 245]);
  const textAngles = new Float32Array([0]);
  const meshPositions = new Float32Array([0.18, 0.18, 0.22, 0.86, 0.18, 0.22, 0.18, 0.78, 0.22, 0.86, 0.18, 0.22, 0.86, 0.78, 0.22, 0.18, 0.78, 0.22]);
  const meshColors = new Uint8Array([90, 170, 255, 240, 85, 230, 190, 240, 160, 120, 255, 240, 85, 230, 190, 240, 255, 135, 210, 240, 160, 120, 255, 240]);
  const meshNormals = new Float32Array([0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1]);

  const scene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(scene !== 0, "dvz_wasm_api_scene failed");
  const ptrs = [
    allocArray(Module, positions), allocArray(Module, colors), allocArray(Module, sizes),
    allocArray(Module, pixelPositions), allocArray(Module, pixelColors), allocArray(Module, pixelSizes),
    allocArray(Module, markerPositions), allocArray(Module, markerColors),
    allocArray(Module, markerDiameters), allocArray(Module, markerAngles),
    allocArray(Module, markerSymbols),
    allocArray(Module, segmentStarts), allocArray(Module, segmentEnds),
    allocArray(Module, segmentColors), allocArray(Module, segmentWidths),
    allocArray(Module, pathPositions), allocArray(Module, pathColors),
    allocArray(Module, pathWidths),
    allocArray(Module, primitivePositions), allocArray(Module, primitiveColors),
    allocArray(Module, imagePositions), allocArray(Module, imageTexcoords), allocArray(Module, imagePixels),
    allocArray(Module, glyphPositionData), allocArray(Module, glyphBoundsData),
    allocArray(Module, glyphTexcoordsData), allocArray(Module, glyphColorData),
    allocArray(Module, glyphAngleData), allocArray(Module, glyphPixels),
    allocArray(Module, textPositions), allocArray(Module, textAnchors),
    allocArray(Module, textSizes), allocArray(Module, textColors), allocArray(Module, textAngles),
    allocArray(Module, meshPositions), allocArray(Module, meshColors), allocArray(Module, meshNormals),
    allocArray(Module, labelsValues), allocArray(Module, labelsCategoryIds),
    allocArray(Module, labelsCategoryColors), allocArray(Module, labelsPosition),
    allocArray(Module, labelsExtent),
  ];
  try {
    expectStatus(Module._dvz_wasm_api_set_canvas_format(scene, DVZ_FORMAT_R8G8B8A8_UNORM), 0, "api 2D canvas format");
    const figure = Module._dvz_wasm_api_figure(scene, smokeSize, smokeSize);
    const panel = Module._dvz_wasm_api_panel_full(figure);
    requireOk(figure !== 0 && panel !== 0, "api 2D figure/panel failed");
    expectStatus(
      Module._dvz_wasm_api_panel_set_domain(panel, DVZ_DIM_X, -1.0, 1.0),
      0,
      "api 2D x domain",
    );
    expectStatus(
      Module._dvz_wasm_api_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0),
      0,
      "api 2D y domain",
    );
    const xAxis = Module._dvz_wasm_api_panel_axis(panel, DVZ_DIM_X);
    const yAxis = Module._dvz_wasm_api_panel_axis(panel, DVZ_DIM_Y);
    requireOk(xAxis !== 0 && yAxis !== 0, "api 2D axis creation failed");
    expectStatus(Module._dvz_wasm_api_axis_set_grid(xAxis, 1), 0, "api 2D x axis grid");
    expectStatus(Module._dvz_wasm_api_axis_set_grid(yAxis, 1), 0, "api 2D y axis grid");
    expectStatus(Module._dvz_wasm_api_axis_set_label(xAxis, xAxisLabelPtr), 0, "api 2D x axis label");
    expectStatus(Module._dvz_wasm_api_axis_set_label(yAxis, yAxisLabelPtr), 0, "api 2D y axis label");

    const point = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_POINT, 0);
    const pointPositionBuffer = createBuffer(
      Module, scene, DVZ_SCENE_BUFFER_USAGE_VERTEX, 12, positions.byteLength,
      "api point position buffer");
    setBufferData(
      Module, pointPositionBuffer, ptrs[0], positions.byteLength,
      "api point position buffer upload");
    setAttrBuffer(
      Module, point, positionNamePtr, pointPositionBuffer, 0, positions.length / 3,
      "api point position buffer bind");
    setRGBA8(Module, point, colorNamePtr, ptrs[1], colors.length / 4, "api point color");
    setF32(Module, point, diameterNamePtr, ptrs[2], sizes.length, "api point diameter");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, point), 0, "api add point");

    const pixel = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_PIXEL, 0);
    const pixelPositionBuffer = createBuffer(
      Module, scene, DVZ_SCENE_BUFFER_USAGE_VERTEX, 12, pixelPositions.byteLength,
      "api pixel position buffer");
    setBufferData(
      Module, pixelPositionBuffer, ptrs[3], pixelPositions.byteLength,
      "api pixel position buffer upload");
    setAttrBuffer(
      Module, pixel, positionNamePtr, pixelPositionBuffer, 0, pixelPositions.length / 3,
      "api pixel position buffer bind");
    setRGBA8(Module, pixel, colorNamePtr, ptrs[4], pixelColors.length / 4, "api pixel color");
    setF32(Module, pixel, pixelSizeNamePtr, ptrs[5], pixelSizes.length, "api pixel size");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, pixel), 0, "api add pixel");

    const marker = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_MARKER, 0);
    setF32(Module, marker, positionNamePtr, ptrs[6], markerPositions.length / 3, "api marker position");
    setRGBA8(Module, marker, colorNamePtr, ptrs[7], markerColors.length / 4, "api marker color");
    setF32(Module, marker, diameterNamePtr, ptrs[8], markerDiameters.length, "api marker diameter");
    setF32(Module, marker, angleNamePtr, ptrs[9], markerAngles.length, "api marker angle");
    setU32(Module, marker, symbolNamePtr, ptrs[10], markerSymbols.length, "api marker symbol");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, marker), 0, "api add marker");

    const segment = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_SEGMENT, 0);
    setF32(Module, segment, positionStartNamePtr, ptrs[11], segmentStarts.length / 3, "api segment start");
    setF32(Module, segment, positionEndNamePtr, ptrs[12], segmentEnds.length / 3, "api segment end");
    setRGBA8(Module, segment, colorNamePtr, ptrs[13], segmentColors.length / 4, "api segment color");
    setF32(Module, segment, strokeWidthNamePtr, ptrs[14], segmentWidths.length, "api segment width");
    setSegmentCaps(
      Module, segment, DVZ_SEGMENT_CAP_SQUARE, DVZ_SEGMENT_CAP_TRIANGLE_OUT,
      "api segment caps");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, segment), 0, "api add segment");

    const path = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_PATH, 0);
    setF32(Module, path, positionNamePtr, ptrs[15], pathPositions.length / 3, "api path position");
    setRGBA8(Module, path, colorNamePtr, ptrs[16], pathColors.length / 4, "api path color");
    setF32(Module, path, strokeWidthNamePtr, ptrs[17], pathWidths.length, "api path width");
    setPathCaps(Module, path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_SQUARE, "api path caps");
    setPathJoin(Module, path, DVZ_PATH_JOIN_MITER, 2.5, "api path join");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, path), 0, "api add path");

    const primitive = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_PRIMITIVE, 0);
    setF32(Module, primitive, positionNamePtr, ptrs[18], primitivePositions.length / 3, "api primitive position");
    setRGBA8(Module, primitive, colorNamePtr, ptrs[19], primitiveColors.length / 4, "api primitive color");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, primitive), 0, "api add primitive");

    const image = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_IMAGE, 0);
    setF32(Module, image, positionNamePtr, ptrs[20], imagePositions.length / 3, "api image position");
    setF32(Module, image, texcoordsNamePtr, ptrs[21], imageTexcoords.length / 2, "api image texcoords");
    expectStatus(Module._dvz_wasm_api_visual_set_texture_rgba8(image, ptrs[22], imageWidth, imageHeight), 0, "api image texture");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, image), 0, "api add image");

    const labels = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_LABELS, 0);
    setF32(Module, labels, positionNamePtr, ptrs[40], labelsPosition.length / 3, "api labels position");
    setF32(Module, labels, extentNamePtr, ptrs[41], labelsExtent.length / 2, "api labels extent");
    setLabelsS32(
      Module, labels, ptrs[37], labelsWidth, labelsHeight, ptrs[38], ptrs[39],
      labelsCategoryIds.length, "api labels s32");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, labels), 0, "api add labels");

    const glyph = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_GLYPH, 0);
    setF32(Module, glyph, positionNamePtr, ptrs[23], glyphPositionData.length / 3, "api glyph position");
    setF32(Module, glyph, boundsNamePtr, ptrs[24], glyphBoundsData.length / 4, "api glyph bounds");
    setF32(Module, glyph, texcoordsNamePtr, ptrs[25], glyphTexcoordsData.length / 4, "api glyph texcoords");
    setRGBA8(Module, glyph, colorNamePtr, ptrs[26], glyphColorData.length / 4, "api glyph color");
    setF32(Module, glyph, angleNamePtr, ptrs[27], glyphAngleData.length, "api glyph angle");
    expectStatus(Module._dvz_wasm_api_visual_set_texture_rgba8(glyph, ptrs[28], glyphWidth, glyphHeight), 0, "api glyph texture");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, glyph), 0, "api add glyph");

    const mesh = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_MESH, 0);
    setF32(Module, mesh, positionNamePtr, ptrs[34], meshPositions.length / 3, "api mesh position");
    setRGBA8(Module, mesh, colorNamePtr, ptrs[35], meshColors.length / 4, "api mesh color");
    setF32(Module, mesh, normalNamePtr, ptrs[36], meshNormals.length / 3, "api mesh normal");
    setStandardMaterial(Module, mesh, "api mesh standard material");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, mesh), 0, "api add mesh");

    const panzoom = Module._dvz_wasm_api_controller(scene, DVZ_CONTROLLER_TYPE_PANZOOM);
    expectStatus(Module._dvz_wasm_api_panel_bind_controller(panel, panzoom, DVZ_DIM_MASK_XY), 0, "api bind panzoom");
    setCapabilities(Module, scene, 4096, 4, 8, 128, 256, 1, "restrict buffer capability");
    expectStatus(Module._dvz_wasm_api_emit(scene, figure), -1, "buffer capability emit rejection");
    expectAnyDiagnostics(Module, scene, "buffer capability emit rejection");
    setCapabilities(Module, scene, 4096, 4, 8, 256 * 1024 * 1024, 256, 1, "restore buffer capability");

    const text = Module._dvz_wasm_api_visual(scene, DVZ_WASM_VISUAL_TEXT, 0);
    expectStatus(
      Module._dvz_wasm_api_visual_set_strings(text, textNamePtr, textStrings.ptr, 1),
      0,
      "api text strings",
    );
    setF32(Module, text, positionNamePtr, ptrs[29], textPositions.length / 3, "api text position");
    setF32(Module, text, anchorNamePtr, ptrs[30], textAnchors.length / 2, "api text anchor");
    setF32(Module, text, sizeNamePtr, ptrs[31], textSizes.length, "api text size");
    setRGBA8(Module, text, colorNamePtr, ptrs[32], textColors.length / 4, "api text color");
    setF32(Module, text, angleNamePtr, ptrs[33], textAngles.length, "api text angle");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel, text), 0, "api add text");

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
    setStandardMaterial(Module, mesh, "api mesh material update", 0.22, 0.16);
    const materialUpdate = emitStream(Module, scene, figure, "generic 2D material update");
    expect2DUpdateStreamShape(materialUpdate.stream, "generic 2D material update");
    expectWriteCommands(materialUpdate.stream, "generic 2D material update");
    const updatedPointPositions = new Float32Array(positions);
    updatedPointPositions[0] += 0.08;
    updatedPointPositions[1] += 0.04;
    const updatedPointPositionsPtr = allocArray(Module, updatedPointPositions);
    try {
      setBufferData(
        Module, pointPositionBuffer, updatedPointPositionsPtr, updatedPointPositions.byteLength,
        "api point position buffer update");
      const bufferUpdate = emitStream(Module, scene, figure, "generic 2D buffer update");
      expect2DUpdateStreamShape(bufferUpdate.stream, "generic 2D buffer update");
      expectWriteCommands(bufferUpdate.stream, "generic 2D buffer update");
    } finally {
      Module._free(updatedPointPositionsPtr);
    }
    setSegmentCaps(
      Module, segment, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_BUTT,
      "api segment cap update");
    setPathCaps(
      Module, path, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_TRIANGLE_OUT,
      "api path cap update");
    setPathJoin(Module, path, DVZ_PATH_JOIN_BEVEL, 4.0, "api path join update");
    const strokeStyleUpdate = emitStream(Module, scene, figure, "generic 2D stroke style update");
    expect2DUpdateStreamShape(strokeStyleUpdate.stream, "generic 2D stroke style update");
    expectWriteCommands(strokeStyleUpdate.stream, "generic 2D stroke style update");
    const grownPositionPtr = allocArray(Module, new Float32Array([...positions, -0.1, 0.75, 0]));
    try {
      const pointCountStderr = captureExpectedStderr("point count growth", () => {
        expectStatus(
          Module._dvz_wasm_api_visual_set_f32(
            point, positionNamePtr, grownPositionPtr, positions.length / 3 + 1),
          -1,
          "api point count growth",
        );
      });
      expectCapturedStderr(
        pointCountStderr,
        "point visual attribute 'position' item_count 6 does not match existing attribute 'color' item_count 5",
        "point count growth");
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
      expectFrameCommandShape(visualReload.stream, "generic 2D visual reload", 2);
      expectSetupCommands(visualReload.stream, "generic 2D visual reload");
      expectDraw(visualReload.stream, 6, 5, "generic 2D visual reload point");
      expectDraw(visualReload.stream, 6, 6, "generic 2D visual reload pixel");
      expectDraw(visualReload.stream, 6, 4, "generic 2D visual reload marker");
      expectDrawIndexed(visualReload.stream, 18, 1, "generic 2D visual reload segment");
      expectDrawIndexed(visualReload.stream, 24, 1, "generic 2D visual reload path");
      expectDraw(visualReload.stream, 3, 1, "generic 2D visual reload primitive");
      expectDraw(visualReload.stream, 4, 1, "generic 2D visual reload image");
      expectDraw(visualReload.stream, 6, 1, "generic 2D visual reload labels");
      expectDraw(visualReload.stream, 18, 1, "generic 2D visual reload glyph");
      expectDraw(visualReload.stream, 24, 1, "generic 2D visual reload text");
      expectDraw(visualReload.stream, 6, 1, "generic 2D visual reload mesh");
      expectStatus(
        Module._dvz_wasm_api_visual_set_texture_rgba8(image, ptrs[22], imageWidth, imageHeight),
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
      expectReleasedPackets(
        Module, scene, "generic 2D split packet reload",
        splitReload.resourceVersion, splitReload.frameIndex);
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
    expect2DUpdateStreamShape(resized.stream, "generic 2D resized", 5, { allowSetupCommands: true });
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
    freeCStringArray(Module, textStrings);
    Module._dvz_wasm_api_scene_destroy(scene);
  }

  const cube = makeCubeMesh(1.25);
  const cubeTextureWidth = 16;
  const cubeTextureHeight = 16;
  const cubeTexture = makeCheckerTexture(cubeTextureWidth, cubeTextureHeight);
  const spherePositions = new Float32Array([
    -0.72, -0.38, 0.32,
    0.74, -0.34, -0.18,
    0.0, 0.72, 0.18,
  ]);
  const sphereColors = new Uint8Array([
    245, 120, 90, 255,
    80, 210, 195, 255,
    245, 215, 90, 255,
  ]);
  const sphereRadii = new Float32Array([0.18, 0.16, 0.14]);
  const scene3d = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
  requireOk(scene3d !== 0, "dvz_wasm_api_scene 3D failed");
  const cubePtrs = [
    allocArray(Module, cube.positions), allocArray(Module, cube.colors),
    allocArray(Module, cube.normals), allocArray(Module, cube.texcoords),
    allocArray(Module, cubeTexture),
    allocArray(Module, spherePositions), allocArray(Module, sphereColors),
    allocArray(Module, sphereRadii),
  ];
  try {
    expectStatus(Module._dvz_wasm_api_set_canvas_format(scene3d, DVZ_FORMAT_R8G8B8A8_UNORM), 0, "api 3D canvas format");
    const figure3d = Module._dvz_wasm_api_figure(scene3d, smokeSize, smokeSize);
    const panel3d = Module._dvz_wasm_api_panel_full(figure3d);
    requireOk(figure3d !== 0 && panel3d !== 0, "api 3D figure/panel failed");
    expectStatus(Module._dvz_wasm_api_panel_set_camera(panel3d, 0, 0, 3, 0, 0, 0, Math.PI / 4, 0.1, 100), 0, "api 3D camera");
    const sphere3d = Module._dvz_wasm_api_visual(scene3d, DVZ_WASM_VISUAL_SPHERE, 0);
    setF32(Module, sphere3d, positionNamePtr, cubePtrs[5], spherePositions.length / 3, "api 3D sphere position");
    setRGBA8(Module, sphere3d, colorNamePtr, cubePtrs[6], sphereColors.length / 4, "api 3D sphere color");
    setF32(Module, sphere3d, radiusNamePtr, cubePtrs[7], sphereRadii.length, "api 3D sphere radius");
    expectStatus(Module._dvz_wasm_api_panel_add_visual(panel3d, sphere3d), 0, "api 3D add sphere");
    const mesh3d = Module._dvz_wasm_api_visual(scene3d, DVZ_WASM_VISUAL_MESH, 0);
    setF32(Module, mesh3d, positionNamePtr, cubePtrs[0], cube.positions.length / 3, "api 3D mesh position");
    setRGBA8(Module, mesh3d, colorNamePtr, cubePtrs[1], cube.colors.length / 4, "api 3D mesh color");
    setF32(Module, mesh3d, normalNamePtr, cubePtrs[2], cube.normals.length / 3, "api 3D mesh normal");
    setF32(Module, mesh3d, texcoordsNamePtr, cubePtrs[3], cube.texcoords.length / 2, "api 3D mesh texcoords");
    expectStatus(
      Module._dvz_wasm_api_visual_set_texture_rgba8(
        mesh3d, cubePtrs[4], cubeTextureWidth, cubeTextureHeight),
      0,
      "api 3D mesh texture",
    );
    setStandardMaterial(Module, mesh3d, "api 3D mesh standard material", 0.42, 0.04);
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
  Module._free(pixelSizeNamePtr);
  Module._free(angleNamePtr);
  Module._free(symbolNamePtr);
  Module._free(positionStartNamePtr);
  Module._free(positionEndNamePtr);
  Module._free(strokeWidthNamePtr);
  Module._free(normalNamePtr);
  Module._free(radiusNamePtr);
  Module._free(texcoordsNamePtr);
  Module._free(boundsNamePtr);
  Module._free(textNamePtr);
  Module._free(anchorNamePtr);
  Module._free(sizeNamePtr);
  Module._free(extentNamePtr);
  Module._free(xAxisLabelPtr);
  Module._free(yAxisLabelPtr);
}
