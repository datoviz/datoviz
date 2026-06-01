# WebGPU Subset

Status: experimental v0.4 browser subset.

Datoviz v0.4 includes an experimental browser path:

```text
C/WASM scene state -> scene frame plan -> WGSL DRP2 stream -> browser WebGPU runtime -> canvas
```

This is a portability proof for the v0.4 scene and DRP2 contract. It is not native Vulkan feature
parity and should be treated as an unstable preview.

## Supported Scene Slice

The browser scene demos use the generic `dvz_wasm_api_*` object ABI in `src/wasm/scene_api.c`.

Supported visual and interaction families:

- point visuals;
- primitive triangle-list visuals;
- RGBA8 2D image visuals;
- basic mesh visuals;
- 2D panzoom controller input;
- one basic 3D mesh scene with camera and arcball controller input.

Supported browser pages:

- `examples/webgpu/wasm_scene.html`: point + primitive + image + mesh + panzoom;
- `examples/webgpu/wasm_scene_3d.html`: basic 3D mesh + camera + arcball;
- `examples/webgpu/fixtures.html`: DRP2 fixture dashboard for the pure browser WebGPU runner,
  retained runtime stress checks, and WASM scene smoke rows.

Supported browser canvas formats:

- `rgba8unorm`;
- `bgra8unorm`.

The WASM scene ABI currently emits JSON DRP2 streams. JSON is the debug and fixture transport, not
the intended long-term hot path.

## Supported DRP2 Runtime Slice

The browser WebGPU runner executes the committed fixture subset of:

- WGSL shader modules;
- `rgba8unorm`, `bgra8unorm`, `r32uint`, and `depth32float` texture formats;
- sample counts `1` and `4` where supported by the browser/device;
- vertex, index, uniform, storage, sampled-texture, and sampler bindings;
- render and compute pipelines for the committed positive fixture set;
- render passes with one or more color attachments;
- depth attachment checks used by the fixture set;
- dynamic buffer offsets by materializing equivalent WebGPU bind groups;
- copy commands covered by the fixture set, including row-pitch adaptation when WebGPU requires it;
- object destruction and use-after-destroy validation for the active command subset.

Active command coverage is recorded in `examples/webgpu/COMPAT.md`.

## Unsupported Or Deferred

The browser path intentionally does not support:

- native Vulkan, vklite, canvas, window, stream, video, app, or GUI modules in WASM;
- GLSL/SPIR-V browser shader translation;
- native scene/app parity;
- custom shader APIs;
- text, labels, axes, colorbars, scale bars, picking, readback, volume, sphere, path, segment, and
  advanced technique parity in the WASM scene demos;
- zero-copy or binary command transport;
- stable public JS/TS bindings for the WASM scene ABI.

Deferred DRP2 commands remain outside the active browser subset:

- `CreatePipelineLayout`;
- `DestroyPipelineLayout`;
- `DrawIndirect`;
- `DrawIndexedIndirect`;
- `DispatchWorkgroupsIndirect`.

Unsupported commands, unsupported shader formats, unsupported texture formats, unsupported sample
counts, invalid resource ids, lifecycle errors, and malformed stream state should fail with explicit
diagnostics.

## ABI Lifetime Rules

The experimental WASM scene ABI uses opaque `uint32_t` handles. JavaScript must pass handles back
unchanged and must not inspect pointer values or internal scene state.

Payload and diagnostics lifetime:

- `dvz_wasm_api_payload_ptr()` and `dvz_wasm_api_payload_size()` return a borrowed UTF-8 JSON
  payload;
- the payload is valid only until the next mutating ABI call on the same scene or scene destruction;
- JavaScript must copy or decode the payload before resize, pointer, wheel, data upload, emit,
  canvas-format, capability, or destroy calls;
- `dvz_wasm_api_set_capabilities()` passes normalized browser limits for texture dimension, bind
  groups, vertex buffers, buffer size, texture-copy row alignment, and sample count into scene
  emission;
- `dvz_wasm_api_diagnostic()` returns borrowed strings with the same lifetime as the current
  diagnostic report;
- successful emits should leave diagnostics empty;
- failed emits and rejected operations should return an error status and expose diagnostics when the
  scene or DRP2 layer can explain the failure.

Visual update behavior:

- same-shape visual data updates emit retained-runtime update streams;
- texture size changes emit setup-bearing streams and the browser wrapper reloads the runtime before
  frame replay;
- point item-count growth is rejected by the current per-attribute ABI and reports attr/count
  diagnostics.

## Validation

Use the browserless checks first:

```bash
just webgpu-fixture-preflight
just webgpu-runner-smoke
just wasm-scene-smoke
just webgpu-browser-smoke
```

Then run a local browser server:

```bash
python3 -m http.server 8765
```

Open:

```text
http://localhost:8765/examples/webgpu/fixtures.html
http://localhost:8765/examples/webgpu/wasm_scene.html
http://localhost:8765/examples/webgpu/wasm_scene_3d.html
```

Expected manual results for the current subset:

- fixture dashboard: all committed rows pass, no unsupported rows, no failures;
- fixture dashboard WASM Scene Smoke section: 2D point update and 3D mesh update rows pass;
- 2D WASM page: point, primitive, image, and mesh content render; pan/zoom and resize work;
- 3D WASM page: cube renders; arcball drag, wheel zoom, and resize work.

`just webgpu-browser-smoke` automates the two WASM scene page checks and the dashboard WASM Scene
Smoke rows with headless Chrome when Chrome/Chromium is available locally. It writes transient PNG
evidence under `build/webgpu-browser-smoke/`.
