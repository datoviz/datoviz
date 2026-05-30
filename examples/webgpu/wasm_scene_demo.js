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
    case 0:
      return DVZ_POINTER_BUTTON_LEFT;
    case 1:
      return DVZ_POINTER_BUTTON_MIDDLE;
    case 2:
      return DVZ_POINTER_BUTTON_RIGHT;
    default:
      if ((event.buttons & 1) !== 0) {
        return DVZ_POINTER_BUTTON_LEFT;
      }
      if ((event.buttons & 4) !== 0) {
        return DVZ_POINTER_BUTTON_MIDDLE;
      }
      if ((event.buttons & 2) !== 0) {
        return DVZ_POINTER_BUTTON_RIGHT;
      }
      return DVZ_POINTER_BUTTON_NONE;
  }
}

function modifierMask(event) {
  let mods = 0;
  if (event.shiftKey) {
    mods |= 1;
  }
  if (event.ctrlKey) {
    mods |= 2;
  }
  if (event.altKey) {
    mods |= 4;
  }
  if (event.metaKey) {
    mods |= 8;
  }
  return mods;
}

function canvasFormatCode(format) {
  switch (format) {
    case "rgba8unorm":
      return DVZ_FORMAT_R8G8B8A8_UNORM;
    case "bgra8unorm":
      return DVZ_FORMAT_B8G8R8A8_UNORM;
    default:
      throw new Error(`unsupported browser canvas format ${format}`);
  }
}

function canvasPoint(event) {
  const rect = canvas.getBoundingClientRect();
  const scale = Math.max(1, window.devicePixelRatio || 1);
  return {
    x: (event.clientX - rect.left) * scale,
    y: (event.clientY - rect.top) * scale,
    scale,
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
  const text = new TextDecoder().decode(Module.HEAPU8.subarray(ptr, ptr + size));
  return JSON.parse(text);
}

function diagnosticMessage(prefix) {
  const count = Module._dvz_wasm_scene_diagnostic_count(handle);
  const messages = [];
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

function makePoints() {
  const count = 120;
  const positions = new Float32Array(count * 3);
  const colors = new Uint8Array(count * 4);
  const sizes = new Float32Array(count);

  for (let i = 0; i < count; i++) {
    const t = i / Math.max(1, count - 1);
    const angle = t * Math.PI * 9.0;
    const radius = 0.08 + 0.84 * t;
    positions[3 * i + 0] = Math.cos(angle) * radius;
    positions[3 * i + 1] = Math.sin(angle) * radius;
    positions[3 * i + 2] = 0;
    colors[4 * i + 0] = Math.round(42 + 190 * t);
    colors[4 * i + 1] = Math.round(210 - 120 * t);
    colors[4 * i + 2] = Math.round(235 - 150 * Math.abs(0.5 - t));
    colors[4 * i + 3] = 255;
    sizes[i] = 8 + 10 * (0.5 + 0.5 * Math.sin(i * 0.61));
  }

  return { positions, colors, sizes, count };
}

function setPoints() {
  const { positions, colors, sizes, count } = makePoints();
  const positionsPtr = allocArray(positions);
  const colorsPtr = allocArray(colors);
  const sizesPtr = allocArray(sizes);
  try {
    const status = Module._dvz_wasm_scene_set_points(handle, positionsPtr, colorsPtr, sizesPtr, count);
    requireOk(status === 0, `setting scene points failed with ${status}`);
  } finally {
    Module._free(positionsPtr);
    Module._free(colorsPtr);
    Module._free(sizesPtr);
  }
}

function resizeScene() {
  const changed = resizeWebGpuCanvas(gpu.device, gpu.context, gpu.format);
  const scale = Math.max(1, window.devicePixelRatio || 1);
  const status = Module._dvz_wasm_scene_resize(handle, canvas.width, canvas.height, scale);
  requireOk(status === 0, `scene resize failed with ${status}`);
  return changed;
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
  canvas.addEventListener("pointermove", (event) => {
    routePointer(event, DVZ_POINTER_EVENT_MOVE);
  });
  canvas.addEventListener("pointerup", (event) => {
    routePointer(event, DVZ_POINTER_EVENT_RELEASE);
    if (canvas.hasPointerCapture(event.pointerId)) {
      canvas.releasePointerCapture(event.pointerId);
    }
  });
  canvas.addEventListener("pointercancel", (event) => {
    routePointer(event, DVZ_POINTER_EVENT_RELEASE);
  });
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
      return new URL(path, wasmModuleUrl).href;
    },
  });

  setStatus("Starting WebGPU");
  gpu = await initWebGPU();
  handle = Module._dvz_wasm_scene_create(canvas.width, canvas.height);
  requireOk(handle !== 0, "scene creation failed");
  requireOk(
    Module._dvz_wasm_scene_set_canvas_format(handle, canvasFormatCode(gpu.format)) === 0,
    `scene rejected browser canvas format ${gpu.format}`,
  );
  resizeScene();
  setPoints();

  const initialStream = emitScene();
  runtime = new Drp2WebGpuRuntime(gpu.device, gpu.context, gpu.format, {
    capabilities: gpu.capabilities,
  });
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
