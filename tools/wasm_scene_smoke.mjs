#!/usr/bin/env node

import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { mkdir, writeFile } from "node:fs/promises";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const modulePath = resolve(root, "build-wasm-scene/wasm/datoviz_wasm_scene.mjs");
const outputPath = resolve(root, "build-wasm-scene/wasm/wasm_scene_point_primitive_image_mesh_panzoom.json");

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
const primitivePositions = new Float32Array([
  -0.85, -0.7, 0.15,
  -0.15, -0.7, 0.15,
  -0.5, 0.1, 0.15,
]);
const primitiveColors = new Uint8Array([
  255, 120, 90, 220,
  255, 180, 90, 220,
  255, 90, 150, 220,
]);
const primitivePositionsPtr = allocArray(Module, primitivePositions);
const primitiveColorsPtr = allocArray(Module, primitiveColors);
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
const imagePixelsPtr = allocArray(Module, imagePixels);
const meshPositions = new Float32Array([
  0.18, 0.18, 0.22,
  0.86, 0.18, 0.22,
  0.18, 0.78, 0.22,
  0.86, 0.18, 0.22,
  0.86, 0.78, 0.22,
  0.18, 0.78, 0.22,
]);
const meshColors = new Uint8Array([
  90, 170, 255, 240,
  85, 230, 190, 240,
  160, 120, 255, 240,
  85, 230, 190, 240,
  255, 135, 210, 240,
  160, 120, 255, 240,
]);
const meshNormals = new Float32Array([
  0, 0, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 1,
]);
const meshPositionsPtr = allocArray(Module, meshPositions);
const meshColorsPtr = allocArray(Module, meshColors);
const meshNormalsPtr = allocArray(Module, meshNormals);

try {
  let status = Module._dvz_wasm_scene_set_points(
    handle,
    positionsPtr,
    colorsPtr,
    sizesPtr,
    sizes.length,
  );
  requireOk(status === 0, `dvz_wasm_scene_set_points failed with ${status}`);
  status = Module._dvz_wasm_scene_set_primitive(
    handle,
    primitivePositionsPtr,
    primitiveColorsPtr,
    primitivePositions.length / 3,
  );
  requireOk(status === 0, `dvz_wasm_scene_set_primitive failed with ${status}`);
  status = Module._dvz_wasm_scene_set_image(handle, imagePixelsPtr, imageWidth, imageHeight);
  requireOk(status === 0, `dvz_wasm_scene_set_image failed with ${status}`);
  status = Module._dvz_wasm_scene_set_mesh(
    handle,
    meshPositionsPtr,
    meshColorsPtr,
    meshNormalsPtr,
    meshPositions.length / 3,
  );
  requireOk(status === 0, `dvz_wasm_scene_set_mesh failed with ${status}`);

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
  Module._free(primitivePositionsPtr);
  Module._free(primitiveColorsPtr);
  Module._free(imagePixelsPtr);
  Module._free(meshPositionsPtr);
  Module._free(meshColorsPtr);
  Module._free(meshNormalsPtr);
  Module._dvz_wasm_scene_destroy(handle);
}
