# DRP2 WebGPU Compatibility

This note records the current browser WebGPU proof-of-concept compatibility surface.

The WebGPU runner is a strict subset check, not a full DRP2 backend. It tracks the active DRP2
command surface and validates the currently portable fixture slice: `37` positive DRP2 fixtures,
`2` WebGPU-only attachment streams, and `82` expected-failure semantic negative fixtures.

As of the capability-preflight slice (`c03e89227`), the pure browser WebGPU runner was considered
closed for the original narrow v0.4 RC experimental subset. The current RC target widens the
scene/WASM example-host layer so most non-desktop scene examples can have live WebGPU website
versions. Runner command expansion should remain evidence-driven and scoped to that target.


## RC Promotion Target

Current proof counts remain the fixture/dashboard truth until new browser evidence is recorded.
The first portable scenario/frame-callback slice is current for `feature_timer_animation`; compute
buffer animation and the promoted query/readback selection routes now have browser-live evidence.
Remaining RC promotions are:

1. broaden the portable scenario host from `feature_timer_animation` to website examples;
2. manifest-backed example classification as `webgpu-live`, `webgpu-planned`,
   `webgpu-deferred`, or `native-only`.

Do not move broader live-example coverage into the supported/current section until
`wasm-scene-smoke`, `webgpu-browser-smoke`, and any relevant native/DRP2 checks have recorded
evidence for the promoted slice.


## Fixture Dashboard

Run from the repository root:

```bash
python3 -m http.server 8765
```

Then open:

```text
http://localhost:8765/examples/webgpu/fixtures.html
```

The committed dashboard manifest currently covers:

- `37` positive DRP2 fixtures under `spec/drp2/fixtures/positive`
- `2` WebGPU-only strict stream checks under `examples/webgpu/streams`
- `82` semantic negative fixtures under `spec/drp2/fixtures/negative`
- `121` total dashboard rows

Current status as of this note:

- positive fixture count: `37`
- WebGPU stream count: `2`
- negative parity fixture count: `82`
- expected browser dashboard result for the committed subset: `121 pass, 0 unsupported, 0 fail`
- recorded browser dashboard result on 2026-05-28 after the repeated-runtime-frame smoke slice
  (`183812f27`): `120 pass, 0 unsupported, 0 fail`
- recorded browser dashboard result on 2026-05-29 after the retained browser-runtime stress slice
  (`292e82899`): fixture compatibility `120 pass, 0 unsupported, 0 fail`; retained runtime stress
  `4 pass, 0 fail`
- recorded browser dashboard result on 2026-05-29 after the demo-runtime reload stress slice
  (`a1c0d7306`): fixture compatibility `120 pass, 0 unsupported, 0 fail`; retained runtime stress
  `7 pass, 0 fail`
- recorded browser dashboard result on 2026-05-29 after the capability-preflight diagnostics slice
  (`c03e89227`): fixture compatibility `120 pass, 0 unsupported, 0 fail`; retained runtime stress
  `7 pass, 0 fail`
- recorded manual browser result on 2026-05-30 after the generic WASM subset documentation,
  diagnostic ABI, and panzoom metadata slices (`9f5e93bbb`): fixture compatibility
  `120 pass, 0 unsupported, 0 fail`; retained runtime stress `7 pass, 0 fail`; 2D WASM
  point/primitive/image/mesh page rendered and pan/zoom worked; 3D WASM cube page rendered and
  arcball interaction worked.
- recorded manual browser result on 2026-05-30 after WASM ABI diagnostic hardening: fixture
  compatibility `120 pass, 0 unsupported, 0 fail`; retained runtime stress `7 pass, 0 fail`; 2D
  WASM point/primitive/image/mesh page rendered and pan/zoom worked; 3D WASM cube page rendered and
  arcball interaction worked.
- recorded browserless WASM robustness proof on 2026-05-31: `just wasm-scene-smoke` validates
  visual-family stream shape for point, primitive, RGBA8 image, basic mesh, 2D update streams, and
  3D mesh/arcball update streams; generated 2D and 3D WASM streams pass WebGPU fixture preflight and
  execute through the JS WebGPU runner's repeated-frame resource-stability smoke.
- recorded automated browser proof on 2026-05-31: `just webgpu-browser-smoke` launches headless
  Chrome with WebGPU enabled, serves the local repo, renders the then-current 2D and 3D WASM scene
  entry points, exercises pointer and wheel interaction, checks the browser status remains
  non-error, and writes nonblank canvas PNG captures under `build/webgpu-browser-smoke/`.
- recorded browserless and automated browser proof on 2026-06-01 after WASM capability handoff and
  retirement of the direct browser-side panzoom uniform path: `just webgpu-fixture-preflight`
  passed `39` strict positive/WebGPU stream rows; `just webgpu-runner-smoke` passed
  `37` positive fixtures, `2` WebGPU streams, and `82` negative parity fixtures;
  `just wasm-scene-smoke` passed the 2D/3D generic ABI streams; `just webgpu-browser-smoke`
  rendered the 2D and 3D WASM pages, exercised pointer and wheel interaction, and wrote nonblank
  PNG captures under `build/webgpu-browser-smoke/`.
- recorded browserless and automated browser proof on 2026-06-01 after retained frame-resource
  tracking and visual update contract hardening: `just webgpu-fixture-preflight` passed `39` strict
  positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive fixtures,
  `2` WebGPU streams, and `82` negative parity fixtures; retained runtime stress now checks stable
  resource counts and zero open, recorded, or retired submitted references; `just wasm-scene-smoke`
  validates same-shape visual updates, rejected point count growth, texture-resize reload streams,
  and the 2D/3D generic ABI streams; `just webgpu-browser-smoke` rendered the 2D and 3D WASM pages,
  exercised pointer and wheel interaction, and wrote nonblank PNG captures under
  `build/webgpu-browser-smoke/`.
- recorded automated browser proof on 2026-06-01 after browser scene lifecycle and dashboard
  hardening: `just webgpu-browser-smoke` rendered the 2D and 3D WASM pages, exercised pointer and
  wheel interaction, checked pagehide scene destruction, ran the fixture dashboard's WASM Scene
  Smoke rows, and reported `2` dashboard WASM scene checks passing.
- recorded automated browser proof on 2026-06-01 after retained setup-update hardening:
  `just webgpu-browser-smoke` now covers the dashboard 2D WASM row with same-shape point updates and
  RGBA8 image texture resize reloads against retained browser runtime state.
- recorded browserless, native, and automated browser proof on 2026-06-01 after direct WASM payload
  transport: `just wasm-scene-smoke` validates payload-ref JSON plus borrowed WASM-memory payload
  spans; `just test drp2` covers the native payload-ref serializer/accessors; and
  `just webgpu-browser-smoke` renders the 2D/3D WASM pages through typed-array payloads.
- recorded browserless and automated browser proof on 2026-06-01 after packet diagnostic hardening:
  `just wasm-scene-smoke` validates failed packet emits set a nonzero packet status with an explicit
  diagnostic, and `just webgpu-browser-smoke` renders the 2D/3D WASM pages, exercises retained
  update/frame packet lifecycles, checks pagehide destruction, and reports `2` dashboard WASM scene
  checks passing.
- recorded automated browser proof on 2026-06-01 after the unified examples host split:
  reusable WASM browser session code lives under `web/wasm/`, the release-visible 2D and 3D demos
  load through `examples/webgpu/examples.html`, and `just webgpu-browser-smoke` renders both demos,
  exercises retained packet lifecycles, checks pagehide destruction, and reports `2` dashboard WASM
  scene checks passing.
- recorded local release proof on 2026-06-04: `just webgpu-fixture-preflight` passed `39` strict
  positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive fixtures, `2`
  WebGPU streams, and `82` negative parity fixtures; `just wasm-scene-smoke` emitted, preflighted,
  and replayed the 2D and 3D WASM scene streams; `just webgpu-browser-smoke` rendered both WASM
  demos, exercised interaction, and reported dashboard WASM scene checks `2 pass, 0 fail`.
- recorded local marker-promotion proof on 2026-06-05: `just webgpu-fixture-preflight` passed `39`
  strict positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive fixtures, `2`
  WebGPU streams, and `82` negative parity fixtures; `just wasm-scene-smoke` emitted, preflighted,
  and replayed the 2D point/pixel/marker/primitive/image/mesh + panzoom stream and the 3D
  mesh/arcball stream; `just webgpu-browser-smoke` rendered both WASM demos, exercised interaction,
  and reported dashboard WASM scene checks `2 pass, 0 fail`.
- recorded local segment-promotion proof on 2026-06-05: `just webgpu-fixture-preflight` passed
  `39` strict positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive
  fixtures, `2` WebGPU streams, and `82` negative parity fixtures; `just wasm-scene-smoke` emitted,
  preflighted, and replayed the 2D point/pixel/marker/segment/primitive/image/mesh + panzoom stream
  and the 3D mesh/arcball stream; `just webgpu-browser-smoke` rendered both WASM demos, exercised
  interaction, and reported dashboard WASM scene checks `2 pass, 0 fail`.
- recorded local textured-mesh promotion proof on 2026-06-05: `just webgpu-fixture-preflight`
  passed `39` strict positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive
  fixtures, `2` WebGPU streams, and `82` negative parity fixtures; `just wasm-scene-smoke` emitted,
  preflighted, and replayed the 2D point/pixel/marker/segment/primitive/image/mesh + panzoom stream
  and the 3D textured mesh/arcball stream; `just webgpu-browser-smoke` rendered both WASM demos,
  exercised interaction, and reported dashboard WASM scene checks `2 pass, 0 fail`.
- recorded local sphere-promotion proof on 2026-06-05: `python3 tools/check_scene_shader_abi.py`
  and `just test test_scene_sphere_mode` passed; `just webgpu-fixture-preflight` passed `39`
  strict positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive fixtures,
  `2` WebGPU streams, and `82` negative parity fixtures; `just wasm-scene-smoke` emitted,
  preflighted, and replayed the 2D point/pixel/marker/segment/primitive/image/mesh + panzoom stream
  and the 3D sphere/textured mesh/arcball stream; `just webgpu-browser-smoke` rendered both WASM
  demos, exercised interaction, and reported dashboard WASM scene checks `2 pass, 0 fail`.
- recorded local path-promotion proof on 2026-06-05: `python3 tools/check_scene_shader_abi.py`
  and `just test test_scene_path_line_width_emit_glsl` passed; `just webgpu-fixture-preflight`
  passed `39` strict positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive
  fixtures, `2` WebGPU streams, and `82` negative parity fixtures; `just wasm-scene-smoke`
  emitted, preflighted, and replayed the 2D point/pixel/marker/segment/path/primitive/image/mesh +
  panzoom stream and the 3D sphere/textured mesh/arcball stream; `just webgpu-browser-smoke`
  rendered both WASM demos, exercised interaction, and reported dashboard WASM scene checks
  `2 pass, 0 fail`.
- recorded local mesh-material promotion proof on 2026-06-05: `just test
  test_scene_visual_material_setter` and `just test test_scene_visual_internal_material_state`
  passed; `just webgpu-fixture-preflight` passed `39` strict positive/WebGPU stream rows; `just
  webgpu-runner-smoke` passed `37` positive fixtures, `2` WebGPU streams, and `82` negative parity
  fixtures; `just wasm-scene-smoke` applied standard material parameters to 2D and 3D mesh visuals,
  rejected unsupported point material updates, and replayed retained material update streams; `just
  webgpu-browser-smoke` rendered both WASM demos, exercised interaction, and reported dashboard
  WASM scene checks `2 pass, 0 fail`.
- recorded local path/stroke cap-join promotion proof on 2026-06-05: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check web/wasm/scene.js`, and `node --check
  examples/webgpu/demos/wasm_2d.js` passed; `just test test_scene_segment_caps` passed; `just
  webgpu-fixture-preflight` passed `39` strict positive/WebGPU stream rows; `just
  webgpu-runner-smoke` passed `37` positive fixtures, `2` WebGPU streams, and `82` negative parity
  fixtures; `just wasm-scene-smoke` applied segment cap, path cap, and path join setters, rejected
  unsupported point style updates, and replayed retained style update streams through the JS WebGPU
  runner smoke; `just webgpu-browser-smoke` rendered both WASM demos, exercised interaction, and
  reported dashboard WASM scene checks `2 pass, 0 fail`.
- recorded local buffer-backed point/pixel promotion proof on 2026-06-05: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check web/wasm/scene.js`, and `node --check
  examples/webgpu/demos/wasm_2d.js` passed; `just test
  test_scene_point_external_position_buffer_emits_no_upload` and `just test
  test_scene_point_storage_position_buffer_emits_usage` passed; `just webgpu-fixture-preflight`
  passed `39` strict positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive
  fixtures, `2` WebGPU streams, and `82` negative parity fixtures; `just wasm-scene-smoke` created
  scene buffers, bound them to point and pixel positions, rejected invalid buffer descriptors and
  unsupported attr-buffer binds, and replayed retained buffer update streams through the JS WebGPU
  runner smoke; `just webgpu-browser-smoke` rendered both WASM demos, exercised interaction, and
  reported dashboard WASM scene checks `2 pass, 0 fail`.
- recorded local glyph-promotion proof on 2026-06-05: `node --check tools/wasm_scene_smoke.mjs`,
  `node --check web/wasm/scene.js`, and `node --check examples/webgpu/demos/wasm_2d.js` passed;
  `just test test_scene_glyph_emit_glsl` passed; `just webgpu-fixture-preflight` passed `39`
  strict positive/WebGPU stream rows; `just webgpu-runner-smoke` passed `37` positive fixtures,
  `2` WebGPU streams, and `82` negative parity fixtures; `just wasm-scene-smoke` emitted,
  preflighted, and replayed the 2D point/pixel/marker/segment/path/primitive/image/glyph/mesh +
  panzoom stream and the 3D sphere/textured mesh/arcball stream; `just webgpu-browser-smoke`
  rendered both WASM demos, exercised interaction, and reported dashboard WASM scene checks
  `2 pass, 0 fail`.
- recorded local semantic-text promotion proof on 2026-06-05: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check web/wasm/scene.js`, `node --check
  examples/webgpu/demos/wasm_2d.js`, `node --check web/drp2/context.js`, and `node --check
  web/drp2/webgpu.js` passed; `just wasm-scene-smoke` emitted, preflighted, packetized, and
  replayed the 2D point/pixel/marker/segment/path/primitive/image/glyph/text/mesh + panzoom stream
  and the 3D sphere/textured mesh/arcball stream through the JS WebGPU runner smoke.
- recorded local labels-promotion proof on 2026-06-05: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check web/wasm/scene.js`, `node --check
  examples/webgpu/demos/wasm_2d.js`, `node --check web/drp2/context.js`, and `node --check
  web/drp2/webgpu.js` passed; `direnv exec . just test
  test_scene_labels_visual_binds_categorical_scale`, `direnv exec . just test
  test_scene_labels_state_setters`, and `direnv exec . just test test_scene_labels_emit_wgsl`
  passed; `just webgpu-fixture-preflight`, `just webgpu-runner-smoke`, `just wasm-scene-smoke`,
  and `just webgpu-browser-smoke` passed with the 2D WASM stream rendering signed categorical
  labels through an `r32sint` texture.
- recorded local axes-promotion proof on 2026-06-05: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check web/wasm/scene.js`, and `node --check
  examples/webgpu/demos/wasm_2d.js` passed; `direnv exec . just test
  test_axis_domain_and_ticks`, `direnv exec . just test test_axis_text_labels`, and `direnv exec
  . just test test_axis_panzoom_visible_domain` passed; `just wasm-scene-smoke` emitted,
  preflighted, and replayed the 2D WASM stream with retained X/Y axes, grid lines, and bitmap
  tick/axis labels.
- recorded local portable-scenario/frame-callback proof on 2026-06-07: `node --check
  web/wasm/scene.js`, `node --check web/wasm/session.js`, `node --check
  tools/wasm_scene_smoke.mjs`, and `node --check tools/webgpu_browser_smoke.mjs` passed; `just
  example-c features/timer_animation` passed and preserved the native scenario runner path; `just
  wasm-scene-smoke` generated and replayed the `feature_timer_animation` C scenario stream plus the
  existing 2D/3D WASM streams, with WebGPU runner smoke reporting `generated_streams=3`; `just
  webgpu-browser-smoke` rendered 2D and 3D WASM pages, advanced browser-driven frames on the
  `wasm-timer-animation` route, and reported dashboard WASM scene checks `2 pass, 0 fail`.
- recorded local point-query browser-live checkpoint on 2026-06-08: `node --check
  web/wasm/scene.js`, `node --check web/wasm/session.js`, and `node --check
  tools/webgpu_browser_smoke.mjs` passed; `just wasm-scene-smoke` generated and replayed the
  existing scenario and WASM scene streams; `node tools/wasm_query_packet_probe.mjs` emitted point
  query setup/update/frame packets with readback metadata; `just webgpu-browser-smoke` rendered
  `wasm-pick-point` and reported `processed=1`, proving point-picking readback and retained
  result-state updates through the artifact-backed lifetime path.
- recorded local marker-query browser-live checkpoint on 2026-06-08: `node --check
  examples/webgpu/examples.js`, `node --check examples/webgpu/demos/wasm_pick_marker.js`, and
  `node --check tools/webgpu_browser_smoke.mjs` passed; `just webgpu-browser-smoke` rendered
  `wasm-pick-marker` and reported `processed=1`, proving marker item readback through the same
  artifact-backed query path.
- recorded local hover/selection browser-live checkpoint on 2026-06-08: `node --check
  examples/webgpu/demos/wasm_pick_hover.js`, `node --check
  examples/webgpu/demos/wasm_selection.js`, and `node --check tools/webgpu_browser_smoke.mjs`
  passed; `just webgpu-browser-smoke` rendered `wasm-pick-hover` and `wasm-selection`, each with
  `processed=1`, proving retained hover and selection state updates through the same query path.
- recorded local image-probe browser-live checkpoint on 2026-06-08: `node --check
  examples/webgpu/demos/wasm_image_probe.js` and `node --check tools/webgpu_browser_smoke.mjs`
  passed; `just webgpu-browser-smoke` rendered `wasm-image-probe` with `processed=1`, proving one
  sampled pixel probe through the browser query/readback path.
- recorded local sphere-selection promotion checkpoint on 2026-06-09: `node --check
  tools/wasm_scene_smoke.mjs` and `node --check examples/webgpu/live_examples.js` passed;
  `just example-c features/selection_sphere` passed; `just wasm-scene-smoke` emitted, packetized,
  and replayed the retained sphere selection scenario plus direct r32uint sphere query packets.
  `just webgpu-browser-smoke` exited successfully in this shell, but both live checks were skipped
  by headless WebGPU instance loss before scene rendering.
- recorded local mesh-instance-selection promotion checkpoint on 2026-06-09: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check examples/webgpu/live_examples.js`, and
  `python3 tools/check_scene_shader_abi.py` passed; `just example-c
  features/selection_mesh_instances` passed; `just wasm-scene-smoke` emitted, packetized, and
  replayed the retained mesh instance selection scenario plus direct r32uint mesh query packets.
- recorded local compute-buffer-animation promotion checkpoint on 2026-06-09: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check examples/webgpu/live_examples.js`, and
  `node --check web/drp2/packet.js` passed; `just example-c features/compute_buffer_animation`
  passed; `just wasm-scene-smoke` emitted, packetized, and replayed the retained compute scenario,
  including compute pipeline creation, workgroup dispatch, and compute-to-vertex resource barriers.
- recorded local colorbar promotion checkpoint on 2026-06-09: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check tools/webgpu_browser_smoke.mjs`, and `node --check
  examples/webgpu/live_examples.js` passed; `just wasm-scene-smoke` emitted, packetized, and
  replayed the retained colorbar scenario, including the scalar image, colorbar ramp primitive,
  tick segments, and glyph labels. `just webgpu-browser-smoke` exited successfully in this shell,
  with the live timer, colorbar, and query checks skipped by known headless WebGPU instance loss
  before scene rendering.
- recorded local scale-bar promotion checkpoint on 2026-06-09: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check tools/webgpu_browser_smoke.mjs`, and `node --check
  examples/webgpu/live_examples.js` passed; `just example-c features/scalebar` passed; `just
  wasm-scene-smoke` emitted, packetized, and replayed the retained scale-bar scenario, including
  the point reference visual, generated scale segment, and glyph label. `just
  webgpu-browser-smoke` exited successfully in this shell, with the live timer, colorbar,
  scale-bar, and query checks skipped by known headless WebGPU instance loss before scene
  rendering.
- recorded local scale-bar-units promotion checkpoint on 2026-06-09: `node --check
  tools/wasm_scene_smoke.mjs`, `node --check tools/webgpu_browser_smoke.mjs`, and `node --check
  examples/webgpu/live_examples.js` passed; `just example-c features/scalebar_units` passed; `just
  wasm-scene-smoke` emitted, packetized, and replayed the retained scale-bar units scenario,
  including the path trace, generated scale segment, and glyph duration label. `just
  webgpu-browser-smoke` exited successfully in this shell, with the live timer, colorbar,
  scale-bar, scale-bar-units, and query checks skipped by known headless WebGPU instance loss
  before scene rendering.
- remaining unsupported entries in the committed subset: none

This subset is intentionally labeled as the "WebGPU fixture subset": passing it means the browser
runner can execute the committed portable fixtures and WebGPU-specific attachment probes, and reject
the semantic negative fixtures with the expected `commandIndex`, `cmd`, and `code`. It does not mean
deferred DRP2 commands or every future schema command are browser-supported.


## Supported Commands

The PoC currently executes these DRP2 commands:

- `HelloRenderer`
- `RendererHelloReply`
- `Error` as a no-op diagnostic marker
- `CreateBuffer`
- `DestroyBuffer`
- `WriteBuffer`
- `CreateTexture`
- `DestroyTexture`
- `CreateTextureView`
- `DestroyTextureView`
- `WriteTexture`
- `CreateSampler`
- `DestroySampler`
- `CreateBindGroupLayout`
- `DestroyBindGroupLayout`
- `CreateBindGroup`
- `DestroyBindGroup`
- `CreateShaderModule`
- `DestroyShaderModule`
- `CreateRenderPipeline`
- `DestroyRenderPipeline`
- `CreateComputePipeline`
- `DestroyComputePipeline`
- `BeginCommandEncoder`
- `FinishCommandEncoder`
- `BeginRenderPass`
- `EndRenderPass`
- `BeginComputePass`
- `EndComputePass`
- `SetPipeline` for render and compute passes
- `SetVertexBuffer`
- `SetIndexBuffer`
- `SetBindGroup`
- `SetViewport`
- `SetScissor`
- `SetBlendConstant`
- `SetStencilReference`
- `Draw`
- `DrawIndexed`
- `DispatchWorkgroups`
- `ResourceBarrier` as a validated ordering marker
- `CopyBufferToBuffer`
- `CopyBufferToTexture`
- `CopyTextureToBuffer`
- `CopyTextureToTexture`
- `QueueSubmit`
- `QueueSubmitReply` as a no-op reply marker


## Unsupported Commands

All active commands listed in `spec/drp2/schema/README.md` currently have a WebGPU runner switch case,
either as executable behavior or as explicit lifecycle/diagnostic handling.

The following schema files are deferred and non-authoritative for the current DRP2 command surface, so
the WebGPU runner rejects them through the unsupported-command path if they appear in an ad hoc stream:

- `CreatePipelineLayout`
- `DestroyPipelineLayout`
- `DrawIndirect`
- `DrawIndexedIndirect`
- `DispatchWorkgroupsIndirect`


## Supported Fields And Narrow Mappings

The PoC supports the fixture subset of:

- WGSL shader modules only
- `rgba8unorm`, `bgra8unorm`, and `depth32float` textures
- `r32uint` integer render targets for picking-style readback fixtures
- one or more color attachments per render pass for the committed fixture subset
- `point-list`, `line-list`, `line-strip`, `triangle-list`, and `triangle-strip` topology
- `uint16` and `uint32` index buffers
- sampled texture bindings
- sampler bindings
- uniform buffer bindings
- storage buffer bindings
- dynamic buffer offsets by materializing adjusted bind groups at `SetBindGroup` time
- compute pipelines, compute passes, and direct workgroup dispatch for the positive fixture subset


## Capability Reporting

`initWebGPU(canvas)` returns a DRP2-shaped capability snapshot alongside the adapter device, canvas
context, and canvas format. The browser runtime uses that snapshot as the default capability set and
lets fixture-level `capabilities` entries narrow it for negative tests. The current snapshot reports:

- `supported_shader_formats`: `wgsl`
- `supported_texture_formats`: the committed WebGPU texture subset plus the preferred canvas format
- `supported_sample_counts`: `1` and `4`
- `supports_color_blending`: `true`
- `max_texture_dimension_2d` when exposed by the WebGPU device limits
- `supports_fp64`: `false`

The fixture dashboard stores this snapshot in the summary tooltip so browser runs can report the
capability context that was used for validation.

The runner preflights stream-level capabilities before command execution. Unsupported commands and
unsupported shader, texture, render-target, or depth/stencil capability choices return DRP2-level
diagnostics before creating browser GPU resources.


## Browser Canvas Target Conventions

The browser runner has an explicit external-target convention for WebGPU presentation streams:

- `texture_id: 0` in a render-pass color attachment means the current browser canvas texture.
- `texture_id: 0` in a render-pass depth/stencil attachment means a transient browser-owned
  `depth32float` attachment matching the pass extent.
- Pipeline color target format `"canvas"` means `navigator.gpu.getPreferredCanvasFormat()`.
- Texture dimensions `"canvas"` for width/height mean the current canvas pixel extent. The runner
  and fixture preflight require both width and height to use the alias together.

## PoC-Local Adaptations

These are compatibility choices in the browser runner for ad hoc demo streams and older command
forms. The fixture dashboard requires explicit bind-group layout and render-pipeline metadata.

- The main demo page can keep persistent WebGPU resources for a loaded stream and replay only the
  frame command slice after the first command encoder. The strict fixture dashboard still executes
  each stream as a one-shot command list.
- Missing `CreateRenderPipeline.color_targets` follows the DRP2 default for ad hoc developer
  streams: the configured canvas format for canvas targets, otherwise `rgba8unorm` when no
  attachment format is available. Committed browser streams now carry explicit color targets.
- Missing `vertex_buffers` means vertex pulling or builtins in DRP2. The historical one-slot
  compatibility fallback for shaders declaring `@location(0)` is now opt-in through the main demo
  session only; normal runner and fixture paths reject that missing metadata.
- Tight `CopyTextureToBuffer.bytes_per_row` values are adapted through an aligned temporary buffer
  because WebGPU requires copy row pitch to be a multiple of 256 bytes.
- Buffer binding offsets must be WebGPU-aligned in the browser runner. DRP2 streams that use
  non-aligned offsets are rejected explicitly instead of silently binding a different range.
- The old live pan/zoom dropdown path has been retired. Release-visible browser interaction now
  goes through the generic WASM scene ABI, which routes DOM input to scene controllers and replays
  scene-emitted DRP2 update streams.
- Bind-group layout `visibility`, storage `access`, render-pipeline `vertex_buffers`, and
  render-pipeline `color_targets` are required by the fixture dashboard and present in committed
  browser streams. Shader input vertex-buffer inference is limited to the explicit
  demo-compatibility option.
- The fixture dashboard retained runtime stress section loads `scene_point_wgsl`,
  `scene_primitive_wgsl`, `texture_sampling_wgsl`, and `attachment_depth_wgsl` once each, renders
  `10` repeated frames through `Drp2WebGpuRuntime`, and checks stable resource counts with no open,
  recorded, or submitted references after every frame. Browser-canvas depth targets are cached as
  runtime-owned frame resources and counted separately from DRP2 texture objects.
- The fixture dashboard demo-path stress rows drive the same `WebGpuDemoSession` used by the main
  demo page through resize-triggered reload and stream reload while checking stable persistent
  resource counts and no open, recorded, or submitted references.
- Destroy commands validate live-object dependencies, use-after-destroy, recorded-work references,
  and submitted-work references. The runner calls native WebGPU `destroy()` only for object types
  that expose it and otherwise tombstones the DRP2 object id.


## DRP2 Contract Gaps Exposed

The first WebGPU pass resolved these DRP2 portability questions in the protocol notes:

- `CreateRenderPipeline.color_targets` remains optional with a backend-selected default.
- `CreateRenderPipeline.vertex_buffers` remains optional, but portable producers should provide it
  when the vertex shader declares user input locations.
- bind-group layout entries may now carry `visibility`.
- storage layout entries may now carry `access` (`read` or `read_write`).
- tight `CopyTextureToBuffer.bytes_per_row` remains valid DRP2, with backend adaptation allowed when
  a backend requires stricter row-pitch alignment.
- dynamic buffer offsets remain DRP2 offsets; the browser runner currently validates WebGPU
  alignment and rejects unsupported offsets rather than silently rebasing them.
- interactive demo uniform targets are now stream metadata, so demo code no longer carries
  scene-emitted buffer ids in the dropdown configuration.

The positive fixture dashboard now runs without bind-group or render-pipeline metadata fallbacks.
The same strict fixture assumptions are checked without a browser by:

```bash
just webgpu-fixture-preflight
```

The broader browserless WebGPU smoke path also executes the `37 + 2` subset with a fake WebGPU device
and checks the manifest's `81` DRP2 semantic negative fixtures for parity of `commandIndex`, `cmd`,
and `code`:

```bash
just webgpu-runner-smoke
```

Both checks are part of `just spec-check`.
