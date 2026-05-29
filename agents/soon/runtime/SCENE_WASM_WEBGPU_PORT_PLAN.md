# Scene/WASM/WebGPU Port Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-30`
> - **Purpose:** track the staged route for running the active scene code in the browser through
>   portable scene emission, WASM, WGSL DRP2 streams, and WebGPU execution.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/integration/WEBGPU_WASM.md`](../../../spec/scene/integration/WEBGPU_WASM.md)
2. [`../../../spec/scene/api/WASM_PORTABILITY.md`](../../../spec/scene/api/WASM_PORTABILITY.md)
3. [`../../../spec/drp2/`](../../../spec/drp2/)

Use this file only for implementation order, risk, and validation. Do not duplicate the browser
architecture, public scene API portability rules, DRP2 command contracts, or transport policy here.

The goal remains a documented, narrow scene/DRP2/WGSL/WebGPU subset that can run in a browser
without porting `vk`, `vklite`, `canvas`, `window`, `stream`, `video`, or native `app`.


## Readiness Snapshot

As of 2026-05-30, the refactored scene structure is ready to start the WASM export slice, but the
WASM browser path itself is not implemented.

What is ready:

1. Scene internals are split into ownership areas that can support a portable subset: core scene
   state, domain resources, frame plans, render contracts, scene emission, queries, annotations,
   techniques, runtime bridge code, shader registry code, and family-owned visual directories.
2. WGSL scene emission exists for the current WebGPU-oriented subset, with point, primitive, and
   image scene fixtures already feeding the browser runner.
3. The pure WebGPU runner remains closed for the current RC experimental subset:
   `just webgpu-fixture-preflight` passes `39/39`, and `just webgpu-runner-smoke` passes `37`
   positive fixtures, `2` WebGPU streams, and `81` semantic negative fixtures.
4. A native portability probe can build scene plus DRP2 with the native presentation stack disabled:
   `DVZ_BUILD_VK=OFF`, `DVZ_BUILD_CANVAS=OFF`, `DVZ_BUILD_APP=OFF`, and `DVZ_BUILD_GUI=OFF`.

What is not ready:

1. `src/wasm` is still an empty scaffold; there is no WASM C API, Emscripten profile, or exported
   create/emit/destroy surface.
2. `src/scene/CMakeLists.txt` still builds one monolithic `datoviz_scene` object from all scene
   source groups, including native runtime bridge code, techniques, SPIR-V/GLSL/WGSL generated
   assets, and embedded fonts.
3. `src/drp2/CMakeLists.txt` has a partial portable split, but `runtime.c` remains in the always-built
   DRP2 object target.
4. `include/datoviz/drp2/runtime.h` still exposes native-flavored runtime configuration and helper
   types such as `DvzDevice`, `DvzVma`, `DvzBuffer`, and `DvzStreamFrame`.
5. The native portability probe is useful evidence, but it is not yet the clean minimal
   scene/DRP2/WGSL target required for Emscripten.


## Remaining Port Work

Recommended follow-up commits:

1. Add a portable native build target for scene plus DRP2 stream/semantic/serialization code with
   Vulkan, vklite, canvas, native app, native runtime bridge, shaderc/dlopen, and browser runtime
   ownership disabled. The target should build only the subset intended to cross into WASM.
2. Split or gate the current monolithic scene and DRP2 object targets so the portable target can
   exclude `src/scene/runtime/`, native technique/runtime bridge helpers, and DRP2 vklite runtime
   entry points while retaining WGSL scene emission.
3. Fix any public header leakage exposed by that target, especially native runtime, Vulkan, canvas,
   stream-frame, window, or platform types.
4. Move vklite-specific DRP2 runtime config and borrowed-frame helpers behind native-only headers,
   keeping portable stream construction, JSON/debug serialization, and semantic validation public.
5. Add an Emscripten build profile for the portable scene/DRP2 subset.
6. Export a tiny C/WASM API for creating/destroying a scene, figure, panel, and point visual, then
   emitting one WGSL DRP2 frame stream.
7. Feed the scene-emitted point stream into the existing WebGPU runner without demo-specific stream
   rewriting.
8. Add primitive and image scenes after the point milestone is stable.
9. Add incremental uniform-buffer updates for pan/zoom and resize by routing browser input through
   WASM scene/controller calls that emit DRP2 `WriteBuffer` and frame-update commands. Do not make
   the WebGPU runtime mutate scene uniforms through private buffer ids or browser-owned interaction
   metadata.
10. Replace JSON/base64 hot-path transport with direct payload spans or a compact binary command
   path once the debugging path is proven.
11. Add browser app examples only after the runtime can preserve resources across repeated frame
   submissions.
12. Broaden parity to readback, multi-panel, linked controllers, mesh/path/sphere/text/colorbar,
   and graph-backed techniques only after the core path is usable and efficient.


## First Milestone

1. Build a WASM module containing portable scene and DRP2 stream emission.
2. From browser JavaScript, create one scene, one figure, one panel, and one point visual.
3. Request WGSL scene emission with an external browser canvas color target.
4. Submit the emitted DRP2 stream to the WebGPU runtime.
5. Render points using the portable WebGPU lowering selected by scene emission.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation slices, use progressively broader validation:

```text
portable native build without Vulkan/canvas
DRP2 semantic tests that do not require vklite
scene emission tests requesting WGSL without vklite execution
Emscripten compile
Node or browser smoke for create/destroy and stream emission
just webgpu-fixture-preflight
```

Add browser point/primitive/image canvas smokes, repeated-frame resource-count checks, and
native/browser stream comparisons as the runtime moves beyond the first milestone.
