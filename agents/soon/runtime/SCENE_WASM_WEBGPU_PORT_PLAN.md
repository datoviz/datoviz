# Scene/WASM/WebGPU Port Plan

> **Execution Status**
> - **Status:** `PICKUP PLAN`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define the staged route for running the active Datoviz scene layer in the
>   browser by compiling scene-side C code to WASM and executing emitted DRP2 streams through a
>   browser WebGPU runtime.


## Goal

Run the active retained scene path in the browser without porting the native Vulkan presentation
stack.

This lane is part of the v0.4 scope as an experimental portability path. Its v0.4 success criterion
is a documented, narrow scene/DRP2/WGSL/WebGPU subset that can run in the browser, not complete
feature parity with the native Vulkan runtime.

The intended architecture is:

```text
C/WASM scene state
  -> scene FramePlan
  -> WGSL DRP2 command stream
  -> browser WebGPU DRP2 runtime
  -> HTML canvas
```

The scene layer should remain the owner of scene semantics, visual lowering, controller state,
shader selection, resource ids, and request bookkeeping. The browser runtime should execute the
portable DRP2 stream it receives; it should not know about Datoviz visual families except through
the DRP2 contract.


## Non-Goals

1. Do not port `vk`, `vklite`, `canvas`, `window`, `stream`, `video`, or native `app` directly to
   WASM.
2. Do not make the browser runtime translate GLSL, substitute built-in shaders by hash, or infer
   scene semantics.
3. Do not require DVZR recording/replay as the first browser transport.
4. Do not broaden scene API surface just to make the first browser slice work.
5. Do not block the first slice on every scene technique, transparency path, or request/readback
   feature.


## Current Baseline

The branch already has the pieces needed for a feasible first slice:

1. `scene` is an active default-build module and can emit DRP2 streams from retained point,
   primitive, mesh, path, image, and early technique paths.
2. `DvzFramePlanEmitConfig.shader_format` can request `DVZ_SCENE_SHADER_FORMAT_WGSL`.
3. Scene-owned WGSL shaders live under `src/scene/wgsl/`.
4. `examples/c/export_scene_*_wgsl.c` can export scene-generated WGSL DRP2 JSON streams.
5. `examples/webgpu/drp2_webgpu.js` is an experimental browser WebGPU DRP2 runner.
6. `examples/webgpu/fixtures.html` runs the committed positive DRP2 fixture manifest.
7. `examples/webgpu/COMPAT.md` records the current WebGPU proof-of-concept compatibility surface.
8. `src/wasm/` exists only as light scaffolding; it is not yet a scene/WASM build target.

The main architectural gap is not scene logic. It is the boundary cleanup needed to build and
expose only portable scene + DRP2 pieces without native Vulkan/runtime headers leaking into the
browser deliverable.


## Stage 1: Portable Contract Cleanup

### Objective

Make DRP2 stream emission and scene planning genuinely portable, independently buildable, and
clearly separated from vklite execution.

### Work

1. Split DRP2 public headers into portable and native-runtime surfaces.
   - Keep `DvzDrp2CommandStream`, stream construction, JSON/debug serialization, recording helpers
     where practical, and semantic validation in the portable surface.
   - Move vklite-specific runtime entry points and borrowed `DvzStreamFrame` helpers behind a native
     runtime header.
   - Avoid including `datoviz/stream/frame_stream.h` from portable DRP2 headers.

2. Remove public native runtime types from portable structs.
   - Keep `DvzDrp2Runtime` opaque where validation/runtime handles are needed.
   - Move `DvzDevice`, `DvzVma`, `DvzBuffer`, and external native buffer descriptors to native-only
     APIs.

3. Replace Vulkan-shaped public DRP2 values with DRP2-owned enums.
   - Formats.
   - Primitive topology.
   - Shader stages and visibility.
   - Compare operations.
   - Cull mode and front face.
   - Blend factors, blend operations, and color write masks.

4. Keep backend mapping local.
   - vklite maps DRP2 enums to Vulkan values.
   - WebGPU maps DRP2 enums to browser WebGPU strings and flags.
   - Scene emits DRP2 enums, not Vulkan values.

5. Remove scene public header dependence on Vulkan headers.
   - `include/datoviz/scene/enums.h` should not include `datoviz/vk/enums.h`.
   - Scene visual APIs that currently reuse Vulkan-style topology should move to scene/DRP2-owned
     topology names or a small compatibility mapping.

6. Keep semantic validation backend-agnostic.
   - Validation should remain usable from native tests, wasm tests, and browser preflight tooling.
   - Backend-only validation should stay in backend runtime code.

### Exit Criteria

1. A native build can compile `scene + drp2 stream/semantic` with `DVZ_BUILD_VK=OFF`,
   `DVZ_BUILD_CANVAS=OFF`, and no vklite runtime dependency.
2. Public portable DRP2 and scene headers do not include Vulkan, canvas, stream-frame, or vklite
   headers.
3. Existing native vklite DRP2 tests continue to pass through explicit backend mapping.
4. The existing WebGPU fixture preflight still sees the same portable command semantics.


## Stage 2: WASM Scene Build

### Objective

Produce a minimal WASM module that owns scene state and emits WGSL DRP2 streams.

### Work

1. Add a dedicated build profile.
   - A CMake preset, toolchain entry, or `just wasm-scene` recipe is acceptable.
   - Use Emscripten as the first target.
   - Keep the profile narrow and explicit rather than trying to make the whole default library
     browser-compatible.

2. Build only portable modules.
   - Required: `common`, selected `ds`, `math`, `drp2` stream/semantic pieces, and `scene`.
   - Likely excluded initially: `fileio`, `thread`, `vk`, `vklite`, `canvas`, `window`, `stream`,
     `video`, `app`, GUI, CUDA, native DVZR player tools, and shaderc/dlopen paths.
   - Re-enable `fileio` only if it is needed for local browser examples or fixture loading.

3. Add a small exported C/WASM API.
   - Create and destroy a scene.
   - Create a figure and panels.
   - Create a small set of visuals.
   - Upload or update visual data.
   - Update figure size.
   - Update controller state.
   - Emit one WGSL DRP2 frame stream.
   - Return stream JSON or a pointer/length to encoded commands.
   - Destroy emitted streams and any owned payload buffers.

4. Keep memory ownership explicit.
   - JavaScript should know whether a returned pointer is borrowed until the next call or owned
     until an explicit destroy call.
   - Large visual data uploads should use typed-array views into WASM memory where possible.
   - The initial JSON path may copy; later stages should remove that copy for hot paths.

5. Compile embedded WGSL shader resources into the WASM module.
   - The scene shader registry should select WGSL from the existing registry boundary.
   - Missing WGSL variants should become diagnostics during scene emission.

### Exit Criteria

1. A browser page can instantiate the WASM module.
2. JavaScript can create a simple scene in WASM.
3. WASM can emit a WGSL DRP2 stream for at least one point, primitive, and image scene.
4. The stream can be passed to the existing WebGPU runner and rendered on a canvas.


## Stage 3: Browser DRP2 Runtime Hardening

### Objective

Turn the current WebGPU proof-of-concept into a runtime boundary that can consume scene-emitted
streams without demo-specific patches.

### Work

1. Keep WebGPU execution in JavaScript or TypeScript.
   - The browser WebGPU API is object- and promise-oriented.
   - C/WASM should not try to own browser GPU objects directly.

2. Define a browser runtime object model.
   - Buffers.
   - Textures and texture views.
   - Samplers.
   - Bind-group layouts and bind groups.
   - Shader modules.
   - Render and compute pipelines.
   - Command encoders and passes.

3. Align runtime errors with DRP2 semantic validation.
   - Report unsupported command.
   - Report invalid resource id.
   - Report backend capability failure.
   - Report shader/pipeline creation failure.
   - Keep enough command index context to debug scene emissions.

4. Remove PoC-only stream adaptations.
   - No hard-coded scene buffer ids for pan/zoom.
   - No implicit shader substitution.
   - No hidden vertex-layout or bind-layout inference in strict paths.
   - Keep compatibility fallbacks only for legacy standalone demo streams if useful.

5. Preserve persistent GPU resources across frame streams.
   - Resource creation streams should run once or only when resource ids change.
   - Repeated frame streams should be able to update buffers/textures and draw with existing GPU
     resources.

6. Keep fixture coverage as a conformance lane.
   - `examples/webgpu/fixtures.html` should remain the visual browser smoke.
   - `just webgpu-fixture-preflight` should remain the non-browser fixture sanity check.
   - Add scene-emitted WGSL streams to the fixture set once the contract is strict enough.

### Exit Criteria

1. Scene-emitted WGSL streams for point, primitive, image, and a basic mesh render through WebGPU
   without custom stream rewriting.
2. Strict fixture mode does not rely on demo fallbacks.
3. WebGPU runtime resources stay alive across repeated frame submissions.
4. Runtime errors identify the failing DRP2 command and backend reason.


## Stage 4: Efficient Transport And Incremental Updates

### Objective

Replace full-frame JSON/base64 transport with a path suitable for interactive scenes and large
datasets.

### Work

1. Define transport levels.
   - Level 0: JSON stream returned by WASM; useful for inspection and first demos.
   - Level 1: JSON commands with binary payloads passed as WASM memory views.
   - Level 2: compact binary command stream plus direct payload spans.
   - Level 3: split setup/update/draw streams with stable resource ids and minimal per-frame
     payload.

2. Keep JSON as a debugging format.
   - Do not remove `dvz_drp2_stream_json()`.
   - Use JSON in fixtures, trace output, regression artifacts, and diagnostics.
   - Do not use JSON as the hot-path browser transport after this stage.

3. Add resource update discipline.
   - Stable scene resources keep stable DRP2 ids.
   - Scene emits `Create*` only when a resource is new or recreated.
   - Scene emits `WriteBuffer`, `WriteTexture`, or small uniform updates for repeated frames.
   - Browser runtime validates use-after-destroy and stale references at least in debug mode.

4. Add direct payload handling.
   - Large vertex/index/texture payloads should be transferred as typed-array spans.
   - The browser runtime should upload from those spans without base64 decode.
   - Decide when payload spans are copied versus borrowed from WASM memory.

5. Handle resize without full rebuild.
   - Figure size and DPR changes should update target extents and any extent-dependent resources.
   - WebGPU resource recreation should refresh dependent bind groups or invalidate them explicitly.

6. Define stream lifetime.
   - Scene must know when an emitted stream can be destroyed.
   - Browser runtime must not retain borrowed WASM memory after the caller frees or overwrites it.
   - Long-lived GPU resources should be owned by the browser runtime, keyed by DRP2 ids.

### Exit Criteria

1. Pan/zoom and arcball interactions update small uniform buffers rather than resending all visual
   data.
2. Repeated frame rendering does not recreate unchanged buffers, textures, pipelines, or bind groups.
3. Large payloads avoid base64 in the hot path.
4. Resize works without stale bind groups or destroyed texture references.


## Stage 5: Browser App Layer

### Objective

Provide a browser-native presentation layer equivalent in role to native `app`, without porting
native window/canvas internals.

### Work

1. Own browser presentation.
   - HTML canvas creation or attachment.
   - WebGPU adapter/device/context acquisition.
   - Canvas format and target texture management.
   - DPR-aware sizing.
   - Animation frame scheduling.

2. Route browser input into scene controllers.
   - Pointer drag.
   - Wheel zoom.
   - Double-click reset.
   - Keyboard modifiers.
   - Touch gestures later if needed.

3. Keep scene interaction state in WASM.
   - Panzoom and arcball state should live in scene/controller objects.
   - JavaScript should translate browser events into calls on those objects.
   - JavaScript should not duplicate controller math unless it is explicitly a browser-only helper.

4. Add browser status and diagnostics.
   - Frame timing.
   - WebGPU capability snapshot.
   - Last DRP2 validation/runtime error.
   - Optional stream trace.

5. Add examples that mirror native examples.
   - Static point scene.
   - Interactive panzoom points.
   - Image with texture update.
   - Basic mesh with arcball.
   - Multi-panel figure.

### Exit Criteria

1. A browser example can create a scene, interact with it, resize it, and render continuously.
2. The event loop uses retained scene state and incremental DRP2 updates.
3. The browser app layer is clearly separate from native `app`.
4. The example can be run from a simple local static server.


## Stage 6: Parity Expansion

### Objective

Broaden browser support after the core scene/WASM/WebGPU path is usable and efficient.

### Work

1. Add request/readback support.
   - Point picking.
   - Image probing.
   - Segment-label probing.
   - Wider picking payloads once the scene request model is ready.

2. Add multi-panel and linked-controller parity.
   - Viewport/scissor behavior.
   - Linked panzoom propagation.
   - Independent panel controllers.
   - Correct logical-to-pixel coordinate mapping.

3. Broaden visual families and shaders.
   - Mesh and path parity.
   - Sphere impostor after the native sphere visual is stable.
   - Text/annotation only after the retained scene text path is realized.
   - Colorbar rendering after the scene path owns its visual realization.

4. Add graph-backed techniques conservatively.
   - EDL.
   - WBOIT.
   - SSAO.
   - G-buffer and outline-oriented paths.
   - Defer each until WebGPU resource lifetime, sampled attachments, and bind-group refresh are
     solid.

5. Add parity testing.
   - Native-vs-browser stream fixture comparison.
   - Browser screenshot or readback hash checks where deterministic enough.
   - Semantic validation parity for bad streams.
   - Performance counters for resource churn and frame time.

### Exit Criteria

1. The browser backend is a documented supported target for a defined scene subset.
2. Unsupported features fail with explicit diagnostics.
3. Native and browser fixture coverage share the same DRP2 contract.
4. Performance-sensitive demos avoid unexpected resource churn.


## Suggested Implementation Order

1. Add a portable DRP2/scene build target without WASM first.
2. Fix header and enum leakage exposed by that target.
3. Add the Emscripten build profile.
4. Export a tiny C API for creating and emitting a point scene.
5. Feed that stream into the existing WebGPU runner.
6. Harden the WebGPU runtime strict path around that scene-emitted stream.
7. Add primitive and image scenes.
8. Add incremental uniform-buffer updates for pan/zoom.
9. Replace JSON payload transport on the hot path.
10. Add browser app examples and then broaden feature parity.


## Validation Matrix

Use progressively broader validation as the stages advance.

1. Native portable build:
   - `cmake` or `just` target with Vulkan/canvas disabled.
   - `drp2` semantic tests that do not require vklite.
   - `scene` emission tests that request WGSL and do not execute through vklite.

2. WASM build:
   - Emscripten compile.
   - Smoke call from Node or browser to create/destroy a scene.
   - Smoke call to emit a WGSL stream.

3. Browser runtime:
   - `examples/webgpu/fixtures.html`.
   - `just webgpu-fixture-preflight`.
   - Manual canvas smoke for scene point/primitive/image.

4. Interaction:
   - Pan/zoom repeated-frame smoke.
   - Resize smoke.
   - Multi-panel smoke.
   - Runtime resource-count or trace checks to catch full rebuilds.

5. Parity:
   - Native scene-emitted stream and browser scene-emitted stream command comparison.
   - Browser readback/screenshot hashes for small deterministic fixtures.
   - Native vklite tests remain green after enum and contract cleanup.


## Risk Register

1. Public Vulkan enum leakage may be deeper than the current headers suggest.
   - Mitigation: add the portable no-Vulkan build target early and let it expose all leaks.

2. JSON transport may hide performance problems until late.
   - Mitigation: treat JSON as Stage 2 only and schedule binary/direct payload transport in Stage 4.

3. WebGPU alignment and copy-pitch rules differ from current DRP2 assumptions.
   - Mitigation: keep DRP2 semantics portable and let backends adapt, but make adaptation explicit
     in compatibility docs and tests.

4. Point rendering differs between Vulkan and WebGPU.
   - Mitigation: keep public point semantics stable and lower to native point-list or instanced
     quad/billboard according to target capabilities.

5. Multi-pass techniques can stress resource lifetime and bind-group refresh.
   - Mitigation: postpone WBOIT, EDL, SSAO, and G-buffer browser parity until single-pass retained
     resources and resize are robust.

6. WASM memory ownership can become ambiguous.
   - Mitigation: document every exported pointer as owned or borrowed and enforce explicit destroy
     calls for owned outputs.

7. Browser capability differences may fragment scene behavior.
   - Mitigation: feed a browser-derived `DvzCapabilitySnapshot` into scene emission and make
     unsupported lowering paths produce diagnostics rather than fallback surprises.


## First Milestone Definition

The first milestone should be deliberately small:

1. Build a WASM module containing portable scene and DRP2 stream emission.
2. From browser JavaScript, create:
   - one scene,
   - one figure,
   - one panel,
   - one point visual with position, color, and size data.
3. Request WGSL scene emission with an external browser canvas color target.
4. Submit the emitted DRP2 stream to the WebGPU runtime.
5. Render points as the portable WebGPU lowering selected by scene emission.

This proves the essential ownership boundary before investing in transport optimization or broader
feature parity.
