#!/usr/bin/env node

import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { mkdir, writeFile } from "node:fs/promises";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const modulePath = resolve(root, "build-wasm-scene/wasm/datoviz_wasm_scene.mjs");
const outputPath = resolve(root, "build-wasm-scene/wasm/wasm_scene_point_panzoom.json");

const DVZ_POINTER_EVENT_PRESS = 1;
const DVZ_POINTER_EVENT_RELEASE = 0;
const DVZ_POINTER_EVENT_MOVE = 2;
const DVZ_POINTER_BUTTON_LEFT = 1;
const DVZ_FORMAT_R8G8B8A8_UNORM = 37;

function requireOk(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function readPayload(Module, handle) {
  const ptr = Module._dvz_wasm_scene_payload_ptr(handle);
  const size = Module._dvz_wasm_scene_payload_size(handle);
  requireOk(ptr !== 0, "WASM scene payload pointer is null");
  requireOk(size > 0, "WASM scene payload is empty");
  return new TextDecoder().decode(Module.HEAPU8.subarray(ptr, ptr + size));
}

function requirePayloadCleared(Module, handle, label) {
  const ptr = Module._dvz_wasm_scene_payload_ptr(handle);
  const size = Module._dvz_wasm_scene_payload_size(handle);
  requireOk(ptr === 0, `${label} did not clear borrowed payload pointer`);
  requireOk(size === 0, `${label} did not clear borrowed payload size`);
}

function diagnostics(Module, handle) {
  const count = Module._dvz_wasm_scene_diagnostic_count(handle);
  const messages = [];
  for (let i = 0; i < count; i++) {
    const ptr = Module._dvz_wasm_scene_diagnostic(handle, i);
    messages.push(ptr !== 0 ? Module.UTF8ToString(ptr) : "<null diagnostic>");
  }
  return messages;
}

function requireNoDiagnostics(Module, handle, prefix) {
  const messages = diagnostics(Module, handle);
  requireOk(messages.length === 0, `${prefix}: ${messages.join("; ")}`);
}

function throwDiagnostics(Module, handle, prefix) {
  const messages = diagnostics(Module, handle);
  requireOk(messages.length > 0, `${prefix}: no diagnostic was reported`);
  throw new Error(`${prefix}: ${messages.join("; ")}`);
}

function emitStream(Module, handle, label) {
  const status = Module._dvz_wasm_scene_emit(handle);
  if (status !== 0) {
    throwDiagnostics(Module, handle, `${label} emit failed with ${status}`);
  }
  requireNoDiagnostics(Module, handle, `${label} emit unexpectedly reported diagnostics`);
  const streamText = readPayload(Module, handle);
  const stream = JSON.parse(streamText);
  requireOk(Array.isArray(stream.commands), `${label} stream has no commands array`);
  requireOk(stream.commands.length > 0, `${label} stream has no commands`);
  return stream;
}

function allocArray(Module, typedArray) {
  const ptr = Module._malloc(typedArray.byteLength);
  requireOk(ptr !== 0, "malloc failed");
  Module.HEAPU8.set(new Uint8Array(typedArray.buffer, typedArray.byteOffset, typedArray.byteLength), ptr);
  return ptr;
}

const { default: createModule } = await import(pathToFileURL(modulePath).href);
const Module = await createModule({
  locateFile(path) {
    return join(dirname(modulePath), path);
  },
});

const smokeSize = 64;
const handle = Module._dvz_wasm_scene_create(smokeSize, smokeSize);
requireOk(handle !== 0, "dvz_wasm_scene_create failed");
requireOk(
  Module._dvz_wasm_scene_set_canvas_format(handle, DVZ_FORMAT_R8G8B8A8_UNORM) === 0,
  "dvz_wasm_scene_set_canvas_format failed",
);

const positions = new Float32Array([
  -0.75, -0.45, 0.0,
  -0.35, 0.35, 0.0,
  0.05, -0.1, 0.0,
  0.42, 0.5, 0.0,
  0.72, -0.35, 0.0,
]);
const colors = new Uint8Array([
  231, 77, 60, 255,
  46, 204, 113, 255,
  52, 152, 219, 255,
  241, 196, 15, 255,
  155, 89, 182, 255,
]);
const sizes = new Float32Array([32, 44, 36, 48, 40]);

const positionsPtr = allocArray(Module, positions);
const colorsPtr = allocArray(Module, colors);
const sizesPtr = allocArray(Module, sizes);

try {
  let status = Module._dvz_wasm_scene_set_points(
    handle,
    positionsPtr,
    colorsPtr,
    sizesPtr,
    sizes.length,
  );
  requireOk(status === 0, `dvz_wasm_scene_set_points failed with ${status}`);

  const initialStream = emitStream(Module, handle, "initial");

  const t0 = 10.0;
  Module._dvz_wasm_scene_pointer(
    handle,
    DVZ_POINTER_EVENT_PRESS,
    smokeSize / 2,
    smokeSize / 2,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    1,
    t0,
  );
  requirePayloadCleared(Module, handle, "pointer event");
  Module._dvz_wasm_scene_pointer(
    handle,
    DVZ_POINTER_EVENT_MOVE,
    smokeSize / 2 + 6,
    smokeSize / 2 - 2,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    1,
    t0 + 16.0,
  );
  Module._dvz_wasm_scene_pointer(
    handle,
    DVZ_POINTER_EVENT_RELEASE,
    smokeSize / 2 + 6,
    smokeSize / 2 - 2,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    1,
    t0 + 32.0,
  );
  Module._dvz_wasm_scene_wheel(handle, smokeSize / 2, smokeSize / 2, 0, 1, 0, 1, t0 + 48.0);

  const interactiveStream = emitStream(Module, handle, "interactive");

  status = Module._dvz_wasm_scene_resize(handle, smokeSize * 2, smokeSize + 16, 2.0);
  requireOk(status === 0, `resize failed with ${status}`);
  requirePayloadCleared(Module, handle, "resize");
  const resizeStream = emitStream(Module, handle, "resize");

  Module._dvz_wasm_scene_pointer(
    handle,
    DVZ_POINTER_EVENT_PRESS,
    smokeSize / 2 + 10,
    smokeSize / 2 + 4,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    2,
    t0 + 64.0,
  );
  Module._dvz_wasm_scene_pointer(
    handle,
    DVZ_POINTER_EVENT_MOVE,
    smokeSize / 2 + 2,
    smokeSize / 2 + 12,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    2,
    t0 + 80.0,
  );
  Module._dvz_wasm_scene_pointer(
    handle,
    DVZ_POINTER_EVENT_RELEASE,
    smokeSize / 2 + 2,
    smokeSize / 2 + 12,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    2,
    t0 + 96.0,
  );
  const secondInteractiveStream = emitStream(Module, handle, "second interactive");

  await mkdir(dirname(outputPath), { recursive: true });
  await writeFile(outputPath, `${JSON.stringify(initialStream, null, 2)}\n`, "utf8");
  console.log(`Wrote ${outputPath}`);
  console.log(
    `commands=initial:${initialStream.commands.length} ` +
      `interactive:${interactiveStream.commands.length} ` +
      `resize:${resizeStream.commands.length} ` +
      `second_interactive:${secondInteractiveStream.commands.length}`,
  );
} finally {
  Module._free(positionsPtr);
  Module._free(colorsPtr);
  Module._free(sizesPtr);
  Module._dvz_wasm_scene_destroy(handle);
}
