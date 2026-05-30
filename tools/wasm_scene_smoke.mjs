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

function throwDiagnostics(Module, handle, prefix) {
  const count = Module._dvz_wasm_scene_diagnostic_count(handle);
  const messages = [];
  for (let i = 0; i < count; i++) {
    const ptr = Module._dvz_wasm_scene_diagnostic(handle, i);
    messages.push(ptr !== 0 ? Module.UTF8ToString(ptr) : "<null diagnostic>");
  }
  throw new Error(`${prefix}${messages.length > 0 ? `: ${messages.join("; ")}` : ""}`);
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

  status = Module._dvz_wasm_scene_emit(handle);
  if (status !== 0) {
    throwDiagnostics(Module, handle, `initial emit failed with ${status}`);
  }
  const initialStreamText = readPayload(Module, handle);
  const initialStream = JSON.parse(initialStreamText);
  requireOk(Array.isArray(initialStream.commands), "initial stream has no commands array");
  requireOk(initialStream.commands.length > 0, "initial stream has no commands");

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

  status = Module._dvz_wasm_scene_emit(handle);
  if (status !== 0) {
    throwDiagnostics(Module, handle, `interactive emit failed with ${status}`);
  }
  const streamText = readPayload(Module, handle);
  const stream = JSON.parse(streamText);
  requireOk(Array.isArray(stream.commands), "interactive stream has no commands array");
  requireOk(stream.commands.length > 0, "interactive stream has no commands");

  await mkdir(dirname(outputPath), { recursive: true });
  await writeFile(outputPath, `${JSON.stringify(initialStream, null, 2)}\n`, "utf8");
  console.log(`Wrote ${outputPath}`);
  console.log(`commands=${initialStream.commands.length}`);
} finally {
  Module._free(positionsPtr);
  Module._free(colorsPtr);
  Module._free(sizesPtr);
  Module._dvz_wasm_scene_destroy(handle);
}
