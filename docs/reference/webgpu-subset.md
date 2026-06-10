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
- basic retained 2D axes with ticks, grid lines, and bitmap text labels;
- scalar colorbar, scale bar, categorical legend, and anchored readout examples lowered through
  primitive, segment, marker, and glyph/text draws;
- basic signed 2D categorical labels visuals;
- low-level atlas-backed glyph visuals;
- semantic bitmap text visuals lowered through glyph atlas draws;
- basic, textured, and material-controlled mesh visuals;
- basic sphere visuals;
- vector visuals lowered through segment/path draws;
- 2D panzoom controller input;
- 3D arcball, fly, turntable, and orbit-camera controller examples lowered through the shared
  scene/controller path.

Supported browser pages:

- `examples/webgpu/examples.html?demo=wasm-2d`: basic axes + point + pixel + marker + styled
  segment/path + primitive + image + labels + glyph + text + mesh + panzoom;
- `examples/webgpu/examples.html?demo=wasm-3d`: basic sphere + textured 3D mesh + camera +
  arcball;
- `examples/webgpu/examples.html?demo=wasm-timer-animation`: first portable C scenario host proof,
  running `feature_timer_animation` through WASM frame callbacks and retained point updates;
- `examples/webgpu/examples.html?demo=wasm-picking`: combined portable query/readback sampler for
  point picking, marker picking, hover, and selection through the artifact-backed result-state path;
- `examples/webgpu/examples.html?demo=wasm-image-probe`: sampled image pixel probe through the
  query/readback path;
- `examples/webgpu/live.html?id=...`: public live gallery routes generated from canonical C
  examples and portable C scenarios, currently covering 68 promoted examples including standalone
  point, pixel, marker, primitive, segment, path, image, mesh, sphere, text, glyph, and labels
  visuals, panel single/grid/multi/linked basics, panzoom, path axes, axis labels, text block,
  overlay card, guide lines/spans, bars/bands, sampled-field/image color-scale examples, panel
  background, colorbar, scale bars, categorical legend, annotation readout, fly/turntable/orbit
  controller examples, linked probe/colorbar, scientific plotting, vector, wind field, isolines,
  selection routes, compute buffer animation, GPU particle smoke, polygon composite, linked panels
  with axes, scale-bar measurement workflow, surface-grid, U.S. state choropleth, textured
  planets, and protein showcases, and retained data update/visibility, depth-test, alpha blending,
  material mesh, and lighting routes;
- `examples/webgpu/fixtures.html`: DRP2 fixture dashboard for the pure browser WebGPU runner,
  retained runtime stress checks, and WASM scene smoke rows.

Supported browser canvas formats:

- `rgba8unorm`;
- `bgra8unorm`.

The browser runtime path uses split binary DRP2 setup, update, and frame packets with payload
arenas. JSON emission remains available for debug and fixture export, but it is not used as the
browser render path. Both packets and JSON should be projections of the current scene frame
artifact.

## Supported DRP2 Runtime Slice

The browser WebGPU runner executes the committed fixture subset of:

- WGSL shader modules;
- `rgba8unorm`, `bgra8unorm`, `r32uint`, `r32sint`, and `depth32float` texture formats;
- sample counts `1` and `4` where supported by the browser/device;
- vertex, index, uniform, storage, sampled-texture, and sampler bindings;
- render and compute pipelines for the committed positive fixture set;
- render passes with one or more color attachments;
- depth attachment checks used by the fixture set;
- dynamic buffer offsets by materializing equivalent WebGPU bind groups;
- copy commands covered by the fixture set, including row-pitch adaptation when WebGPU requires it
  and packet-level buffer/texture readback plumbing;
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
| Scene visual parity | limited to point/pixel dense and buffer-backed positions, basic marker, segment/path cap-join controls, primitive, RGBA8/scalar image routes, basic retained 2D axes/ticks/grid labels, basic signed 2D labels, low-level atlas glyph, semantic bitmap text, basic/textured/material mesh, basic sphere, vector-lowered routes, and composed layout/annotation examples built from those families |
| Controller parity | panzoom, arcball, fly, turntable, and orbit-camera examples are live; full native controller parity remains out of scope |
| Portable scenario host parity | broad live-example coverage is current for the promoted RC subset; native-only host and app semantics remain outside WASM |
| Compute-to-render parity | DRP2 fixture, native scene proof, `feature_compute_buffer_animation`, and `showcase_gpu_particle_smoke` browser-live routes are current |
| Query/picking/readback scene parity | point and marker item picking, point hover, point selection, sphere selection, mesh instance selection, and one sampled image probe are browser-live |

Any future feature promoted into the browser subset must update the scene emitter, DRP2 schema or
fixture coverage when needed, WebGPU execution, capability reporting, diagnostics, and this page in
the same change set.

Gallery pages are the preferred user-visible validation surface for the browser subset. A
`webgpu-live` gallery route must use the same canonical C example or portable C scenario as native
validation. Browser JavaScript may host the WASM module, load assets, normalize browser events,
configure WebGPU, execute DRP2 packets, surface diagnostics, and provide page UI, but it must not
reimplement the example scene construction, visual state, animation, picking, selection,
query/probe, or data semantics.

Browser smoke routes are deliberately smaller than the gallery. The required RC smoke set is one
basic runtime route plus one query/readback route; broader WASM sampler pages may remain for
development but are not the definition of public WebGPU support.

## Unsupported Or Deferred

The browser path intentionally does not support:

- native Vulkan, vklite, canvas, window, stream, video, app, or GUI modules in WASM;
- GLSL/SPIR-V browser shader translation;
- native scene/app parity;
- custom shader APIs;
- browser-live scene query/readback beyond the promoted point/marker/hover/pixel-selection/
  sphere-selection/mesh-instance-selection/image-probe slice;
- volume, unsigned/rich labels, path subpath controls, broad stroke/vector styling parity, sphere
  raycast/depth/material parity, full query parity, and advanced technique parity in the WASM scene
  demos;
- zero-copy payload transport;
- stable public JS/TS bindings for the WASM scene ABI.

## WASM Promotion Queue

Promote browser scene features in this order unless a release blocker changes the priority:

1. promote composed showcase routes after their component families have focused live proof;
2. add richer labels, including unsigned label variants and palette/category updates;
3. broaden vector/path parity, including path subpaths and stroke edge cases;
4. add reduced volume rendering only after texture-3D memory and diagnostics are explicit.

Each promoted item must reuse the retained native scene path, add the narrow WASM ABI surface it
needs, extend `tools/wasm_scene_smoke.mjs`, add or verify the public live route or development
sampler it needs, and record proof in this document plus `examples/webgpu/COMPAT.md`. Planned RC
targets must not be described as current support until browser smoke evidence lands.

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
- packet and arena spans are borrowed from the current frame artifact and valid only until
  explicit packet release, the next emit call on the same scene, or scene destruction;
- JavaScript must decode or copy packet spans immediately and must not retain packet or arena WASM
  views after artifact release;
- `dvz_wasm_api_emit()` plus `dvz_wasm_api_payload_ptr()` and `dvz_wasm_api_payload_size()` expose
  borrowed UTF-8 JSON serialized from the current frame artifact stream snapshot for debug and
  fixture export only;
- JSON is not used as the browser render path;
- `dvz_wasm_api_set_capabilities()` passes normalized browser limits for texture dimension, bind
  groups, vertex buffers, buffer size, texture-copy row alignment, sample count, and color-blending
  support into scene emission;
- `dvz_wasm_api_diagnostic()` returns borrowed strings with the same lifetime as the current
  diagnostic report or frame artifact;
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
http://localhost:8765/examples/webgpu/live.html?id=feature_timer_animation
http://localhost:8765/examples/webgpu/live.html?id=feature_picking
http://localhost:8765/examples/webgpu/live.html?id=image_probe
```

Expected manual results for the current subset:

- fixture dashboard: all committed rows pass, no unsupported rows, no failures;
- fixture dashboard WASM Scene Smoke section: 2D point/pixel/marker/segment update, 2D image texture
  resize reload, and 3D sphere/textured mesh update rows pass;
- live timer animation: the retained point animation advances through browser-driven frames;
- live picking: pointer delivery, async readback, and retained hover/selection state update;
- live image probe: sampled image query/readback renders and reports probe state.

`just webgpu-browser-smoke` automates representative live basic, query/readback, compute, and
targeted promoted-route checks with headless Chrome when Chrome/Chromium is available locally. It
writes transient PNG evidence under `build/webgpu-browser-smoke/`.

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

Semantic text promotion proof recorded on 2026-06-05:

- `node --check tools/wasm_scene_smoke.mjs`: passed;
- `node --check web/wasm/scene.js`: passed;
- `node --check examples/webgpu/demos/wasm_2d.js`: passed;
- `node --check web/drp2/context.js`: passed;
- `node --check web/drp2/webgpu.js`: passed;
- `just wasm-scene-smoke`: WASM semantic text strings emitted a generated glyph visual, a bitmap
  atlas texture upload, source-over transparent render passes, split packets, and browser-runner
  replay for the retained 2D scene.

Labels promotion proof recorded on 2026-06-05:

- `node --check tools/wasm_scene_smoke.mjs`: passed;
- `node --check web/wasm/scene.js`: passed;
- `node --check examples/webgpu/demos/wasm_2d.js`: passed;
- `node --check web/drp2/context.js`: passed;
- `node --check web/drp2/webgpu.js`: passed;
- `direnv exec . just test test_scene_labels_visual_binds_categorical_scale`: passed;
- `direnv exec . just test test_scene_labels_state_setters`: passed;
- `direnv exec . just test test_scene_labels_emit_wgsl`: passed;
- `just wasm-scene-smoke`: WASM signed 2D labels emitted an `r32sint` texture, categorical
  lookup uniform, split packets, and browser-runner replay for the retained 2D scene;
- `just webgpu-browser-smoke`: 2D and 3D WASM pages rendered, browser interaction was exercised,
  and dashboard WASM scene checks reported `2 pass, 0 fail`.

Axes promotion proof recorded on 2026-06-05:

- `node --check tools/wasm_scene_smoke.mjs`: passed;
- `node --check web/wasm/scene.js`: passed;
- `node --check examples/webgpu/demos/wasm_2d.js`: passed;
- `direnv exec . just test test_axis_domain_and_ticks`: passed;
- `direnv exec . just test test_axis_text_labels`: passed;
- `direnv exec . just test test_axis_panzoom_visible_domain`: passed;
- `just wasm-scene-smoke`: WASM panel domains and retained X/Y axes emitted primitive tick/grid
  visuals plus bitmap tick/axis labels through split packets and browser-runner replay.

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

Portable scenario/frame-callback proof recorded on 2026-06-07:

- `node --check web/wasm/scene.js`, `node --check web/wasm/session.js`,
  `node --check tools/wasm_scene_smoke.mjs`, and `node --check tools/webgpu_browser_smoke.mjs`
  passed;
- `just example-c features/timer_animation`: passed;
- `just wasm-scene-smoke`: `feature_timer_animation` C scenario stream plus the existing 2D/3D
  WASM streams emitted, preflighted, and replayed by the JS WebGPU runner smoke as
  `generated_streams=3`;
- `just webgpu-browser-smoke`: the `wasm-timer-animation` browser route rendered and advanced
  scenario frames; dashboard WASM scene checks reported `2 pass, 0 fail`.

Headless Chrome/Dawn can skip WebGPU render checks before the scene contract is exercised because
of external instance loss, including `A valid external Instance reference no longer exists` or
`Instance dropped in popErrorScope`. Visible browser runs of the WASM examples host and fixture
dashboard remain the render validation path for the current experimental subset.
