# Scene/WASM/WebGPU Port Plan

> **Execution Status**
> - **Status:** `PICKUP PLAN`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track the staged route for running active Datoviz scene code in the browser by
>   compiling portable scene code to WASM and executing emitted DRP2 streams through WebGPU.


## Durable Contract

The shared WebGPU/WASM integration contract lives in
[../../../spec/scene/integration/WEBGPU_WASM.md](../../../spec/scene/integration/WEBGPU_WASM.md).

The public scene API portability rules live in
[../../../spec/scene/api/WASM_PORTABILITY.md](../../../spec/scene/api/WASM_PORTABILITY.md).

This file tracks implementation order, risk, and validation only.


## Goal

Run the active retained scene path in the browser without porting the native Vulkan presentation
stack.

The v0.4 success criterion is a documented, narrow scene/DRP2/WGSL/WebGPU subset that can run in
the browser, not complete feature parity with native Vulkan.


## Stage 1: Portable Contract Cleanup

Make DRP2 stream emission and scene planning portable, independently buildable, and clearly
separated from vklite execution.

Exit criteria:

1. a native build can compile `scene + drp2` stream/semantic code with `DVZ_BUILD_VK=OFF` and
   `DVZ_BUILD_CANVAS=OFF`;
2. public portable DRP2 and scene headers do not include Vulkan, canvas, stream-frame, or vklite
   headers;
3. existing native vklite DRP2 tests continue to pass through explicit backend mapping;
4. existing WebGPU fixture preflight still sees the same portable command semantics.


## Stage 2: WASM Scene Build

Produce a minimal Emscripten WASM module that owns scene state and emits WGSL DRP2 streams.

Required first exports:

1. create and destroy a scene;
2. create a figure and panel;
3. create point, primitive, and image visuals;
4. upload/update visual data;
5. update figure size and controller state;
6. emit one WGSL DRP2 frame stream;
7. return JSON or pointer/length encoded commands;
8. destroy emitted streams and owned payload buffers.

Exit criteria:

1. a browser page can instantiate the WASM module;
2. JavaScript can create a simple scene in WASM;
3. WASM can emit a WGSL DRP2 stream for point, primitive, and image scenes;
4. the stream can be passed to the WebGPU runner and rendered.


## Stage 3: Browser Runtime Hardening

Turn the current WebGPU proof-of-concept into a runtime boundary that consumes scene-emitted
streams without demo-specific patches.

Exit criteria:

1. scene-emitted WGSL streams for point, primitive, image, and basic mesh render through WebGPU
   without custom stream rewriting;
2. strict fixture mode does not rely on demo fallbacks;
3. WebGPU runtime resources stay alive across repeated frame submissions;
4. runtime errors identify the failing DRP2 command and backend reason.


## Stage 4: Efficient Transport And Incremental Updates

Replace full-frame JSON/base64 transport with a path suitable for interactive scenes and large
datasets.

Exit criteria:

1. pan/zoom and arcball interactions update small uniform buffers rather than resending visual data;
2. repeated frames do not recreate unchanged buffers, textures, pipelines, or bind groups;
3. large payloads avoid base64 in the hot path;
4. resize works without stale bind groups or destroyed texture references.


## Stage 5: Browser App Layer

Provide a browser-native presentation layer equivalent in role to native `app`, without porting
native window/canvas internals.

Exit criteria:

1. a browser example can create a scene, interact with it, resize it, and render continuously;
2. the event loop uses retained scene state and incremental DRP2 updates;
3. the browser app layer is clearly separate from native `app`;
4. the example can be run from a simple local static server.


## Stage 6: Parity Expansion

Broaden browser support only after the core scene/WASM/WebGPU path is usable and efficient:

1. request/readback support;
2. multi-panel and linked-controller parity;
3. mesh/path/sphere/text/colorbar visual-family parity;
4. graph-backed techniques such as EDL, WBOIT, SSAO, G-buffer, and outlines;
5. native-vs-browser stream fixture comparison.


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

Use progressively broader validation:

1. native portable build with Vulkan/canvas disabled;
2. DRP2 semantic tests that do not require vklite;
3. scene emission tests that request WGSL and do not execute through vklite;
4. Emscripten compile;
5. Node or browser smoke for scene create/destroy and WGSL stream emission;
6. `examples/webgpu/fixtures.html`;
7. `just webgpu-fixture-preflight`;
8. pan/zoom repeated-frame, resize, and multi-panel browser smokes;
9. native/browser scene-emitted stream comparisons.


## First Milestone

1. Build a WASM module containing portable scene and DRP2 stream emission.
2. From browser JavaScript, create one scene, one figure, one panel, and one point visual.
3. Request WGSL scene emission with an external browser canvas color target.
4. Submit the emitted DRP2 stream to the WebGPU runtime.
5. Render points using the portable WebGPU lowering selected by scene emission.
