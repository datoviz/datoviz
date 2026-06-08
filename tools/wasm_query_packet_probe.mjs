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

  const setup = packet(scene, DVZ_DRP2_PACKET_SETUP);
  const frame = packet(scene, DVZ_DRP2_PACKET_FRAME);
  requireOk(
    setup.commands.some((command) => command.cmd === "CreateTexture" && command.format === "r32uint"),
    "query setup packet did not create an r32uint target",
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

  console.log(
    `query_packets setup=${packetSize(scene, DVZ_DRP2_PACKET_SETUP)} ` +
    `update=${packetSize(scene, DVZ_DRP2_PACKET_UPDATE)} ` +
    `frame=${packetSize(scene, DVZ_DRP2_PACKET_FRAME)}`,
  );
} catch (error) {
  const count = Module._dvz_wasm_api_diagnostic_count(scene);
  for (let i = 0; i < count; i++) console.error(`diagnostic[${i}]=${diagnostic(scene, i)}`);
  throw error;
} finally {
  Module._dvz_wasm_api_scene_destroy(scene);
}
