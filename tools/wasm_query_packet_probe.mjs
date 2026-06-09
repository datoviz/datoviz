#!/usr/bin/env node

import { pathToFileURL } from "node:url";
import { resolve } from "node:path";
import { decodeDrp2Packet } from "../web/drp2/packet.js";

const modulePath = process.argv[2] ?? "build-wasm-scene/wasm/datoviz_wasm_scene.mjs";
const createModule = (await import(pathToFileURL(resolve(modulePath)))).default;
const Module = await createModule();

const DVZ_FORMAT_R8G8B8A8_UNORM = 37;
const DVZ_DRP2_PACKET_SETUP = 1;
const DVZ_DRP2_PACKET_UPDATE = 2;
const DVZ_DRP2_PACKET_FRAME = 3;
const DVZ_POINTER_EVENT_MOVE = 2;
const DVZ_POINTER_BUTTON_LEFT = 1;

function requireOk(condition, message) {
  if (!condition) throw new Error(message);
}

function diagnostic(scene, index) {
  const ptr = Module._dvz_wasm_api_diagnostic(scene, index);
  return ptr ? Module.UTF8ToString(ptr) : "<null>";
}

function packetSize(scene, kind) {
  const size = Module._dvz_wasm_api_packet_size(scene, kind);
  requireOk(size > 0, `missing packet kind ${kind}`);
  return size;
}

function packet(scene, kind) {
  const packetPtr = Module._dvz_wasm_api_packet_ptr(scene, kind);
  const packetBytes = packetSize(scene, kind);
  const arenaPtr = Module._dvz_wasm_api_packet_arena_ptr(scene, kind);
  const arenaBytes = Module._dvz_wasm_api_packet_arena_size(scene, kind);
  const bytes = Module.HEAPU8.subarray(packetPtr, packetPtr + packetBytes).slice();
  const arena = arenaPtr !== 0
    ? Module.HEAPU8.subarray(arenaPtr, arenaPtr + arenaBytes).slice()
    : new Uint8Array();
  return decodeDrp2Packet(bytes, arena);
}

function assertPacketsReleased(scene, label, resourceVersion, frameIndex) {
  requireOk(Module._dvz_wasm_api_release_packets(scene) === 0, `${label}: release failed`);
  requireOk(Module._dvz_wasm_api_packet_status(scene) === 0, `${label}: released packet status failed`);
  for (const kind of [DVZ_DRP2_PACKET_SETUP, DVZ_DRP2_PACKET_UPDATE, DVZ_DRP2_PACKET_FRAME]) {
    requireOk(Module._dvz_wasm_api_packet_ptr(scene, kind) === 0, `${label}: packet ${kind} ptr survived release`);
    requireOk(Module._dvz_wasm_api_packet_size(scene, kind) === 0, `${label}: packet ${kind} size survived release`);
    requireOk(Module._dvz_wasm_api_packet_arena_ptr(scene, kind) === 0, `${label}: packet ${kind} arena ptr survived release`);
    requireOk(Module._dvz_wasm_api_packet_arena_size(scene, kind) === 0, `${label}: packet ${kind} arena size survived release`);
  }
  requireOk(Module._dvz_wasm_api_resource_version(scene) === resourceVersion, `${label}: resource counter changed on release`);
  requireOk(Module._dvz_wasm_api_frame_index(scene) === frameIndex, `${label}: frame counter changed on release`);
}

const scene = Module._dvz_wasm_api_scene(256, 256);
requireOk(scene !== 0, "scene creation failed");

try {
  requireOk(
    Module._dvz_wasm_api_set_canvas_format(scene, DVZ_FORMAT_R8G8B8A8_UNORM) === 0,
    "canvas format setup failed",
  );
  requireOk(Module._dvz_wasm_api_scenario_create(scene, 1) === 0, "point scenario creation failed");
  const figure = Module._dvz_wasm_api_scenario_figure(scene);
  requireOk(figure !== 0, "point scenario has no figure");

  requireOk(Module._dvz_wasm_api_emit_packets(scene, figure) === 0, "initial packet emit failed");
  const initialResourceVersion = Module._dvz_wasm_api_resource_version(scene);
  const initialFrameIndex = Module._dvz_wasm_api_frame_index(scene);
  requireOk(initialResourceVersion > 0, "initial packet emit did not set resource version");
  requireOk(initialFrameIndex > 0, "initial packet emit did not set frame index");
  requireOk(
    Module._dvz_wasm_api_scenario_pointer(
      scene, DVZ_POINTER_EVENT_MOVE, 32, 32, DVZ_POINTER_BUTTON_LEFT, 0, 1, 400) === 0,
    "query pointer event failed",
  );
  requireOk(Module._dvz_wasm_api_scenario_post_frame(scene) === 0, "post-frame query hook failed");
  requireOk(Module._dvz_wasm_api_query_pending_count(scene) === 1, "expected one pending query");

  requireOk(Module._dvz_wasm_api_emit_query_packets(scene, figure) === 0, "query packet emit failed");
  requireOk(Module._dvz_wasm_api_packet_status(scene) === 0, "query packet status failed");
  requireOk(Module._dvz_wasm_api_query_active(scene) === 1, "query did not remain active");
  requireOk(Module._dvz_wasm_api_query_readback_size(scene) === 4, "unexpected readback size");
  requireOk(Module._dvz_wasm_api_diagnostic_count(scene) === 0, "unexpected query diagnostics");
  const queryResourceVersion = Module._dvz_wasm_api_resource_version(scene);
  const queryFrameIndex = Module._dvz_wasm_api_frame_index(scene);
  requireOk(queryResourceVersion > initialResourceVersion, "query packet resource version did not advance");
  requireOk(queryFrameIndex > initialFrameIndex, "query packet frame index did not advance");

  const setup = packet(scene, DVZ_DRP2_PACKET_SETUP);
  const frame = packet(scene, DVZ_DRP2_PACKET_FRAME);
  const querySetupSize = packetSize(scene, DVZ_DRP2_PACKET_SETUP);
  const queryUpdateSize = packetSize(scene, DVZ_DRP2_PACKET_UPDATE);
  const queryFrameSize = packetSize(scene, DVZ_DRP2_PACKET_FRAME);
  requireOk(setup.resource_version === queryResourceVersion, "query setup resource counter mismatch");
  requireOk(frame.resource_version === queryResourceVersion, "query frame resource counter mismatch");
  requireOk(setup.frame_index === queryFrameIndex, "query setup frame counter mismatch");
  requireOk(frame.frame_index === queryFrameIndex, "query frame counter mismatch");
  requireOk(
    setup.commands.some((command) => command.cmd === "CreateTexture" && command.format === "r32uint"),
    "query setup packet did not create an r32uint target",
  );
  const pipeline = setup.commands.find((command) => command.cmd === "CreateRenderPipeline");
  requireOk(pipeline !== undefined, "query setup packet did not create a render pipeline");
  const attributeLocations = (pipeline.vertex_buffers ?? [])
    .flatMap((buffer) => buffer.attributes ?? [])
    .map((attribute) => attribute.shader_location);
  requireOk(
    !attributeLocations.includes(1),
    "point query pipeline still advertises unused color attribute",
  );
  requireOk(
    frame.commands.some((command) => command.cmd === "CopyTextureToBuffer"),
    "query frame packet did not copy the query target to a buffer",
  );
  const submit = frame.commands.find(
    (command) => command.cmd === "QueueSubmit" && Array.isArray(command.readbacks),
  );
  requireOk(submit !== undefined, "query frame packet did not request a readback");
  requireOk(submit.readbacks[0]?.size === 4, "query readback size was not 4 bytes");
  assertPacketsReleased(scene, "query frame artifact", queryResourceVersion, queryFrameIndex);
  requireOk(Module._dvz_wasm_api_query_active(scene) === 1, "query release cleared active query");
  requireOk(Module._dvz_wasm_api_query_readback_size(scene) === 4, "query release cleared readback state");

  requireOk(Module._dvz_wasm_api_emit_packets(scene, figure) === 0, "normal packet emit after query failed");
  const renderFrame = packet(scene, DVZ_DRP2_PACKET_FRAME);
  requireOk(
    renderFrame.frame_index === Module._dvz_wasm_api_frame_index(scene),
    "normal packet view returned stale query packet bytes",
  );
  requireOk(renderFrame.frame_index > queryFrameIndex, "normal packet frame did not advance after query");
  requireOk(Module._dvz_wasm_api_query_active(scene) === 1, "normal packet emit cleared active query");

  console.log(
    `query_packets setup=${querySetupSize} ` +
    `update=${queryUpdateSize} ` +
    `frame=${queryFrameSize}`,
  );
} catch (error) {
  const count = Module._dvz_wasm_api_diagnostic_count(scene);
  for (let i = 0; i < count; i++) console.error(`diagnostic[${i}]=${diagnostic(scene, i)}`);
  throw error;
} finally {
  Module._dvz_wasm_api_scene_destroy(scene);
}
