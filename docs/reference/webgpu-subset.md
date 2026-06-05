# WebGPU Subset

Status: experimental v0.4 browser subset.

Datoviz v0.4 includes an experimental browser path:

```text
C/WASM scene state -> scene frame plan -> WGSL DRP2 packets -> browser WebGPU runtime -> canvas
```

This is a portability proof for the v0.4 scene and DRP2 contract. It is not native Vulkan feature
parity and should be treated as an unstable preview.

## Supported Scene Slice

The browser scene demos use the generic `dvz_wasm_api_*` object ABI in `src/wasm/scene_api.c`.
The reusable browser wrapper and session code lives under `web/wasm/`; files under
`examples/webgpu/demos/` are demo content only.

Supported visual and interaction families:

- point visuals with dense and buffer-backed positions;
- pixel visuals with dense and buffer-backed positions;
- basic built-in marker visuals;
- segment visuals with visual-wide cap controls;
- path visuals with visual-wide cap and join controls;
- primitive triangle-list visuals;
- RGBA8 2D image visuals;
- low-level atlas-backed glyph visuals;
- basic, textured, and material-controlled mesh visuals;
- basic sphere visuals;
- 2D panzoom controller input;
- one basic 3D sphere + textured mesh scene with camera and arcball controller input.

Supported browser pages:

- `examples/webgpu/examples.html?demo=wasm-2d`: point + pixel + marker + styled segment/path +
  primitive + image + glyph + mesh + panzoom;
- `examples/webgpu/examples.html?demo=wasm-3d`: basic sphere + textured 3D mesh + camera +
  arcball;
- `examples/webgpu/fixtures.html`: DRP2 fixture dashboard for the pure browser WebGPU runner,
  retained runtime stress checks, and WASM scene smoke rows.

Supported browser canvas formats:

- `rgba8unorm`;
- `bgra8unorm`.

The browser runtime path uses split binary DRP2 setup, update, and frame packets with payload
arenas. JSON emission remains available for debug and fixture export, but it is not used as the
browser render path.

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

## Parity Contract

The v0.4 browser path has parity with Vulkan only at the shared contract boundary:

| Surface | Browser status |
| --- | --- |
| DRP2 validation and lifecycle semantics | committed positive fixture subset and semantic negatives are checked by the WebGPU runner |
| Setup/update/frame resource model | active through split binary packets and retained browser runtime state |
| WGSL shader modules | supported and required for portable browser execution |
| Vulkan-specific modules and presentation | unsupported in WASM; native-only |
| Scene visual parity | limited to point/pixel dense and buffer-backed positions, basic marker, segment/path cap-join controls, primitive, RGBA8 image, low-level atlas glyph, basic/textured/material mesh, and basic sphere demos |
| Controller parity | limited to panzoom and one 3D arcball proof |
| Compute-to-render parity | experimental; fixture coverage exists, gallery-level behavior remains a separate release lane |
| Query/picking/readback scene parity | deferred for live WASM scenes |

Any future feature promoted into the browser subset must update the scene emitter, DRP2 schema or
fixture coverage when needed, WebGPU execution, capability reporting, diagnostics, and this page in
the same change set.

## Unsupported Or Deferred

The browser path intentionally does not support:

- native Vulkan, vklite, canvas, window, stream, video, app, or GUI modules in WASM;
- GLSL/SPIR-V browser shader translation;
- native scene/app parity;
- custom shader APIs;
- semantic text, labels, axes, colorbars, scale bars, picking, readback, volume, path subpath controls,
  broad stroke/vector parity, sphere raycast/depth/material parity, and advanced technique parity
  in the WASM scene demos;
- zero-copy payload transport;
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

- `dvz_wasm_api_emit_packets()` is the browser runtime path and returns setup, update, and frame
  binary DRP2 packets plus payload arenas through packet accessors;
- packet and arena spans are valid only until the next mutating ABI call on the same scene or scene
  destruction;
- JavaScript must decode and execute packet spans immediately and must not retain packet or arena
  views across resize, pointer, wheel, data upload, emit, canvas-format, capability, or destroy
  calls;
- `dvz_wasm_api_emit()` plus `dvz_wasm_api_payload_ptr()` and `dvz_wasm_api_payload_size()` expose
  borrowed UTF-8 JSON for debug and fixture export only;
- JSON is not used as the browser render path;
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
- texture size changes emit setup-bearing streams and the browser wrapper updates retained runtime
  state before frame replay;
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
http://localhost:8765/examples/webgpu/examples.html?demo=wasm-2d
http://localhost:8765/examples/webgpu/examples.html?demo=wasm-3d
```

Expected manual results for the current subset:

- fixture dashboard: all committed rows pass, no unsupported rows, no failures;
- fixture dashboard WASM Scene Smoke section: 2D point/pixel/marker/segment update, 2D image texture
  resize reload, and 3D sphere/textured mesh update rows pass;
- 2D WASM example: point, pixel, marker, segment, path, primitive, image, glyph, and mesh content render;
  pan/zoom and resize work;
- 3D WASM example: sphere impostors and a textured cube render; arcball drag, wheel zoom, and
  resize work.

`just webgpu-browser-smoke` automates the two WASM example checks through the split-packet
runtime path and the dashboard WASM Scene Smoke rows with headless Chrome when Chrome/Chromium is
available locally. It writes transient PNG evidence under `build/webgpu-browser-smoke/`.

Last local release proof recorded on 2026-06-04:

- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: 2D and 3D WASM scene streams emitted, preflighted, and replayed by the
  JS runner smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Mesh material promotion proof recorded on 2026-06-05:

- `just test test_scene_visual_material_setter`: passed;
- `just test test_scene_visual_internal_material_state`: passed;
- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: WASM material setters applied standard material parameters to 2D and 3D
  mesh visuals, rejected unsupported point material updates, and emitted retained material update
  streams;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Path/stroke cap-join promotion proof recorded on 2026-06-05:

- `node --check tools/wasm_scene_smoke.mjs`: passed;
- `node --check web/wasm/scene.js`: passed;
- `node --check examples/webgpu/demos/wasm_2d.js`: passed;
- `just test test_scene_segment_caps`: passed;
- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: WASM segment cap, path cap, and path join setters emitted initial and
  retained update streams, rejected unsupported point style updates, and replayed the generated
  2D/3D streams through the JS WebGPU runner smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Buffer-backed point/pixel promotion proof recorded on 2026-06-05:

- `node --check tools/wasm_scene_smoke.mjs`: passed;
- `node --check web/wasm/scene.js`: passed;
- `node --check examples/webgpu/demos/wasm_2d.js`: passed;
- `just test test_scene_point_external_position_buffer_emits_no_upload`: passed;
- `just test test_scene_point_storage_position_buffer_emits_usage`: passed;
- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: WASM scene buffers backed point and pixel positions, rejected invalid
  buffer descriptors and unsupported attr-buffer binds, emitted retained buffer update streams, and
  replayed the generated 2D/3D streams through the JS WebGPU runner smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Glyph promotion proof recorded on 2026-06-05:

- `node --check tools/wasm_scene_smoke.mjs`: passed;
- `node --check web/wasm/scene.js`: passed;
- `node --check examples/webgpu/demos/wasm_2d.js`: passed;
- `just test test_scene_glyph_emit_glsl`: passed;
- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: WASM glyph visuals emitted an atlas-backed `scene.glyph` pipeline,
  texture upload, and retained 2D frame draws through the JS WebGPU runner smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Path promotion proof recorded on 2026-06-05:

- `python3 tools/check_scene_shader_abi.py`: passed;
- `just test test_scene_path_line_width_emit_glsl`: passed;
- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: 2D point/pixel/marker/segment/path/primitive/image/mesh + panzoom and
  3D sphere/textured mesh/arcball WASM scene streams emitted, preflighted, and replayed by the JS
  runner smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Marker promotion proof recorded on 2026-06-05:

- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: 2D point/pixel/marker/primitive/image/mesh + panzoom and 3D
  mesh/arcball WASM scene streams emitted, preflighted, and replayed by the JS runner smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Segment promotion proof recorded on 2026-06-05:

- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: 2D point/pixel/marker/segment/primitive/image/mesh + panzoom and 3D
  mesh/arcball WASM scene streams emitted, preflighted, and replayed by the JS runner smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Textured mesh promotion proof recorded on 2026-06-05:

- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: 2D point/pixel/marker/segment/primitive/image/mesh + panzoom and 3D
  textured mesh/arcball WASM scene streams emitted, preflighted, and replayed by the JS runner
  smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Sphere promotion proof recorded on 2026-06-05:

- `python3 tools/check_scene_shader_abi.py`: passed;
- `just test test_scene_sphere_mode`: passed;
- `just webgpu-fixture-preflight`: `39` passed, `0` failed;
- `just webgpu-runner-smoke`: `37` positive fixtures, `2` WebGPU streams, and `82` semantic
  negative fixtures passed;
- `just wasm-scene-smoke`: 2D point/pixel/marker/segment/primitive/image/mesh + panzoom and 3D
  sphere/textured mesh/arcball WASM scene streams emitted, preflighted, and replayed by the JS
  runner smoke;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Headless Chrome/Dawn can skip WebGPU render checks before the scene contract is exercised because
of external instance loss, including `A valid external Instance reference no longer exists` or
`Instance dropped in popErrorScope`. Visible browser runs of the WASM examples host and fixture
dashboard remain the render validation path for the current experimental subset.
