import {
  Drp2WebGpuRuntime,
  initWebGPU,
  resizeWebGpuCanvas,
} from "../drp2/webgpu.js";
import { mountDataBundles } from "./data_loader.js";

const DVZ_FORMAT_R16G16B16A16_SFLOAT = 97;
const DVZ_DIM_X = 0;
const DVZ_DIM_Y = 1;
const DVZ_DIM_MASK_XY = 3;
const DVZ_DIM_MASK_XYZ = 7;
const DVZ_CONTROLLER_TYPE_PANZOOM = 1;
const DVZ_CONTROLLER_TYPE_ARCBALL = 2;
const DVZ_POINTER_EVENT_RELEASE = 0;
const DVZ_POINTER_EVENT_PRESS = 1;
const DVZ_POINTER_EVENT_MOVE = 2;
const DVZ_POINTER_EVENT_CLICK = 3;
const DVZ_POINTER_CLICK_MAX_DISTANCE_PX = 4;
const DVZ_POINTER_BUTTON_NONE = 0;
const DVZ_POINTER_BUTTON_LEFT = 1;
const DVZ_POINTER_BUTTON_MIDDLE = 2;
const DVZ_POINTER_BUTTON_RIGHT = 3;
const DVZ_MATERIAL_MODEL_UNLIT = 0;
const DVZ_MATERIAL_MODEL_PHONG = 1;
const DVZ_MATERIAL_MODEL_STANDARD = 2;
const DVZ_SEGMENT_CAP_NONE = 0;
const DVZ_SEGMENT_CAP_ROUND = 1;
const DVZ_SEGMENT_CAP_TRIANGLE_IN = 2;
const DVZ_SEGMENT_CAP_TRIANGLE_OUT = 3;
const DVZ_SEGMENT_CAP_SQUARE = 4;
const DVZ_SEGMENT_CAP_BUTT = 5;
const DVZ_PATH_JOIN_MITER = 0;
const DVZ_PATH_JOIN_ROUND = 1;
const DVZ_PATH_JOIN_BEVEL = 2;
const DVZ_SCENE_BUFFER_USAGE_VERTEX = 1;
const DVZ_SCENE_BUFFER_USAGE_INDEX = 2;
const DVZ_SCENE_BUFFER_USAGE_UNIFORM = 4;
const DVZ_SCENE_BUFFER_USAGE_STORAGE = 8;

export const DvzWasmVisual = Object.freeze({
  point: 1,
  pixel: 2,
  marker: 3,
  segment: 4,
  path: 5,
  image: 6,
  mesh: 7,
  glyph: 8,
  primitive: 9,
  sphere: 10,
  text: 11,
  labels: 12,
});

function requireOk(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function dimCode(dim) {
  switch (dim) {
    case "x":
    case "X":
    case DVZ_DIM_X:
      return DVZ_DIM_X;
    case "y":
    case "Y":
    case DVZ_DIM_Y:
      return DVZ_DIM_Y;
    default:
      throw new Error(`unsupported scene dimension ${dim}`);
  }
}

function positiveInteger(value, fallback) {
  return Number.isFinite(value) && value > 0 ? Math.floor(value) : fallback;
}

function canvasLogicalSize(canvas) {
  const rect = canvas.getBoundingClientRect();
  return {
    width: positiveInteger(canvas.clientWidth, positiveInteger(rect.width, canvas.width)),
    height: positiveInteger(canvas.clientHeight, positiveInteger(rect.height, canvas.height)),
  };
}

function requestedLogicalSize(canvas, options = {}) {
  const fallback = canvasLogicalSize(canvas);
  return {
    width: positiveInteger(options.logicalWidth, fallback.width),
    height: positiveInteger(options.logicalHeight, fallback.height),
  };
}

function browserCapabilityArgs(capabilities = {}) {
  const sampleCounts = Array.isArray(capabilities.supported_sample_counts)
    ? capabilities.supported_sample_counts.filter((value) => Number.isFinite(value) && value > 0)
    : [];
  return {
    maxTextureDimension2d: positiveInteger(capabilities.max_texture_dimension_2d, 4096),
    maxBindGroups: positiveInteger(capabilities.max_bind_groups, 4),
    maxVertexBuffers: positiveInteger(capabilities.max_vertex_buffers, 8),
    maxBufferSize: positiveInteger(capabilities.max_buffer_size, 256 * 1024 * 1024),
    minTextureCopyBytesPerRowAlignment: positiveInteger(
      capabilities.min_texture_copy_bytes_per_row_alignment,
      256,
    ),
    maxSampleCount: Math.max(1, ...sampleCounts),
    supportsColorBlending: capabilities.supports_color_blending !== false,
  };
}

function materialModelCode(model) {
  switch (model) {
    case "unlit":
    case DVZ_MATERIAL_MODEL_UNLIT:
      return DVZ_MATERIAL_MODEL_UNLIT;
    case "phong":
    case undefined:
    case DVZ_MATERIAL_MODEL_PHONG:
      return DVZ_MATERIAL_MODEL_PHONG;
    case "standard":
    case DVZ_MATERIAL_MODEL_STANDARD:
      return DVZ_MATERIAL_MODEL_STANDARD;
    default:
      throw new Error(`unsupported material model ${model}`);
  }
}

function segmentCapCode(cap) {
  switch (cap) {
    case "none":
    case DVZ_SEGMENT_CAP_NONE:
      return DVZ_SEGMENT_CAP_NONE;
    case "round":
    case undefined:
    case DVZ_SEGMENT_CAP_ROUND:
      return DVZ_SEGMENT_CAP_ROUND;
    case "triangle-in":
    case "triangleIn":
    case DVZ_SEGMENT_CAP_TRIANGLE_IN:
      return DVZ_SEGMENT_CAP_TRIANGLE_IN;
    case "triangle-out":
    case "triangleOut":
    case DVZ_SEGMENT_CAP_TRIANGLE_OUT:
      return DVZ_SEGMENT_CAP_TRIANGLE_OUT;
    case "square":
    case DVZ_SEGMENT_CAP_SQUARE:
      return DVZ_SEGMENT_CAP_SQUARE;
    case "butt":
    case DVZ_SEGMENT_CAP_BUTT:
      return DVZ_SEGMENT_CAP_BUTT;
    default:
      throw new Error(`unsupported segment cap ${cap}`);
  }
}

function pathJoinCode(join) {
  switch (join) {
    case "miter":
    case DVZ_PATH_JOIN_MITER:
      return DVZ_PATH_JOIN_MITER;
    case "round":
    case undefined:
    case DVZ_PATH_JOIN_ROUND:
      return DVZ_PATH_JOIN_ROUND;
    case "bevel":
    case DVZ_PATH_JOIN_BEVEL:
      return DVZ_PATH_JOIN_BEVEL;
    default:
      throw new Error(`unsupported path join ${join}`);
  }
}

function sceneBufferUsageCode(usage = "vertex") {
  if (Number.isFinite(usage)) {
    return Math.floor(usage);
  }
  const parts = Array.isArray(usage)
    ? usage
    : String(usage).split(/[|,+\s]+/).filter((part) => part.length > 0);
  let code = 0;
  for (const part of parts) {
    switch (part) {
      case "vertex":
        code |= DVZ_SCENE_BUFFER_USAGE_VERTEX;
        break;
      case "index":
        code |= DVZ_SCENE_BUFFER_USAGE_INDEX;
        break;
      case "uniform":
        code |= DVZ_SCENE_BUFFER_USAGE_UNIFORM;
        break;
      case "storage":
        code |= DVZ_SCENE_BUFFER_USAGE_STORAGE;
        break;
      default:
        throw new Error(`unsupported scene buffer usage ${part}`);
    }
  }
  return code;
}

function wasmModuleUrl() {
  const url = new URL("../../build-wasm-scene/wasm/datoviz_wasm_scene.mjs", import.meta.url);
  url.searchParams.set("v", Date.now().toString());
  return url;
}

async function loadDatovizWasmModule() {
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
  return Module;
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

function allocCStringArray(Module, values) {
  requireOk(values.length > 0, "WASM string array must not be empty");
  const stringPtrs = values.map((value) => allocCString(Module, value));
  const arrayPtr = Module._malloc(stringPtrs.length * 4);
  if (arrayPtr === 0) {
    for (const ptr of stringPtrs) {
      Module._free(ptr);
    }
    throw new Error("WASM allocation failed");
  }
  Module.HEAPU32.set(stringPtrs, arrayPtr / 4);
  return { arrayPtr, stringPtrs };
}

function diagnosticMessage(Module, scene, prefix) {
  const count = Module._dvz_wasm_api_diagnostic_count(scene);
  const messages = [];
  for (let i = 0; i < count; i++) {
    const ptr = Module._dvz_wasm_api_diagnostic(scene, i);
    messages.push(ptr !== 0 ? Module.UTF8ToString(ptr) : "<null diagnostic>");
  }
  return `${prefix}${messages.length > 0 ? `: ${messages.join("; ")}` : ""}`;
}

function decodeBase64(data) {
  const binary = atob(data);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes;
}

function hasPacketApi(Module) {
  return (
    typeof Module._dvz_wasm_api_emit_packets === "function" &&
    typeof Module._dvz_wasm_api_packet_ptr === "function" &&
    typeof Module._dvz_wasm_api_packet_size === "function" &&
    typeof Module._dvz_wasm_api_packet_arena_ptr === "function" &&
    typeof Module._dvz_wasm_api_packet_arena_size === "function" &&
    typeof Module._dvz_wasm_api_packet_status === "function" &&
    typeof Module._dvz_wasm_api_release_packets === "function"
  );
}

function frameArtifactPacketSet(setup, update, frame, resourceVersion, frameIndex) {
  return {
    setup,
    update,
    frame,
    resource_version: resourceVersion,
    frame_index: frameIndex,
    source: "wasm_frame_artifact",
    artifact_resource_version: resourceVersion,
    artifact_frame_index: frameIndex,
    artifact_spans_copied: true,
    artifact_released: false,
  };
}

export class DatovizWasmScene {
  static async create(canvas, options = {}) {
    const Module = await loadDatovizWasmModule();

    const gpu = options.gpu ?? await initWebGPU(canvas);
    resizeWebGpuCanvas(canvas, gpu.device, gpu.context, gpu.format);
    const logical = requestedLogicalSize(canvas, options);
    const scene = Module._dvz_wasm_api_scene(logical.width, logical.height);
    requireOk(scene !== 0, "dvz_wasm_api_scene failed");
    const formatStatus = Module._dvz_wasm_api_set_canvas_format(
      scene, DVZ_FORMAT_R16G16B16A16_SFLOAT);
    if (formatStatus !== 0) {
      throw new Error(diagnosticMessage(Module, scene, "scene rejected linear browser presentation format"));
    }
    const caps = browserCapabilityArgs(gpu.capabilities);
    const capsStatus = Module._dvz_wasm_api_set_capabilities(
      scene,
      caps.maxTextureDimension2d,
      caps.maxBindGroups,
      caps.maxVertexBuffers,
      caps.maxBufferSize,
      caps.minTextureCopyBytesPerRowAlignment,
      caps.maxSampleCount,
      caps.supportsColorBlending ? 1 : 0,
    );
    if (capsStatus !== 0) {
      throw new Error(diagnosticMessage(Module, scene, "scene rejected browser capabilities"));
    }
    const figure = Module._dvz_wasm_api_figure(scene, logical.width, logical.height);
    if (figure === 0) {
      throw new Error(diagnosticMessage(Module, scene, "dvz_wasm_api_figure failed"));
    }
    return new DatovizWasmScene(Module, gpu, canvas, scene, figure, logical);
  }

  static async createScenario(canvas, scenarioId, options = {}) {
    const Module = await loadDatovizWasmModule();
    await mountDataBundles(Module, options.dataBundles ?? []);
    requireOk(
      typeof Module._dvz_wasm_api_scenario_count === "function" &&
        typeof Module._dvz_wasm_api_scenario_create === "function" &&
        typeof Module._dvz_wasm_api_scenario_figure === "function" &&
        typeof Module._dvz_wasm_api_scenario_frame === "function" &&
        typeof Module._dvz_wasm_api_scenario_post_frame === "function" &&
        typeof Module._dvz_wasm_api_scenario_pointer === "function" &&
        typeof Module._dvz_wasm_api_scenario_wheel === "function",
      "WASM module is stale or missing scenario exports; run `just wasm-scene-smoke` and hard-refresh",
    );

    const gpu = options.gpu ?? await initWebGPU(canvas);
    resizeWebGpuCanvas(canvas, gpu.device, gpu.context, gpu.format);
    const logical = requestedLogicalSize(canvas, options);
    const scene = Module._dvz_wasm_api_scene(logical.width, logical.height);
    requireOk(scene !== 0, "dvz_wasm_api_scene failed");
    const formatStatus = Module._dvz_wasm_api_set_canvas_format(
      scene, DVZ_FORMAT_R16G16B16A16_SFLOAT);
    if (formatStatus !== 0) {
      throw new Error(diagnosticMessage(Module, scene, "scene rejected linear browser presentation format"));
    }
    const caps = browserCapabilityArgs(gpu.capabilities);
    const capsStatus = Module._dvz_wasm_api_set_capabilities(
      scene,
      caps.maxTextureDimension2d,
      caps.maxBindGroups,
      caps.maxVertexBuffers,
      caps.maxBufferSize,
      caps.minTextureCopyBytesPerRowAlignment,
      caps.maxSampleCount,
      caps.supportsColorBlending ? 1 : 0,
    );
    if (capsStatus !== 0) {
      throw new Error(diagnosticMessage(Module, scene, "scene rejected browser capabilities"));
    }

    const count = Module._dvz_wasm_api_scenario_count();
    let index = -1;
    for (let i = 0; i < count; i++) {
      const ptr = Module._dvz_wasm_api_scenario_id(i);
      const id = ptr !== 0 ? Module.UTF8ToString(ptr) : "";
      if (id === scenarioId) {
        index = i;
        break;
      }
    }
    requireOk(index >= 0, `unknown WASM scenario ${scenarioId}`);
    const status = Module._dvz_wasm_api_scenario_create(scene, index);
    if (status !== 0) {
      throw new Error(diagnosticMessage(Module, scene, `dvz_wasm_api_scenario_create failed with ${status}`));
    }
    const figure = Module._dvz_wasm_api_scenario_figure(scene);
    if (figure === 0) {
      throw new Error(diagnosticMessage(Module, scene, "dvz_wasm_api_scenario_figure failed"));
    }
    const wrapper = new DatovizWasmScene(Module, gpu, canvas, scene, figure, logical);
    wrapper.scenario = {
      id: scenarioId,
      index,
      fps: Module._dvz_wasm_api_scenario_fps?.(index) ?? 60,
    };
    wrapper.resize();
    return wrapper;
  }

  constructor(Module, gpu, canvas, scene, figure, logicalSize = null) {
    this.Module = Module;
    this.gpu = gpu;
    this.canvas = canvas;
    this.scene = scene;
    this.figure = figure;
    this.logicalSize = logicalSize;
    this.runtime = null;
    this.scenario = null;
    this._cleanup = [];
    this._runtimeExecution = Promise.resolve();
    this._lastResize = null;
  }

  destroy() {
    for (const cleanup of this._cleanup.splice(0).reverse()) {
      cleanup();
    }
    if (this.runtime !== null) {
      this.runtime.destroy();
    }
    this.runtime = null;
    this.figure = 0;
    if (this.scene !== 0) {
      this.Module._dvz_wasm_api_scene_destroy(this.scene);
      this.scene = 0;
    }
  }

  panelFull() {
    this._requireAlive();
    const panel = this.Module._dvz_wasm_api_panel_full(this.figure);
    return this._requireHandle(panel, "dvz_wasm_api_panel_full failed");
  }

  visual(type, flags = 0) {
    this._requireAlive();
    const visualType = typeof type === "string" ? DvzWasmVisual[type] : type;
    requireOk(visualType !== undefined, `unknown visual type ${type}`);
    const visual = this.Module._dvz_wasm_api_visual(this.scene, visualType, flags);
    this._requireHandle(visual, `dvz_wasm_api_visual(${type}) failed`);
    return new DatovizWasmVisualHandle(this.Module, this.scene, visual);
  }

  buffer(options = {}) {
    this._requireAlive();
    const usage = sceneBufferUsageCode(options.usage ?? "vertex");
    const stride = positiveInteger(options.stride, 0);
    const byteSize = positiveInteger(options.byteSize ?? options.byte_size, 0);
    requireOk(stride > 0, "WASM scene buffer stride must be positive");
    const buffer = this.Module._dvz_wasm_api_buffer(this.scene, usage, stride, byteSize);
    this._requireHandle(buffer, "dvz_wasm_api_buffer failed");
    return new DatovizWasmBufferHandle(this.Module, this.scene, buffer);
  }

  addVisual(panel, visual) {
    this._requireAlive();
    const visualHandle = visual instanceof DatovizWasmVisualHandle ? visual.handle : visual;
    this._requireStatus(
      this.Module._dvz_wasm_api_panel_add_visual(panel, visualHandle),
      "dvz_wasm_api_panel_add_visual failed",
    );
  }

  attachPanzoom(panel) {
    this._requireAlive();
    const controller = this.Module._dvz_wasm_api_controller(this.scene, DVZ_CONTROLLER_TYPE_PANZOOM);
    this._requireHandle(controller, "dvz_wasm_api_controller(panzoom) failed");
    this._requireStatus(
      this.Module._dvz_wasm_api_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY),
      "dvz_wasm_api_panel_bind_controller(panzoom) failed",
    );
    return controller;
  }

  attachArcball(panel, options = {}) {
    this._requireAlive();
    const controller = this.Module._dvz_wasm_api_controller(this.scene, DVZ_CONTROLLER_TYPE_ARCBALL);
    this._requireHandle(controller, "dvz_wasm_api_controller(arcball) failed");
    this._requireStatus(
      this.Module._dvz_wasm_api_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ),
      "dvz_wasm_api_panel_bind_controller(arcball) failed",
    );
    const initial = options.initial ?? [0.45, -0.65, 0.2];
    this._requireStatus(
      this.Module._dvz_wasm_api_arcball_initial(controller, initial[0], initial[1], initial[2]),
      "dvz_wasm_api_arcball_initial failed",
    );
    return controller;
  }

  setCamera(panel, options = {}) {
    this._requireAlive();
    const eye = options.eye ?? [0, 0, 3];
    const target = options.target ?? [0, 0, 0];
    this._requireStatus(
      this.Module._dvz_wasm_api_panel_set_camera(
        panel,
        eye[0],
        eye[1],
        eye[2],
        target[0],
        target[1],
        target[2],
        options.fovY ?? Math.PI / 4,
        options.near ?? 0.1,
        options.far ?? 100,
      ),
      "dvz_wasm_api_panel_set_camera failed",
    );
  }

  setDomain(panel, dim, min, max) {
    this._requireAlive();
    this._requireStatus(
      this.Module._dvz_wasm_api_panel_set_domain(panel, dimCode(dim), min, max),
      "dvz_wasm_api_panel_set_domain failed",
    );
  }

  axis(panel, dim) {
    this._requireAlive();
    const axis = this.Module._dvz_wasm_api_panel_axis(panel, dimCode(dim));
    this._requireHandle(axis, "dvz_wasm_api_panel_axis failed");
    return new DatovizWasmAxisHandle(this.Module, this.scene, axis);
  }

  resize() {
    this._requireAlive();
    const resized = resizeWebGpuCanvas(this.canvas, this.gpu.device, this.gpu.context, this.gpu.format);
    const logical = this.logicalSize ?? canvasLogicalSize(this.canvas);
    const scale = Math.max(1, window.devicePixelRatio || 1);
    const next = {
      logicalWidth: logical.width,
      logicalHeight: logical.height,
      framebufferWidth: this.canvas.width,
      framebufferHeight: this.canvas.height,
      scale,
    };
    if (
      !resized &&
      this._lastResize !== null &&
      this._lastResize.logicalWidth === next.logicalWidth &&
      this._lastResize.logicalHeight === next.logicalHeight &&
      this._lastResize.framebufferWidth === next.framebufferWidth &&
      this._lastResize.framebufferHeight === next.framebufferHeight &&
      this._lastResize.scale === next.scale
    ) {
      return false;
    }
    this._lastResize = next;
    this._requireStatus(
      this.Module._dvz_wasm_api_resize(
        this.scene,
        this.figure,
        next.logicalWidth,
        next.logicalHeight,
        next.framebufferWidth,
        next.framebufferHeight,
        scale,
      ),
      "dvz_wasm_api_resize failed",
    );
    return true;
  }

  pointer(type, event) {
    this._requireAlive();
    const point = this._canvasPoint(event);
    this._requireStatus(
      this.Module._dvz_wasm_api_pointer(
        this.scene,
        type,
        point.x,
        point.y,
        this._buttonFromPointerEvent(event),
        this._modifierMask(event),
        point.scale,
        performance.now(),
      ),
      "dvz_wasm_api_pointer failed",
    );
  }

  scenarioPointer(type, event) {
    this._requireAlive();
    requireOk(this.scenario !== null, "WASM scene has no active scenario");
    const point = this._canvasPoint(event);
    this._requireStatus(
      this.Module._dvz_wasm_api_scenario_pointer(
        this.scene,
        type,
        point.x,
        point.y,
        this._buttonFromPointerEvent(event),
        this._modifierMask(event),
        point.scale,
        performance.now(),
      ),
      "dvz_wasm_api_scenario_pointer failed",
    );
  }

  wheel(event) {
    this._requireAlive();
    const point = this._canvasPoint(event);
    this._requireStatus(
      this.Module._dvz_wasm_api_wheel(
        this.scene,
        point.x,
        point.y,
        0,
        -event.deltaY / 100,
        this._modifierMask(event),
        point.scale,
        performance.now(),
      ),
      "dvz_wasm_api_wheel failed",
    );
  }

  scenarioWheel(event) {
    this._requireAlive();
    requireOk(this.scenario !== null, "WASM scene has no active scenario");
    const point = this._canvasPoint(event);
    this._requireStatus(
      this.Module._dvz_wasm_api_scenario_wheel(
        this.scene,
        point.x,
        point.y,
        0,
        -event.deltaY / 100,
        this._modifierMask(event),
        point.scale,
        performance.now(),
      ),
      "dvz_wasm_api_scenario_wheel failed",
    );
  }

  scenarioFrame(timeSeconds, dtSeconds) {
    this._requireAlive();
    requireOk(this.scenario !== null, "WASM scene has no active scenario");
    this._requireStatus(
      this.Module._dvz_wasm_api_scenario_frame(this.scene, timeSeconds, dtSeconds),
      "dvz_wasm_api_scenario_frame failed",
    );
  }

  scenarioPostFrame() {
    this._requireAlive();
    requireOk(this.scenario !== null, "WASM scene has no active scenario");
    this._requireStatus(
      this.Module._dvz_wasm_api_scenario_post_frame(this.scene),
      "dvz_wasm_api_scenario_post_frame failed",
    );
  }

  attachControllerInput(onChange) {
    this._requireAlive();
    let activePointer = null;
    let suppressedClick = null;
    const route = (event, type) => {
      event.preventDefault();
      this.pointer(type, event);
      if (this.scenario !== null) {
        this.scenarioPointer(type, event);
      }
      onChange();
    };
    const onPointerDown = (event) => {
      activePointer = {
        id: event.pointerId,
        x: event.clientX,
        y: event.clientY,
        maxDistanceSquared: 0,
      };
      suppressedClick = null;
      this.canvas.setPointerCapture(event.pointerId);
      route(event, DVZ_POINTER_EVENT_PRESS);
    };
    const updatePointerDistance = (event) => {
      if (activePointer === null || event.pointerId !== activePointer.id) {
        return;
      }
      const dx = event.clientX - activePointer.x;
      const dy = event.clientY - activePointer.y;
      activePointer.maxDistanceSquared = Math.max(
        activePointer.maxDistanceSquared,
        dx * dx + dy * dy,
      );
    };
    const onPointerMove = (event) => {
      updatePointerDistance(event);
      route(event, DVZ_POINTER_EVENT_MOVE);
    };
    const onPointerUp = (event) => {
      updatePointerDistance(event);
      route(event, DVZ_POINTER_EVENT_RELEASE);
      if (activePointer !== null && event.pointerId === activePointer.id) {
        const thresholdSquared = DVZ_POINTER_CLICK_MAX_DISTANCE_PX ** 2;
        if (activePointer.maxDistanceSquared > thresholdSquared) {
          suppressedClick = {
            button: event.button,
            x: event.clientX,
            y: event.clientY,
            expires: performance.now() + 1000,
          };
        }
        activePointer = null;
      }
      if (this.canvas.hasPointerCapture(event.pointerId)) {
        this.canvas.releasePointerCapture(event.pointerId);
      }
    };
    const onPointerCancel = (event) => {
      route(event, DVZ_POINTER_EVENT_RELEASE);
      if (activePointer !== null && event.pointerId === activePointer.id) {
        activePointer = null;
      }
    };
    const onClick = (event) => {
      if (this.scenario === null) {
        return;
      }
      event.preventDefault();
      if (suppressedClick !== null) {
        const click = suppressedClick;
        suppressedClick = null;
        const dx = event.clientX - click.x;
        const dy = event.clientY - click.y;
        const thresholdSquared = DVZ_POINTER_CLICK_MAX_DISTANCE_PX ** 2;
        if (
          performance.now() <= click.expires &&
          event.button === click.button &&
          dx * dx + dy * dy <= thresholdSquared
        ) {
          return;
        }
      }
      this.scenarioPointer(DVZ_POINTER_EVENT_CLICK, event);
      onChange();
    };
    const onWheel = (event) => {
      event.preventDefault();
      this.wheel(event);
      if (this.scenario !== null) {
        this.scenarioWheel(event);
      }
      onChange();
    };
    const onContextMenu = (event) => event.preventDefault();
    this.canvas.addEventListener("pointerdown", onPointerDown);
    this.canvas.addEventListener("pointermove", onPointerMove);
    this.canvas.addEventListener("pointerup", onPointerUp);
    this.canvas.addEventListener("pointercancel", onPointerCancel);
    this.canvas.addEventListener("click", onClick);
    this.canvas.addEventListener("wheel", onWheel, { passive: false });
    this.canvas.addEventListener("contextmenu", onContextMenu);
    const detach = () => {
      this.canvas.removeEventListener("pointerdown", onPointerDown);
      this.canvas.removeEventListener("pointermove", onPointerMove);
      this.canvas.removeEventListener("pointerup", onPointerUp);
      this.canvas.removeEventListener("pointercancel", onPointerCancel);
      this.canvas.removeEventListener("click", onClick);
      this.canvas.removeEventListener("wheel", onWheel);
      this.canvas.removeEventListener("contextmenu", onContextMenu);
    };
    this._cleanup.push(detach);
    return detach;
  }

  attachPanzoomInput(onChange) {
    return this.attachControllerInput(onChange);
  }

  attachResizeObserver(onChange) {
    this._requireAlive();
    const observed = this.canvas.parentElement ?? this.canvas;
    let lastWidth = this.canvas.clientWidth;
    let lastHeight = this.canvas.clientHeight;
    let lastScale = Math.max(1, window.devicePixelRatio || 1);
    const observer = new ResizeObserver(() => {
      const width = this.canvas.clientWidth;
      const height = this.canvas.clientHeight;
      const scale = Math.max(1, window.devicePixelRatio || 1);
      if (width === lastWidth && height === lastHeight && scale === lastScale) {
        return;
      }
      lastWidth = width;
      lastHeight = height;
      lastScale = scale;
      onChange();
    });
    observer.observe(observed);
    const detach = () => observer.disconnect();
    this._cleanup.push(detach);
    return detach;
  }

  _canvasPoint(event, framebuffer = false) {
    const rect = this.canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;
    if (framebuffer) {
      const scaleX = rect.width > 0 ? this.canvas.width / rect.width : 1;
      const scaleY = rect.height > 0 ? this.canvas.height / rect.height : 1;
      return {
        x: x * scaleX,
        y: y * scaleY,
        scale: Math.max(scaleX, scaleY, 1),
      };
    }
    const logical = this.logicalSize ?? canvasLogicalSize(this.canvas);
    const logicalScaleX = rect.width > 0 ? logical.width / rect.width : 1;
    const logicalScaleY = rect.height > 0 ? logical.height / rect.height : 1;
    return {
      x: x * logicalScaleX,
      y: y * logicalScaleY,
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
    return diagnosticMessage(this.Module, this.scene, prefix);
  }

  _queryDiagnosticMessage(prefix) {
    const Module = this.Module;
    const pending = Module._dvz_wasm_api_query_pending_count?.(this.scene) ?? "<missing>";
    const active = Module._dvz_wasm_api_query_active?.(this.scene) ?? "<missing>";
    const readbackSize = Module._dvz_wasm_api_query_readback_size?.(this.scene) ?? "<missing>";
    const packetStatus = Module._dvz_wasm_api_packet_status?.(this.scene) ?? "<missing>";
    return this._diagnosticMessage(
      `${prefix}: pending=${pending} active=${active} readback_size=${readbackSize} packet_status=${packetStatus}`,
    );
  }

  _requireAlive() {
    requireOk(this.scene !== 0, "WASM scene has been destroyed");
  }

  _requireHandle(handle, label) {
    if (handle === 0) {
      throw new Error(this._diagnosticMessage(label));
    }
    return handle;
  }

  _requireStatus(status, label) {
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`${label} with ${status}`));
    }
  }

  emitDebugJson() {
    this._requireAlive();
    const status = this.Module._dvz_wasm_api_emit(this.scene, this.figure);
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_emit failed with ${status}`));
    }
    const ptr = this.Module._dvz_wasm_api_payload_ptr(this.scene);
    const size = this.Module._dvz_wasm_api_payload_size(this.scene);
    requireOk(ptr !== 0 && size > 0, "WASM scene emitted no payload");
    const json = JSON.parse(new TextDecoder().decode(this.Module.HEAPU8.subarray(ptr, ptr + size)));
    if (typeof this.Module._dvz_wasm_api_release_packets === "function") {
      this._requireStatus(
        this.Module._dvz_wasm_api_release_packets(this.scene),
        "dvz_wasm_api_release_packets failed",
      );
    }
    return json;
  }

  _currentPacketSet() {
    const packetStatus = this.Module._dvz_wasm_api_packet_status(this.scene);
    if (packetStatus !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_packet_status returned ${packetStatus}`));
    }
    const span = (kind) => {
      const packetPtr = this.Module._dvz_wasm_api_packet_ptr(this.scene, kind);
      const packetSize = this.Module._dvz_wasm_api_packet_size(this.scene, kind);
      const arenaPtr = this.Module._dvz_wasm_api_packet_arena_ptr(this.scene, kind);
      const arenaSize = this.Module._dvz_wasm_api_packet_arena_size(this.scene, kind);
      return {
        packet: packetPtr !== 0
          ? this.Module.HEAPU8.subarray(packetPtr, packetPtr + packetSize).slice()
          : new Uint8Array(),
        arena: arenaPtr !== 0
          ? this.Module.HEAPU8.subarray(arenaPtr, arenaPtr + arenaSize).slice()
          : new Uint8Array(),
      };
    };
    const resourceVersion = this.Module._dvz_wasm_api_resource_version?.(this.scene) ?? 0;
    const frameIndex = this.Module._dvz_wasm_api_frame_index?.(this.scene) ?? 0;
    return frameArtifactPacketSet(span(1), span(2), span(3), resourceVersion, frameIndex);
  }

  _releaseCurrentArtifact(packetSet) {
    this._requireStatus(
      this.Module._dvz_wasm_api_release_packets(this.scene),
      "dvz_wasm_api_release_packets failed",
    );
    packetSet.artifact_released = true;
    return packetSet;
  }

  emitPackets() {
    this._requireAlive();
    requireOk(
      hasPacketApi(this.Module),
      "WASM module is stale or missing split packet exports; run `just wasm-scene-smoke` and hard-refresh",
    );
    const status = this.Module._dvz_wasm_api_emit_packets(this.scene, this.figure);
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_emit_packets failed with ${status}`));
    }
    const packetSet = this._currentPacketSet();
    return this._releaseCurrentArtifact(packetSet);
  }

  _packetExecutionContext(label, packetSet) {
    return (
      `${label}: packet_resource=${packetSet.resource_version} ` +
      `packet_frame=${packetSet.frame_index} ` +
      `runtime_resource=${this.runtime?.packetResourceVersion ?? "<none>"} ` +
      `runtime_frame=${this.runtime?.packetFrameIndex ?? "<none>"}`
    );
  }

  async _executePacketSet(label, packetSet, options = {}) {
    requireOk(this.runtime !== null, "WASM scene runtime has not been created");
    const execute = this._runtimeExecution.then(async () => {
      try {
        return await this.runtime.executePacketSet(packetSet, options);
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        throw new Error(`${message}: ${this._packetExecutionContext(label, packetSet)}`);
      }
    });
    this._runtimeExecution = execute.catch(() => {});
    return await execute;
  }

  async flushScenarioQueries() {
    this._requireAlive();
    if (this.scenario === null || this.runtime === null) {
      return 0;
    }
    const Module = this.Module;
    if (
      typeof Module._dvz_wasm_api_query_pending_count !== "function" ||
      typeof Module._dvz_wasm_api_emit_query_packets !== "function" ||
      typeof Module._dvz_wasm_api_query_active !== "function" ||
      typeof Module._dvz_wasm_api_query_readback_size !== "function" ||
      typeof Module._dvz_wasm_api_query_resolve !== "function"
    ) {
      throw new Error("WASM module is stale or missing query exports; run `just wasm-scene-smoke` and hard-refresh");
    }

    let processed = 0;
    const maxPasses = 8;
    for (let pass = 0; pass < maxPasses; pass++) {
      if (Module._dvz_wasm_api_query_pending_count(this.scene) === 0) {
        return processed;
      }
      const status = Module._dvz_wasm_api_emit_query_packets(this.scene, this.figure);
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_emit_query_packets failed with ${status}`));
      }
      if (Module._dvz_wasm_api_query_active(this.scene) === 0) {
        if ((Module._dvz_wasm_api_diagnostic_count?.(this.scene) ?? 0) > 0) {
          throw new Error(this._queryDiagnosticMessage("WASM query setup completed without readback"));
        }
        processed++;
        continue;
      }

      const packetSet = this._releaseCurrentArtifact(this._currentPacketSet());
      const result = await this._executePacketSet("query", packetSet);
      const readback = result.readbacks?.[0] ?? null;
      const expectedSize = Module._dvz_wasm_api_query_readback_size(this.scene);
      if (readback === null || expectedSize === 0) {
        throw new Error(this._queryDiagnosticMessage("WASM query packet produced no readback"));
      }
      const bytes = decodeBase64(readback.data);
      requireOk(
        bytes.byteLength >= expectedSize,
        this._queryDiagnosticMessage(
          `WASM query readback is shorter than expected: bytes=${bytes.byteLength}`,
        ),
      );
      const ptr = Module._malloc(expectedSize);
      requireOk(ptr !== 0, this._queryDiagnosticMessage("WASM query readback allocation failed"));
      try {
        Module.HEAPU8.set(bytes.subarray(0, expectedSize), ptr);
        this._requireStatus(
          Module._dvz_wasm_api_query_resolve(this.scene, ptr, expectedSize),
          "dvz_wasm_api_query_resolve failed",
        );
      } finally {
        Module._free(ptr);
      }
      processed++;
    }
    throw new Error("WASM scenario query drain exceeded the bounded pass count");
  }

  async renderInitial() {
    this._requireAlive();
    const packetSet = this.emitPackets();
    if (this.runtime !== null) {
      this.runtime.destroy();
    }
    this.runtime = new Drp2WebGpuRuntime(this.gpu.device, this.gpu.context, this.gpu.format, {
      canvas: this.canvas,
      capabilities: this.gpu.capabilities,
      browserPresentFormat: "rgba16float",
    });
    this._runtimeExecution = Promise.resolve();
    await this._executePacketSet("initial", packetSet, { reset: true, replaceExistingResources: false });
    const stream = this.runtime.stream;
    await this.flushScenarioQueries();
    if (this.scenario !== null) {
      this.scenarioPostFrame();
    }
    return stream;
  }

  async recoverRuntime() {
    this._requireAlive();
    requireOk(
      typeof this.Module._dvz_wasm_api_runtime_reset === "function",
      "WASM module is stale or missing runtime reset export; run `just wasm-scene-smoke` and hard-refresh",
    );
    if (this.runtime !== null) {
      this.runtime.destroy();
      this.runtime = null;
    }
    this._runtimeExecution = Promise.resolve();
    this._requireStatus(
      this.Module._dvz_wasm_api_runtime_reset(this.scene),
      "dvz_wasm_api_runtime_reset failed",
    );
    return await this.renderInitial();
  }

  async renderIncremental() {
    this._requireAlive();
    requireOk(this.runtime !== null, "renderInitial() must be called before renderIncremental()");
    const packetSet = this.emitPackets();
    await this._executePacketSet("render", packetSet);
    const stream = this.runtime.stream;
    await this.flushScenarioQueries();
    if (this.scenario !== null) {
      this.scenarioPostFrame();
    }
    return stream;
  }
}

export class DatovizWasmBufferHandle {
  constructor(Module, scene, handle) {
    this.Module = Module;
    this.scene = scene;
    this.handle = handle;
  }

  _diagnosticMessage(prefix) {
    return diagnosticMessage(this.Module, this.scene, prefix);
  }

  setData(values) {
    const dataPtr = allocArray(this.Module, values);
    try {
      const status = this.Module._dvz_wasm_api_buffer_set_data(this.handle, dataPtr, values.byteLength);
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_buffer_set_data failed with ${status}`));
      }
    } finally {
      this.Module._free(dataPtr);
    }
  }
}

export class DatovizWasmAxisHandle {
  constructor(Module, scene, handle) {
    this.Module = Module;
    this.scene = scene;
    this.handle = handle;
  }

  _diagnosticMessage(prefix) {
    return diagnosticMessage(this.Module, this.scene, prefix);
  }

  setVisible(visible) {
    const status = this.Module._dvz_wasm_api_axis_set_visible(this.handle, visible ? 1 : 0);
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_axis_set_visible failed with ${status}`));
    }
  }

  setGrid(visible) {
    const status = this.Module._dvz_wasm_api_axis_set_grid(this.handle, visible ? 1 : 0);
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_axis_set_grid failed with ${status}`));
    }
  }

  setLabel(label) {
    const labelPtr = allocCString(this.Module, label);
    try {
      const status = this.Module._dvz_wasm_api_axis_set_label(this.handle, labelPtr);
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_axis_set_label failed with ${status}`));
      }
    } finally {
      this.Module._free(labelPtr);
    }
  }

  setPlotMargins(left, right, bottom, top) {
    const status = this.Module._dvz_wasm_api_axis_set_plot_margins(
      this.handle,
      left,
      right,
      bottom,
      top,
    );
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_axis_set_plot_margins failed with ${status}`));
    }
  }
}

export class DatovizWasmVisualHandle {
  constructor(Module, scene, handle) {
    this.Module = Module;
    this.scene = scene;
    this.handle = handle;
  }

  _diagnosticMessage(prefix) {
    return diagnosticMessage(this.Module, this.scene, prefix);
  }

  setF32(attr, values, itemCount) {
    const attrPtr = allocCString(this.Module, attr);
    const dataPtr = allocArray(this.Module, values);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_f32(this.handle, attrPtr, dataPtr, itemCount);
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_f32(${attr}) failed with ${status}`));
      }
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
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_rgba8(${attr}) failed with ${status}`));
      }
    } finally {
      this.Module._free(attrPtr);
      this.Module._free(dataPtr);
    }
  }

  setU32(attr, values, itemCount) {
    const attrPtr = allocCString(this.Module, attr);
    const dataPtr = allocArray(this.Module, values);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_u32(this.handle, attrPtr, dataPtr, itemCount);
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_u32(${attr}) failed with ${status}`));
      }
    } finally {
      this.Module._free(attrPtr);
      this.Module._free(dataPtr);
    }
  }

  setStrings(attr, values) {
    const attrPtr = allocCString(this.Module, attr);
    const { arrayPtr, stringPtrs } = allocCStringArray(this.Module, values);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_strings(
        this.handle,
        attrPtr,
        arrayPtr,
        values.length,
      );
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_strings(${attr}) failed with ${status}`));
      }
    } finally {
      this.Module._free(attrPtr);
      this.Module._free(arrayPtr);
      for (const ptr of stringPtrs) {
        this.Module._free(ptr);
      }
    }
  }

  setAttrBuffer(attr, buffer, itemCount, byteOffset = 0) {
    requireOk(buffer instanceof DatovizWasmBufferHandle, "setAttrBuffer() requires a WASM buffer handle");
    const attrPtr = allocCString(this.Module, attr);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_attr_buffer(
        this.handle,
        attrPtr,
        buffer.handle,
        byteOffset,
        itemCount,
      );
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_attr_buffer(${attr}) failed with ${status}`));
      }
    } finally {
      this.Module._free(attrPtr);
    }
  }

  setTextureRGBA8(values, width, height) {
    const dataPtr = allocArray(this.Module, values);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_texture_rgba8(this.handle, dataPtr, width, height);
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_texture_rgba8 failed with ${status}`));
      }
    } finally {
      this.Module._free(dataPtr);
    }
  }

  setLabelsS32(values, width, height, categories) {
    requireOk(values instanceof Int32Array, "setLabelsS32() values must be an Int32Array");
    requireOk(Number.isInteger(width) && width > 0, "setLabelsS32() width must be positive");
    requireOk(Number.isInteger(height) && height > 0, "setLabelsS32() height must be positive");
    requireOk(values.length === width * height, "setLabelsS32() values length must match width*height");
    requireOk(Array.isArray(categories) && categories.length > 0, "setLabelsS32() requires categories");

    const categoryIds = new Int32Array(categories.length);
    const colors = new Uint8Array(categories.length * 4);
    for (let i = 0; i < categories.length; i++) {
      const category = categories[i] ?? {};
      requireOk(Number.isInteger(category.id), `setLabelsS32() category ${i} needs an integer id`);
      const color = category.color ?? category.rgba;
      requireOk(Array.isArray(color) && color.length >= 4, `setLabelsS32() category ${i} needs RGBA color`);
      categoryIds[i] = category.id;
      for (let c = 0; c < 4; c++) {
        colors[4 * i + c] = Math.max(0, Math.min(255, Math.round(color[c])));
      }
    }

    const valuesPtr = allocArray(this.Module, values);
    const idsPtr = allocArray(this.Module, categoryIds);
    const colorsPtr = allocArray(this.Module, colors);
    try {
      const status = this.Module._dvz_wasm_api_visual_set_labels_s32(
        this.handle,
        valuesPtr,
        width,
        height,
        idsPtr,
        colorsPtr,
        categories.length,
      );
      if (status !== 0) {
        throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_labels_s32 failed with ${status}`));
      }
    } finally {
      this.Module._free(valuesPtr);
      this.Module._free(idsPtr);
      this.Module._free(colorsPtr);
    }
  }

  setMaterial(options = {}) {
    const model = materialModelCode(options.model);
    const base = options.baseColorFactor ?? [1, 1, 1, 1];
    const light = options.lightDirection ?? [-0.45, -0.35, 0.82];
    const phong = options.phong ?? {};
    const standard = options.standard ?? {};
    const emissive = standard.emissive ?? [0, 0, 0];
    const status = this.Module._dvz_wasm_api_visual_set_material(
      this.handle,
      model,
      options.opacity ?? 1,
      base[0] ?? 1,
      base[1] ?? 1,
      base[2] ?? 1,
      base[3] ?? 1,
      light[0] ?? -0.45,
      light[1] ?? -0.35,
      light[2] ?? 0.82,
      phong.ambient ?? 0.24,
      phong.diffuse ?? 0.82,
      phong.specular ?? 0.24,
      phong.shininess ?? 26,
      standard.roughness ?? 0.62,
      standard.specular ?? 0.34,
      standard.metallic ?? 0,
      emissive[0] ?? 0,
      emissive[1] ?? 0,
      emissive[2] ?? 0,
      standard.rimStrength ?? 0.1,
    );
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_material failed with ${status}`));
    }
  }

  setSegmentCaps(startCap, endCap = startCap) {
    const status = this.Module._dvz_wasm_api_visual_set_segment_caps(
      this.handle,
      segmentCapCode(startCap),
      segmentCapCode(endCap),
    );
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_segment_caps failed with ${status}`));
    }
  }

  setPathCaps(startCap, endCap = startCap) {
    const status = this.Module._dvz_wasm_api_visual_set_path_caps(
      this.handle,
      segmentCapCode(startCap),
      segmentCapCode(endCap),
    );
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_path_caps failed with ${status}`));
    }
  }

  setPathJoin(join, miterLimit = 4) {
    const status = this.Module._dvz_wasm_api_visual_set_path_join(
      this.handle,
      pathJoinCode(join),
      miterLimit,
    );
    if (status !== 0) {
      throw new Error(this._diagnosticMessage(`dvz_wasm_api_visual_set_path_join failed with ${status}`));
    }
  }
}
