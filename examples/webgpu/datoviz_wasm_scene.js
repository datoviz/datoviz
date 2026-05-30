import {
  Drp2WebGpuRuntime,
  executeDrp2StreamChecked,
  initWebGPU,
  resizeWebGpuCanvas,
} from "./drp2_webgpu.js";

const DVZ_FORMAT_R8G8B8A8_UNORM = 37;
const DVZ_FORMAT_B8G8R8A8_UNORM = 44;
const DVZ_DIM_MASK_XY = 3;
const DVZ_CONTROLLER_TYPE_PANZOOM = 1;
const DVZ_POINTER_EVENT_RELEASE = 0;
const DVZ_POINTER_EVENT_PRESS = 1;
const DVZ_POINTER_EVENT_MOVE = 2;
const DVZ_POINTER_BUTTON_NONE = 0;
const DVZ_POINTER_BUTTON_LEFT = 1;
const DVZ_POINTER_BUTTON_MIDDLE = 2;
const DVZ_POINTER_BUTTON_RIGHT = 3;

export const DvzWasmVisual = Object.freeze({
  point: 1,
  image: 6,
  mesh: 7,
  primitive: 9,
});

function requireOk(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
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

function wasmModuleUrl() {
  const url = new URL("../../build-wasm-scene/wasm/datoviz_wasm_scene.mjs", import.meta.url);
  url.searchParams.set("v", Date.now().toString());
  return url;
}

function allocArray(Module, typedArray) {
  const ptr = Module._malloc(typedArray.byteLength);
  requireOk(ptr !== 0, "WASM allocation failed");
  Module.HEAPU8.set(new Uint8Array(typedArray.buffer, typedArray.byteOffset, typedArray.byteLength), ptr);
  return ptr;
}

function allocCString(Module, text) {
  const bytes = new TextEncoder().encode(`${text}\0`);
  const ptr = Module._malloc(bytes.byteLength);
  requireOk(ptr !== 0, "WASM allocation failed");
  Module.HEAPU8.set(bytes, ptr);
  return ptr;
}

export class DatovizWasmScene {
  static async create(canvas, options = {}) {
    const moduleUrl = wasmModuleUrl();
    const { default: createDatovizWasm } = await import(moduleUrl.href);
    const Module = await createDatovizWasm({
      locateFile(path) {
        const url = new URL(path, moduleUrl);
        url.searchParams.set("v", moduleUrl.searchParams.get("v"));
        return url.href;
      },
    });
    requireOk(
      typeof Module._malloc === "function" && typeof Module._free === "function",
      "WASM module is stale or missing malloc/free exports; run `just wasm-scene-smoke` and hard-refresh",
    );

    const gpu = options.gpu ?? await initWebGPU();
    resizeWebGpuCanvas(gpu.device, gpu.context, gpu.format);
    const scene = Module._dvz_wasm_api_scene(canvas.width, canvas.height);
    requireOk(scene !== 0, "dvz_wasm_api_scene failed");
    requireOk(
      Module._dvz_wasm_api_set_canvas_format(scene, canvasFormatCode(gpu.format)) === 0,
      `scene rejected browser canvas format ${gpu.format}`,
    );
    const figure = Module._dvz_wasm_api_figure(scene, canvas.width, canvas.height);
    requireOk(figure !== 0, "dvz_wasm_api_figure failed");
    return new DatovizWasmScene(Module, gpu, canvas, scene, figure);
  }

  constructor(Module, gpu, canvas, scene, figure) {
    this.Module = Module;
    this.gpu = gpu;
    this.canvas = canvas;
    this.scene = scene;
    this.figure = figure;
    this.runtime = null;
  }

  destroy() {
    if (this.scene !== 0) {
      this.Module._dvz_wasm_api_scene_destroy(this.scene);
      this.scene = 0;
    }
  }

  panelFull() {
    const panel = this.Module._dvz_wasm_api_panel_full(this.figure);
    requireOk(panel !== 0, "dvz_wasm_api_panel_full failed");
    return panel;
  }

  visual(type, flags = 0) {
    const visualType = typeof type === "string" ? DvzWasmVisual[type] : type;
    requireOk(visualType !== undefined, `unknown visual type ${type}`);
    const visual = this.Module._dvz_wasm_api_visual(this.scene, visualType, flags);
    requireOk(visual !== 0, `dvz_wasm_api_visual(${type}) failed`);
    return new DatovizWasmVisualHandle(this.Module, visual);
  }

  addVisual(panel, visual) {
    const visualHandle = visual instanceof DatovizWasmVisualHandle ? visual.handle : visual;
    requireOk(
      this.Module._dvz_wasm_api_panel_add_visual(panel, visualHandle) === 0,
      "dvz_wasm_api_panel_add_visual failed",
    );
  }

  attachPanzoom(panel) {
    const controller = this.Module._dvz_wasm_api_controller(this.scene, DVZ_CONTROLLER_TYPE_PANZOOM);
    requireOk(controller !== 0, "dvz_wasm_api_controller(panzoom) failed");
    requireOk(
      this.Module._dvz_wasm_api_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) === 0,
      "dvz_wasm_api_panel_bind_controller failed",
    );
    return controller;
  }

  resize() {
    resizeWebGpuCanvas(this.gpu.device, this.gpu.context, this.gpu.format);
    const scale = Math.max(1, window.devicePixelRatio || 1);
    requireOk(
      this.Module._dvz_wasm_api_resize(this.scene, this.figure, this.canvas.width, this.canvas.height, scale) === 0,
      "dvz_wasm_api_resize failed",
    );
  }

  pointer(type, event) {
    const point = this._canvasPoint(event);
    requireOk(
      this.Module._dvz_wasm_api_pointer(
        this.scene,
        type,
        point.x,
        point.y,
        this._buttonFromPointerEvent(event),
        this._modifierMask(event),
        point.scale,
        performance.now(),
      ) === 0,
      "dvz_wasm_api_pointer failed",
    );
  }

  wheel(event) {
    const point = this._canvasPoint(event);
    requireOk(
      this.Module._dvz_wasm_api_wheel(
        this.scene,
        point.x,
        point.y,
        0,
        -event.deltaY / 100,
        this._modifierMask(event),
        point.scale,
        performance.now(),
      ) === 0,
      "dvz_wasm_api_wheel failed",
    );
  }

  attachPanzoomInput(onChange) {
    const route = (event, type) => {
      event.preventDefault();
      this.pointer(type, event);
      onChange();
    };
    this.canvas.addEventListener("pointerdown", (event) => {
      this.canvas.setPointerCapture(event.pointerId);
      route(event, DVZ_POINTER_EVENT_PRESS);
    });
    this.canvas.addEventListener("pointermove", (event) => route(event, DVZ_POINTER_EVENT_MOVE));
    this.canvas.addEventListener("pointerup", (event) => {
      route(event, DVZ_POINTER_EVENT_RELEASE);
      if (this.canvas.hasPointerCapture(event.pointerId)) {
        this.canvas.releasePointerCapture(event.pointerId);
      }
    });
    this.canvas.addEventListener("pointercancel", (event) => route(event, DVZ_POINTER_EVENT_RELEASE));
    this.canvas.addEventListener("wheel", (event) => {
      event.preventDefault();
      this.wheel(event);
      onChange();
    }, { passive: false });
    this.canvas.addEventListener("contextmenu", (event) => event.preventDefault());
  }

  _canvasPoint(event) {
    const rect = this.canvas.getBoundingClientRect();
    return {
      x: event.clientX - rect.left,
      y: event.clientY - rect.top,
      scale: Math.max(1, window.devicePixelRatio || 1),
    };
  }

  _buttonFromPointerEvent(event) {
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

  _modifierMask(event) {
    let mods = 0;
    if (event.shiftKey) mods |= 1;
    if (event.ctrlKey) mods |= 2;
    if (event.altKey) mods |= 4;
    if (event.metaKey) mods |= 8;
    return mods;
  }

  _diagnosticMessage(prefix) {
    const count = this.Module._dvz_wasm_api_diagnostic_count(this.scene);
    const messages = [];
    for (let i = 0; i < count; i++) {
      const ptr = this.Module._dvz_wasm_api_diagnostic(this.scene, i);
      messages.push(ptr !== 0 ? this.Module.UTF8ToString(ptr) : "<null diagnostic>");
    }
    return `${prefix}${messages.length > 0 ? `: ${messages.join("; ")}` : ""}`;
  }

  emit() {
    const status = this.Module._dvz_wasm_api_emit(this.scene, this.figure);
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_emit failed with ${status}`));
    }
    const ptr = this.Module._dvz_wasm_api_payload_ptr(this.scene);
    const size = this.Module._dvz_wasm_api_payload_size(this.scene);
    requireOk(ptr !== 0 && size > 0, "WASM scene emitted no payload");
    return JSON.parse(new TextDecoder().decode(this.Module.HEAPU8.subarray(ptr, ptr + size)));
  }

  async renderInitial() {
    const stream = this.emit();
    this.runtime = new Drp2WebGpuRuntime(this.gpu.device, this.gpu.context, this.gpu.format, {
      capabilities: this.gpu.capabilities,
    });
    await this.runtime.load(stream);
    await this.runtime.render();
    return stream;
  }

  async renderIncremental() {
    requireOk(this.runtime !== null, "renderInitial() must be called before renderIncremental()");
    const stream = this.emit();
    await executeDrp2StreamChecked(this.gpu.device, this.gpu.context, this.gpu.format, stream, {
      commands: stream.commands,
      state: this.runtime.state,
      validateCapabilities: false,
    });
    return stream;
  }
}

export class DatovizWasmVisualHandle {
  constructor(Module, handle) {
    this.Module = Module;
    this.handle = handle;
  }

  setF32(attr, values, itemCount) {
    const attrPtr = allocCString(this.Module, attr);
    const dataPtr = allocArray(this.Module, values);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_f32(this.handle, attrPtr, dataPtr, itemCount);
      requireOk(status === 0, `dvz_wasm_api_visual_set_f32(${attr}) failed with ${status}`);
    } finally {
      this.Module._free(attrPtr);
      this.Module._free(dataPtr);
    }
  }

  setRGBA8(attr, values, itemCount) {
    const attrPtr = allocCString(this.Module, attr);
    const dataPtr = allocArray(this.Module, values);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_rgba8(this.handle, attrPtr, dataPtr, itemCount);
      requireOk(status === 0, `dvz_wasm_api_visual_set_rgba8(${attr}) failed with ${status}`);
    } finally {
      this.Module._free(attrPtr);
      this.Module._free(dataPtr);
    }
  }

  setTextureRGBA8(values, width, height) {
    const dataPtr = allocArray(this.Module, values);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_texture_rgba8(this.handle, dataPtr, width, height);
      requireOk(status === 0, `dvz_wasm_api_visual_set_texture_rgba8 failed with ${status}`);
    } finally {
      this.Module._free(dataPtr);
    }
  }
}
