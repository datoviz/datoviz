import {
  Drp2WebGpuRuntime,
  executeDrp2StreamChecked,
  initWebGPU,
  resizeWebGpuCanvas,
} from "./drp2_webgpu.js";

const statusEl = document.querySelector("#status");
const statsEl = document.querySelector("#stats");
const canvas = document.querySelector("#viewport");
const wasmModuleUrl = new URL("../../build-wasm-scene/wasm/datoviz_wasm_scene.mjs", import.meta.url);
const wasmCacheToken = Date.now().toString();
wasmModuleUrl.searchParams.set("v", wasmCacheToken);

const DVZ_POINTER_EVENT_RELEASE = 0;
const DVZ_POINTER_EVENT_PRESS = 1;
const DVZ_POINTER_EVENT_MOVE = 2;
const DVZ_POINTER_BUTTON_NONE = 0;
const DVZ_POINTER_BUTTON_LEFT = 1;
const DVZ_POINTER_BUTTON_MIDDLE = 2;
const DVZ_POINTER_BUTTON_RIGHT = 3;
const DVZ_FORMAT_R8G8B8A8_UNORM = 37;
const DVZ_FORMAT_B8G8R8A8_UNORM = 44;

let Module = null;
let handle = 0;
let runtime = null;
let gpu = null;
let rendering = false;
let pendingRender = false;

function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.classList.toggle("error", isError);
}

function requireOk(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function buttonFromPointerEvent(event) {
  switch (event.button) {
    case 0: return DVZ_POINTER_BUTTON_LEFT;
    case 1: return DVZ_POINTER_BUTTON_MIDDLE;
    case 2: return DVZ_POINTER_BUTTON_RIGHT;
    default:
      if ((event.buttons & 1) !== 0) return DVZ_POINTER_BUTTON_LEFT;
      if ((event.buttons & 4) !== 0) return DVZ_POINTER_BUTTON_MIDDLE;
      if ((event.buttons & 2) !== 0) return DVZ_POINTER_BUTTON_RIGHT;
      return DVZ_POINTER_BUTTON_NONE;
  }
}

function modifierMask(event) {
  let mods = 0;
  if (event.shiftKey) mods |= 1;
  if (event.ctrlKey) mods |= 2;
  if (event.altKey) mods |= 4;
  if (event.metaKey) mods |= 8;
  return mods;
}

function canvasFormatCode(format) {
  switch (format) {
    case "rgba8unorm": return DVZ_FORMAT_R8G8B8A8_UNORM;
    case "bgra8unorm": return DVZ_FORMAT_B8G8R8A8_UNORM;
    default: throw new Error(`unsupported browser canvas format ${format}`);
  }
}

function canvasPoint(event) {
  const rect = canvas.getBoundingClientRect();
  return {
    x: event.clientX - rect.left,
    y: event.clientY - rect.top,
    scale: Math.max(1, window.devicePixelRatio || 1),
  };
}

function allocArray(typedArray) {
  const ptr = Module._malloc(typedArray.byteLength);
  requireOk(ptr !== 0, "WASM allocation failed");
  Module.HEAPU8.set(new Uint8Array(typedArray.buffer, typedArray.byteOffset, typedArray.byteLength), ptr);
  return ptr;
}

function readPayload() {
  const ptr = Module._dvz_wasm_scene_payload_ptr(handle);
  const size = Module._dvz_wasm_scene_payload_size(handle);
  requireOk(ptr !== 0 && size > 0, "WASM scene emitted no payload");
  return JSON.parse(new TextDecoder().decode(Module.HEAPU8.subarray(ptr, ptr + size)));
}

function diagnosticMessage(prefix) {
  const messages = [];
  const count = Module._dvz_wasm_scene_diagnostic_count(handle);
  for (let i = 0; i < count; i++) {
    const ptr = Module._dvz_wasm_scene_diagnostic(handle, i);
    messages.push(ptr !== 0 ? Module.UTF8ToString(ptr) : "<null diagnostic>");
  }
  return `${prefix}${messages.length > 0 ? `: ${messages.join("; ")}` : ""}`;
}

function emitScene() {
  const status = Module._dvz_wasm_scene_emit(handle);
  if (status !== 0) {
    throw new Error(diagnosticMessage(`scene emit failed with ${status}`));
  }
  return readPayload();
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

function setCube() {
  const cube = makeCubeMesh(1.25);
  const positionsPtr = allocArray(cube.positions);
  const colorsPtr = allocArray(cube.colors);
  const normalsPtr = allocArray(cube.normals);
  try {
    const status = Module._dvz_wasm_scene_set_mesh(
      handle,
      positionsPtr,
      colorsPtr,
      normalsPtr,
      cube.positions.length / 3,
    );
    requireOk(status === 0, `setting 3D mesh failed with ${status}`);
  } finally {
    Module._free(positionsPtr);
    Module._free(colorsPtr);
    Module._free(normalsPtr);
  }
}

function resizeScene() {
  resizeWebGpuCanvas(gpu.device, gpu.context, gpu.format);
  const scale = Math.max(1, window.devicePixelRatio || 1);
  const status = Module._dvz_wasm_scene_resize(handle, canvas.width, canvas.height, scale);
  requireOk(status === 0, `scene resize failed with ${status}`);
}

async function renderIncremental() {
  if (rendering) {
    pendingRender = true;
    return;
  }
  rendering = true;
  try {
    do {
      pendingRender = false;
      const stream = emitScene();
      await executeDrp2StreamChecked(gpu.device, gpu.context, gpu.format, stream, {
        commands: stream.commands,
        state: runtime.state,
        validateCapabilities: false,
      });
      statsEl.textContent = `${stream.commands.length} commands`;
      setStatus("Rendered");
    } while (pendingRender);
  } finally {
    rendering = false;
  }
}

function routePointer(event, type) {
  event.preventDefault();
  const point = canvasPoint(event);
  Module._dvz_wasm_scene_pointer(
    handle,
    type,
    point.x,
    point.y,
    buttonFromPointerEvent(event),
    modifierMask(event),
    point.scale,
    performance.now(),
  );
  renderIncremental().catch((error) => setStatus(error.message, true));
}

function attachInput() {
  canvas.addEventListener("pointerdown", (event) => {
    canvas.setPointerCapture(event.pointerId);
    routePointer(event, DVZ_POINTER_EVENT_PRESS);
  });
  canvas.addEventListener("pointermove", (event) => routePointer(event, DVZ_POINTER_EVENT_MOVE));
  canvas.addEventListener("pointerup", (event) => {
    routePointer(event, DVZ_POINTER_EVENT_RELEASE);
    if (canvas.hasPointerCapture(event.pointerId)) canvas.releasePointerCapture(event.pointerId);
  });
  canvas.addEventListener("pointercancel", (event) => routePointer(event, DVZ_POINTER_EVENT_RELEASE));
  canvas.addEventListener("wheel", (event) => {
    event.preventDefault();
    const point = canvasPoint(event);
    Module._dvz_wasm_scene_wheel(
      handle,
      point.x,
      point.y,
      0,
      -event.deltaY / 100,
      modifierMask(event),
      point.scale,
      performance.now(),
    );
    renderIncremental().catch((error) => setStatus(error.message, true));
  }, { passive: false });
  canvas.addEventListener("contextmenu", (event) => event.preventDefault());
}

async function main() {
  setStatus("Loading WASM");
  const { default: createDatovizWasm } = await import(wasmModuleUrl.href);
  Module = await createDatovizWasm({
    locateFile(path) {
      const url = new URL(path, wasmModuleUrl);
      url.searchParams.set("v", wasmCacheToken);
      return url.href;
    },
  });
  requireOk(
    typeof Module._malloc === "function" && typeof Module._free === "function",
    "WASM module is stale or missing malloc/free exports; run `just wasm-scene-smoke` and hard-refresh",
  );

  setStatus("Starting WebGPU");
  gpu = await initWebGPU();
  handle = Module._dvz_wasm_scene_create_3d(canvas.width, canvas.height);
  requireOk(handle !== 0, "3D scene creation failed");
  requireOk(
    Module._dvz_wasm_scene_set_canvas_format(handle, canvasFormatCode(gpu.format)) === 0,
    `scene rejected browser canvas format ${gpu.format}`,
  );
  resizeScene();
  setCube();

  const initialStream = emitScene();
  runtime = new Drp2WebGpuRuntime(gpu.device, gpu.context, gpu.format, { capabilities: gpu.capabilities });
  await runtime.load(initialStream);
  await runtime.render();
  statsEl.textContent = `${initialStream.commands.length} commands`;
  setStatus("Rendered");

  attachInput();
  new ResizeObserver(() => {
    resizeScene();
    renderIncremental().catch((error) => setStatus(error.message, true));
  }).observe(canvas);
}

main().catch((error) => {
  setStatus(error.message, true);
  console.error(error);
});
