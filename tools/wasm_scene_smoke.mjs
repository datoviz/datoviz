#!/usr/bin/env node

import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { mkdir, writeFile } from "node:fs/promises";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const modulePath = resolve(root, "build-wasm-scene/wasm/datoviz_wasm_scene.mjs");
const outputPath = resolve(root, "build-wasm-scene/wasm/wasm_scene_point_primitive_image_mesh_panzoom.json");
const output3dPath = resolve(root, "build-wasm-scene/wasm/wasm_scene_mesh3d_arcball.json");
const outputApiPath = resolve(root, "build-wasm-scene/wasm/wasm_api_scene_point.json");

const DVZ_POINTER_EVENT_PRESS = 1;
const DVZ_POINTER_EVENT_RELEASE = 0;
const DVZ_POINTER_EVENT_MOVE = 2;
const DVZ_POINTER_BUTTON_LEFT = 1;
const DVZ_FORMAT_R8G8B8A8_UNORM = 37;
const DVZ_CONTROLLER_TYPE_PANZOOM = 1;
const DVZ_DIM_MASK_XY = 3;
const DVZ_WASM_VISUAL_POINT = 1;

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

function expectStatus(status, expected, label) {
  requireOk(status === expected, `${label} returned ${status}, expected ${expected}`);
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

function allocCString(Module, text) {
  const bytes = new TextEncoder().encode(`${text}\0`);
  const ptr = Module._malloc(bytes.byteLength);
  requireOk(ptr !== 0, "malloc failed");
  Module.HEAPU8.set(bytes, ptr);
  return ptr;
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
  return {
    positions: new Float32Array(positions),
    colors: new Uint8Array(colors),
    normals: new Float32Array(normals),
  };
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
  expectStatus(Module._dvz_wasm_scene_set_canvas_format(0, DVZ_FORMAT_R8G8B8A8_UNORM), -1, "bad handle canvas format");
  let status = Module._dvz_wasm_scene_set_points(
    handle,
    positionsPtr,
    colorsPtr,
    sizesPtr,
    sizes.length,
  );
  expectStatus(status, 0, "dvz_wasm_scene_set_points");
  expectStatus(
    Module._dvz_wasm_scene_set_primitive(handle, primitivePositionsPtr, primitiveColorsPtr, 2),
    -1,
    "invalid primitive count",
  );
  requirePayloadCleared(Module, handle, "invalid primitive count");
  status = Module._dvz_wasm_scene_set_primitive(
    handle,
    primitivePositionsPtr,
    primitiveColorsPtr,
    primitivePositions.length / 3,
  );
  expectStatus(status, 0, "dvz_wasm_scene_set_primitive");
  expectStatus(Module._dvz_wasm_scene_set_image(handle, imagePixelsPtr, 0, imageHeight), -1, "zero-width image");
  requirePayloadCleared(Module, handle, "zero-width image");
  status = Module._dvz_wasm_scene_set_image(handle, imagePixelsPtr, imageWidth, imageHeight);
  expectStatus(status, 0, "dvz_wasm_scene_set_image");
  expectStatus(
    Module._dvz_wasm_scene_set_mesh(handle, meshPositionsPtr, meshColorsPtr, 0, meshPositions.length / 3),
    -1,
    "mesh missing normals",
  );
  requirePayloadCleared(Module, handle, "mesh missing normals");
  expectStatus(
    Module._dvz_wasm_scene_set_mesh(handle, meshPositionsPtr, meshColorsPtr, meshNormalsPtr, 2),
    -1,
    "invalid mesh count",
  );
  requirePayloadCleared(Module, handle, "invalid mesh count");
  status = Module._dvz_wasm_scene_set_mesh(
    handle,
    meshPositionsPtr,
    meshColorsPtr,
    meshNormalsPtr,
    meshPositions.length / 3,
  );
  expectStatus(status, 0, "dvz_wasm_scene_set_mesh");

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

const cube = makeCubeMesh(1.25);
const handle3d = Module._dvz_wasm_scene_create_3d(smokeSize, smokeSize);
requireOk(handle3d !== 0, "dvz_wasm_scene_create_3d failed");
requireOk(
  Module._dvz_wasm_scene_set_canvas_format(handle3d, DVZ_FORMAT_R8G8B8A8_UNORM) === 0,
  "dvz_wasm_scene_set_canvas_format failed for 3D scene",
);
const cubePositionsPtr = allocArray(Module, cube.positions);
const cubeColorsPtr = allocArray(Module, cube.colors);
const cubeNormalsPtr = allocArray(Module, cube.normals);
try {
  let status = Module._dvz_wasm_scene_set_mesh(
    handle3d,
    cubePositionsPtr,
    cubeColorsPtr,
    cubeNormalsPtr,
    cube.positions.length / 3,
  );
  requireOk(status === 0, `3D dvz_wasm_scene_set_mesh failed with ${status}`);

  const stream3d = emitStream(Module, handle3d, "3D initial");
  Module._dvz_wasm_scene_pointer(
    handle3d,
    DVZ_POINTER_EVENT_PRESS,
    smokeSize / 2,
    smokeSize / 2,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    1,
    100.0,
  );
  Module._dvz_wasm_scene_pointer(
    handle3d,
    DVZ_POINTER_EVENT_MOVE,
    smokeSize / 2 + 8,
    smokeSize / 2 + 6,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    1,
    116.0,
  );
  Module._dvz_wasm_scene_pointer(
    handle3d,
    DVZ_POINTER_EVENT_RELEASE,
    smokeSize / 2 + 8,
    smokeSize / 2 + 6,
    DVZ_POINTER_BUTTON_LEFT,
    0,
    1,
    132.0,
  );
  const interactive3d = emitStream(Module, handle3d, "3D interactive");

  await writeFile(output3dPath, `${JSON.stringify(stream3d, null, 2)}\n`, "utf8");
  console.log(`Wrote ${output3dPath}`);
  console.log(`commands3d=initial:${stream3d.commands.length} interactive:${interactive3d.commands.length}`);
} finally {
  Module._free(cubePositionsPtr);
  Module._free(cubeColorsPtr);
  Module._free(cubeNormalsPtr);
  Module._dvz_wasm_scene_destroy(handle3d);
}

const apiScene = Module._dvz_wasm_api_scene(smokeSize, smokeSize);
requireOk(apiScene !== 0, "dvz_wasm_api_scene failed");
const positionNamePtr = allocCString(Module, "position");
const colorNamePtr = allocCString(Module, "color");
const diameterNamePtr = allocCString(Module, "diameter");
const apiPositionsPtr = allocArray(Module, positions);
const apiColorsPtr = allocArray(Module, colors);
const apiSizesPtr = allocArray(Module, sizes);
try {
  expectStatus(
    Module._dvz_wasm_api_set_canvas_format(apiScene, DVZ_FORMAT_R8G8B8A8_UNORM),
    0,
    "dvz_wasm_api_set_canvas_format",
  );
  const apiFigure = Module._dvz_wasm_api_figure(apiScene, smokeSize, smokeSize);
  requireOk(apiFigure !== 0, "dvz_wasm_api_figure failed");
  const apiPanel = Module._dvz_wasm_api_panel_full(apiFigure);
  requireOk(apiPanel !== 0, "dvz_wasm_api_panel_full failed");
  const apiPoint = Module._dvz_wasm_api_visual(apiScene, DVZ_WASM_VISUAL_POINT, 0);
  requireOk(apiPoint !== 0, "dvz_wasm_api_visual(point) failed");
  expectStatus(
    Module._dvz_wasm_api_visual_set_f32(apiPoint, positionNamePtr, apiPositionsPtr, positions.length / 3),
    0,
    "dvz_wasm_api_visual_set_f32(position)",
  );
  expectStatus(
    Module._dvz_wasm_api_visual_set_rgba8(apiPoint, colorNamePtr, apiColorsPtr, colors.length / 4),
    0,
    "dvz_wasm_api_visual_set_rgba8(color)",
  );
  expectStatus(
    Module._dvz_wasm_api_visual_set_f32(apiPoint, diameterNamePtr, apiSizesPtr, sizes.length),
    0,
    "dvz_wasm_api_visual_set_f32(diameter)",
  );
  expectStatus(
    Module._dvz_wasm_api_panel_add_visual(apiPanel, apiPoint),
    0,
    "dvz_wasm_api_panel_add_visual",
  );
  const apiController = Module._dvz_wasm_api_controller(apiScene, DVZ_CONTROLLER_TYPE_PANZOOM);
  requireOk(apiController !== 0, "dvz_wasm_api_controller(panzoom) failed");
  expectStatus(
    Module._dvz_wasm_api_panel_bind_controller(apiPanel, apiController, DVZ_DIM_MASK_XY),
    0,
    "dvz_wasm_api_panel_bind_controller",
  );
  expectStatus(Module._dvz_wasm_api_emit(apiScene, apiFigure), 0, "dvz_wasm_api_emit");
  requireOk(Module._dvz_wasm_api_diagnostic_count(apiScene) === 0, "generic API emitted diagnostics");
  const ptr = Module._dvz_wasm_api_payload_ptr(apiScene);
  const size = Module._dvz_wasm_api_payload_size(apiScene);
  requireOk(ptr !== 0 && size > 0, "generic API emitted no payload");
  const apiStream = JSON.parse(new TextDecoder().decode(Module.HEAPU8.subarray(ptr, ptr + size)));
  requireOk(Array.isArray(apiStream.commands), "generic API stream has no commands array");
  requireOk(apiStream.commands.length > 0, "generic API stream has no commands");
  expectStatus(
    Module._dvz_wasm_api_pointer(
      apiScene,
      DVZ_POINTER_EVENT_PRESS,
      smokeSize / 2,
      smokeSize / 2,
      DVZ_POINTER_BUTTON_LEFT,
      0,
      1,
      200.0,
    ),
    0,
    "dvz_wasm_api_pointer",
  );
  expectStatus(Module._dvz_wasm_api_emit(apiScene, apiFigure), 0, "dvz_wasm_api_emit after pointer");
  expectStatus(
    Module._dvz_wasm_api_resize(apiScene, apiFigure, smokeSize * 2, smokeSize + 8, 2.0),
    0,
    "dvz_wasm_api_resize",
  );
  expectStatus(Module._dvz_wasm_api_emit(apiScene, apiFigure), 0, "dvz_wasm_api_emit after resize");
  await writeFile(outputApiPath, `${JSON.stringify(apiStream, null, 2)}\n`, "utf8");
  console.log(`Wrote ${outputApiPath}`);
  console.log(`commands_api=${apiStream.commands.length}`);
} finally {
  Module._free(positionNamePtr);
  Module._free(colorNamePtr);
  Module._free(diameterNamePtr);
  Module._free(apiPositionsPtr);
  Module._free(apiColorsPtr);
  Module._free(apiSizesPtr);
  Module._dvz_wasm_api_scene_destroy(apiScene);
}
