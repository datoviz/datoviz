#!/usr/bin/env node

import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { mkdir, writeFile } from "node:fs/promises";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const modulePath = resolve(root, "build-wasm-scene/wasm/datoviz_wasm_scene.mjs");
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
  const stream = JSON.parse(new TextDecoder().decode(Module.HEAPU8.subarray(ptr, ptr + size)));
  requireOk(Array.isArray(stream.commands), `${label} stream has no commands array`);
  requireOk(stream.commands.length > 0, `${label} stream has no commands`);
  return stream;
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

const { default: createModule } = await import(pathToFileURL(modulePath).href);
const Module = await createModule({ locateFile: (path) => join(dirname(modulePath), path) });
const smokeSize = 64;

const positionNamePtr = allocCString(Module, "position");
const colorNamePtr = allocCString(Module, "color");
const diameterNamePtr = allocCString(Module, "diameter");
const normalNamePtr = allocCString(Module, "normal");
const texcoordsNamePtr = allocCString(Module, "texcoords");

try {
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
    const initial = emitStream(Module, scene, figure, "generic 2D initial");
    expectStatus(Module._dvz_wasm_api_pointer(scene, DVZ_POINTER_EVENT_PRESS, 32, 32, DVZ_POINTER_BUTTON_LEFT, 0, 1, 200), 0, "api pointer press");
    expectStatus(Module._dvz_wasm_api_pointer(scene, DVZ_POINTER_EVENT_MOVE, 38, 30, DVZ_POINTER_BUTTON_LEFT, 0, 1, 216), 0, "api pointer move");
    expectStatus(Module._dvz_wasm_api_pointer(scene, DVZ_POINTER_EVENT_RELEASE, 38, 30, DVZ_POINTER_BUTTON_LEFT, 0, 1, 232), 0, "api pointer release");
    const interactive = emitStream(Module, scene, figure, "generic 2D interactive");
    expectStatus(Module._dvz_wasm_api_resize(scene, figure, smokeSize * 2, smokeSize + 8, 2), 0, "api resize");
    const resized = emitStream(Module, scene, figure, "generic 2D resized");
    await mkdir(dirname(output2dPath), { recursive: true });
    await writeFile(output2dPath, `${JSON.stringify(initial, null, 2)}\n`, "utf8");
    console.log(`Wrote ${output2dPath}`);
    console.log(`commands_api2d=initial:${initial.commands.length} interactive:${interactive.commands.length} resize:${resized.commands.length}`);
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
    expectStatus(Module._dvz_wasm_api_pointer(scene3d, DVZ_POINTER_EVENT_PRESS, 32, 32, DVZ_POINTER_BUTTON_LEFT, 0, 1, 300), 0, "api 3D pointer press");
    expectStatus(Module._dvz_wasm_api_pointer(scene3d, DVZ_POINTER_EVENT_MOVE, 40, 38, DVZ_POINTER_BUTTON_LEFT, 0, 1, 316), 0, "api 3D pointer move");
    expectStatus(Module._dvz_wasm_api_pointer(scene3d, DVZ_POINTER_EVENT_RELEASE, 40, 38, DVZ_POINTER_BUTTON_LEFT, 0, 1, 332), 0, "api 3D pointer release");
    const interactive3d = emitStream(Module, scene3d, figure3d, "generic 3D interactive");
    await writeFile(output3dPath, `${JSON.stringify(initial3d, null, 2)}\n`, "utf8");
    console.log(`Wrote ${output3dPath}`);
    console.log(`commands_api3d=initial:${initial3d.commands.length} interactive:${interactive3d.commands.length}`);
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
