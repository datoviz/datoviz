# WebGPU And WASM Integration

This document defines the durable architecture for the experimental scene-to-browser path.

The v0.4 goal is a documented, narrow scene/DRP2/WGSL/WebGPU subset that can run in a browser. It
is not native Vulkan feature parity.

## Architecture

The browser path should run the active retained scene layer without porting the native Vulkan
presentation stack.

```text
C/WASM scene state
  -> scene FramePlan
  -> WGSL DRP2 command stream
  -> browser WebGPU DRP2 runtime
  -> HTML canvas
```

The scene layer remains the owner of scene semantics, visual lowering, controller state, shader
selection, resource ids, and request bookkeeping. The browser runtime executes portable DRP2
streams. It must not know about Datoviz visual families except through the DRP2 contract.

Interaction state follows the same boundary. Browser JavaScript may translate DOM events into
scene/controller API calls, but the WASM scene layer owns the panzoom/camera/controller math and
emits the resulting DRP2 updates, such as `WriteBuffer` commands plus frame commands. The WebGPU
runtime must execute those DRP2 commands directly; it must not mutate scene uniform buffers from
private buffer ids, visual-family assumptions, or browser-only metadata.

## Non-Goals

1. Do not port `vk`, `vklite`, `canvas`, `window`, `stream`, `video`, or native `app` directly to
   WASM.
2. Do not fork scene semantics for WebGPU.
3. Do not make scene techniques WebGPU-specific.
4. Do not add browser-side GLSL-to-WGSL translation.
5. Do not promote deferred DRP2 commands only to mirror native Vulkan or browser WebGPU object
   models.
6. Do not block the first slice on every visual family, technique, or readback feature.

## Portable Scene And DRP2 Boundary

The first hardening step is to make DRP2 stream emission and scene planning portable without native
runtime headers leaking into browser-facing code.

Rules:

1. keep DRP2 command streams, stream construction, JSON/debug serialization, and semantic validation
   in the portable surface;
2. move vklite-specific runtime entry points and borrowed native frame helpers behind native-only
   runtime headers;
3. keep `DvzDrp2Runtime` opaque where validation/runtime handles are needed;
4. move native runtime types such as devices, allocators, buffers, and external native descriptors
   to native-only APIs;
5. replace Vulkan-shaped public DRP2 values with DRP2-owned enums;
6. keep backend mapping local;
7. keep public scene headers free of Vulkan, vklite, canvas, stream-frame, and native window
   headers.

Validation target:

1. a native build can compile scene plus portable DRP2 stream/semantic code with Vulkan and canvas
   disabled;
2. public portable scene/DRP2 headers remain free of native runtime types;
3. native vklite DRP2 tests pass through explicit backend mapping;
4. WebGPU fixture preflight sees the same portable command semantics.

## WASM Scene Module

The first WASM module should be narrow and explicit, built with Emscripten.

Required modules:

1. `common`;
2. selected `ds`;
3. `math`;
4. portable DRP2 stream and semantic pieces;
5. `scene`.

Initially excluded modules include native file I/O where avoidable, thread, Vulkan, vklite, canvas,
window, stream, video, native app, GUI, CUDA, native DVZR tools, and shaderc/dlopen paths.

Closed first-slice milestone:

1. `src/wasm/scene_api.c` exports a generic handle-based scene/figure/panel/visual/controller API.
2. The browser scene slices create point, pixel, primitive, image, mesh, panzoom, camera, and arcball
   objects through the generic ABI.
3. Scene emission can request WGSL, target the external browser canvas format, return DRP2 JSON,
   and feed the browser WebGPU runtime.
4. `just wasm-scene-smoke` builds the Emscripten target, emits 2D and 3D generic ABI streams, and
   runs the WebGPU fixture preflight on the emitted JSON.

Remaining hardening before treating the slice as release-proof:

1. The scene target remains too monolithic for a minimal portable subset.
2. DRP2 native runtime headers still expose native types in places that should be portable.
3. Public portable DRP2 and scene headers must not require Vulkan, vklite, canvas, stream-frame,
   window, or native runtime types.
4. WASM bridge pointer ownership, payload lifetime, diagnostics, and browser capability negotiation
   need to be documented as the experimental ABI contract.

The exported C/WASM API should cover:

1. create and destroy a scene;
2. create a figure and panels;
3. create a small set of visuals;
4. upload or update visual data;
5. update figure size and controller state;
6. emit one WGSL DRP2 frame stream;
7. return stream JSON or an owned pointer/length to encoded commands;
8. destroy emitted streams and owned payload buffers.

Every exported pointer must have explicit ownership: borrowed until the next call, or owned until a
matching destroy call.

## Browser DRP2 Runtime

The browser runtime may remain JavaScript or TypeScript. C/WASM should not try to own browser GPU
objects directly.

Runtime object model:

1. buffers;
2. textures and texture views;
3. samplers;
4. bind-group layouts and bind groups;
5. shader modules;
6. render and compute pipelines;
7. command encoders and passes.

Runtime behavior must align with DRP2 semantic validation:

1. unsupported command diagnostics;
2. invalid resource-id diagnostics;
3. backend capability failures;
4. shader/pipeline creation failures;
5. command-index context for errors;
6. deterministic object destruction and use-after-destroy validation.

Strict fixture paths should not depend on proof-of-concept shortcuts such as missing pipeline
metadata fallbacks, hard-coded scene uniform ids, or browser-side shader substitution. Browser
presentation streams may use the documented browser-canvas external-target convention.

Remaining demo shortcuts should be removed before they become architecture:

1. a future non-demo external-target ABI if browser-canvas conventions need to cross runtime
   boundaries;
2. done for the current browser runner: unaligned buffer-binding offsets now fail explicitly instead
   of silently binding a different range;
3. demo-local assumptions about scene uniform buffer ids;
4. browser-side direct mutation of scene-owned uniforms for pan/zoom or resize.

Browser-side direct mutation of scene-owned GPU resources is also a proof-of-concept shortcut. It is
acceptable only for temporary static-JSON demos before the WASM scene layer exists, and should not
become the browser app architecture.

## Active WebGPU Command Subset

The browser runtime should implement every active DRP2 command used by the supported subset, or
return explicit unsupported-feature diagnostics.

Implemented active-command parity in the committed browser runner includes:

1. `SetViewport`;
2. `SetScissor`;
3. `SetBlendConstant`;
4. `SetStencilReference`;
5. `CopyBufferToBuffer`;
6. destroy/lifetime validation for use-after-destroy and recorded/submitted-work references;
7. multiple color attachments and color-target validation.

Immediate runtime hardening priorities are now:

1. real browser dashboard evidence for the committed subset;
2. repeated-frame resource preservation checks;
3. browser capability reporting and explicit unsupported-feature diagnostics;
4. removal of remaining demo-local shortcuts where practical.

Deferred commands stay deferred until a concrete use case promotes them into the active DRP2
contract:

1. `CreatePipelineLayout`;
2. `DestroyPipelineLayout`;
3. `ResourceBarrier`;
4. indirect draw and dispatch commands.

Promotion must update DRP2 command specs, schemas, native semantic validation, WebGPU execution,
fixtures, and lifecycle rules together.

## Transport And Incremental Updates

JSON remains a debugging and fixture format, not the final hot path.

Transport levels:

1. JSON stream returned by WASM;
2. retired: JSON commands with binary payloads passed as borrowed WASM memory views;
3. done for the browser wrapper: compact binary command packets with payload arenas;
4. done for the browser wrapper: split setup/update/frame packets with stable resource ids and
   minimal per-frame payload.

Hot-path browser transport should avoid base64, avoid resending unchanged visual data, and preserve
browser runtime GPU resources across repeated frame submissions.

Resource update rules:

1. stable scene resources keep stable DRP2 ids;
2. scene emits `Create*` only when a resource is new or recreated;
3. repeated frames use buffer, texture, or uniform updates;
4. resize recreates extent-dependent targets and refreshes or invalidates dependent bind groups;
5. browser runtime must not retain borrowed WASM memory after the caller frees or overwrites it.
6. same-shape visual data updates should emit update-only streams that replay against retained
   browser runtime state;
7. setup-bearing visual updates, such as texture size changes, require retained runtime setup
   update before frame replay;
8. visual item-count growth is rejected by the current per-attribute ABI until a batch update or
   explicit topology-rebuild contract exists.

## Break-Compatible Long-Term Plan

If v0.4 development can break the current experimental WebGPU/WASM compatibility layer, the
preferred architecture is to stop extending JSON as the runtime protocol and move directly to a
structured browser runtime boundary.

Target architecture:

1. make binary DRP2 packets the primary WASM-to-browser transport;
2. keep JSON only for fixture export, debugging, and human-readable evidence;
3. split every scene emission into explicit `setup`, `update`, and `frame` packets;
4. remove JavaScript heuristics that infer setup commands from command names;
5. make resource lifetime explicit in DRP2 with stable ids, create/update/destroy commands,
   dependency rules, and no implicit reload semantics;
6. introduce a browser runtime session object that owns the WebGPU device, canvas context, retained
   resource tables, packet execution, resize, input, render scheduling, diagnostics, and destroy;
7. make the WASM scene bridge return a structured frame result containing status, diagnostics,
   packet pointers and sizes, a payload arena pointer and size, and resource/version counters;
8. use a payload arena with stable offsets referenced by commands, instead of per-payload pointers
   or base64 data;
9. route browser input and resize through a formal scene event queue consumed by WASM scene state;
10. keep JavaScript responsible for browser event normalization and WebGPU execution, but never for
    scene-owned controller math or direct mutation of scene-owned GPU resources.

Compatibility to remove:

1. JSON as the browser runtime hot path;
2. base64 write payloads in runtime execution;
3. inferred setup/frame splitting;
4. demo-local shader metadata fallbacks;
5. hard-coded scene uniform ids or browser-side scene uniform updates;
6. demo-specific browser canvas assumptions outside a declared external-target contract;
7. legacy v0.3-facing API compromises inside the experimental browser path.

Execution sequence:

1. write the binary DRP2 packet, payload arena, and split setup/update/frame contract in `spec/drp2/`;
2. implement native packet encode/decode tests before changing browser execution;
3. change scene emission to produce split frame results with explicit resource versions;
4. replace the WASM ABI with split packet pointers, payload arena metadata, diagnostics, and
   lifecycle ownership rules;
5. rewrite the browser WebGPU runtime to execute packets against retained resource tables;
6. port the current 2D and 3D WASM demos onto the browser runtime session object;
7. delete JSON execution from the demos while preserving JSON fixture/debug export;
8. rebuild `just wasm-scene-smoke` and `just webgpu-browser-smoke` around the new session contract;
9. only then expand the browser visual-family subset.

This plan intentionally preferred a larger compatibility break over continued incremental polish of
the retired direct-payload JSON bridge. The bridge was useful transition evidence, but it is not the
browser runtime protocol.

## Browser App Layer

The browser app layer is equivalent in role to native `app`, but it is not a port of native
window/canvas internals.

Responsibilities:

1. HTML canvas creation or attachment;
2. WebGPU adapter/device/context acquisition;
3. canvas format and target texture management;
4. DPR-aware sizing;
5. animation-frame scheduling;
6. browser input translation into scene controllers;
7. browser status and diagnostics.

JavaScript should translate browser events into scene/controller calls. It should not duplicate
controller math unless that helper is explicitly browser-only.

Now that point-slice WASM scene emission exists, browser interaction should continue to flow through
scene/controller calls that emit DRP2 update streams. The WebGPU runtime should not expose a
separate interaction API for updating retained scene buffers.

## Capability And Diagnostics

The browser runtime must report a capability snapshot aligned with DRP2 negotiation and validation.

It should report:

1. shader formats, expected to be WGSL for the browser path;
2. texture formats, usages, sample counts, and limits;
3. bind-group, binding, dynamic-offset, alignment, and buffer-copy-row constraints;
4. unsupported features separately from malformed stream validation;
5. device loss, WebGPU validation errors, and asynchronous mapping failures as DRP2-level
   diagnostics where practical.

Scene emission should use browser capabilities to reject unsupported lowering paths with explicit
diagnostics rather than silent fallbacks.

## First Milestone

The first milestone is deliberately small and is now represented by the committed point/panzoom
bridge:

1. build a WASM module containing portable scene and DRP2 stream emission;
2. from browser JavaScript, create one scene, one figure, one panel, and point, pixel, primitive
   triangle-list, RGBA8 image, and basic mesh visuals;
3. request WGSL scene emission with an external browser canvas color target;
4. submit the emitted DRP2 stream to the WebGPU runtime;
5. render points using the portable WebGPU lowering selected by scene emission.

The current first slice uses the generic `src/wasm/scene_api.c` object ABI for visuals,
controllers, resize/pointer/wheel routing, and emitted split DRP2 packets.

Current evidence as of 2026-06-05:

1. `just wasm-scene-smoke` builds the Emscripten scene ABI, emits
   `build-wasm-scene/wasm/wasm_api_scene_point_pixel_primitive_image_mesh_panzoom.json` and
   `build-wasm-scene/wasm/wasm_api_scene_mesh3d_arcball.json`, and passes WebGPU fixture preflight.
2. The browser demo entry point is `examples/webgpu/examples.html`, with `demo=wasm-2d` and
   `demo=wasm-3d` query parameters selecting the release-visible WASM demos.
3. Manual local browser proof confirms the point scene renders and panzoom interaction works through
   the generic WASM scene ABI.
4. Manual local browser proof at
   `http://localhost:8765/examples/webgpu/examples.html?demo=wasm-2d` confirms
   point + primitive rendering, pan/zoom, and resize with no visible browser/WebGPU runtime errors.
5. Reloading the same live demo after the image slice confirms point + primitive + RGBA8 image
   rendering, pan/zoom, resize, and cache-busted WASM load with no visible browser/WebGPU runtime
   errors.
6. Reloading the same live demo after the mesh slice confirms point + primitive + RGBA8 image +
   basic mesh rendering, pan/zoom, and resize with no visible browser/WebGPU runtime errors.
7. Loading `http://localhost:8765/examples/webgpu/examples.html?demo=wasm-3d` confirms the 3D cube
   renders, arcball drag rotates it, wheel zoom works, resize works, and no visible browser/WebGPU
   runtime errors occur.
8. Manual local browser proof after the unified examples host split confirms both release-visible
   demos render and interact through `examples/webgpu/examples.html`.

The supported browser-scene subset now covers point, pixel, primitive triangle-list, RGBA8 image,
basic mesh, panzoom, and a first 3D mesh + arcball scene. The browser wrapper passes normalized WebGPU
limits into the WASM scene emitter before figure creation, and the old direct browser-side panzoom
uniform mutation path has been retired from the release-visible demos. Retained browser runtime
stress now tracks browser-owned frame resources and retires submitted references after explicit
queue completion in retained sessions. Automated browser smoke now also checks pagehide scene
destruction, packet-runtime use, and the fixture dashboard's WASM Scene Smoke rows for 2D point
and pixel updates, 2D image texture resize reloads, and 3D mesh retained updates. The browser
wrapper now uses split binary DRP2 setup/update/frame packets and payload arenas for WASM scene
rendering. JSON and payload-ref JSON remain available for fixture/debug export, not as the browser
scene runtime path.
The next release-proofing gaps are broader visual/technique parity and continued browser-runtime
hardening.

### Experimental WASM Scene ABI

The `src/wasm/scene_api.c` API is an unstable experimental ABI for the browser examples:

1. scene handles are opaque `uint32_t` values; JavaScript must pass them back unchanged and must not
   derive addresses or object state from them;
2. `dvz_wasm_api_emit_packets()` is the browser runtime emit path and exposes split binary DRP2
   setup/update/frame packets plus payload arenas through the packet accessors documented in
   `spec/drp2/PACKETS.md`;
3. packet pointers, arena pointers, and diagnostic strings are borrowed WASM linear-memory spans
   valid only until the next mutating bridge call on the same handle or
   `dvz_wasm_api_scene_destroy()`;
4. JavaScript decodes and executes packets immediately and must not retain packet or arena views
   across another bridge call;
5. `dvz_wasm_api_emit()` plus `dvz_wasm_api_payload_ptr()` and
   `dvz_wasm_api_payload_size()` expose borrowed UTF-8 JSON for debug and fixture export only;
6. JavaScript must not use JSON as the browser render path; runtime rendering consumes the split
   packet accessors and their payload arenas;
7. JavaScript must copy or decode the debug JSON payload before calling resize, pointer, wheel,
   data-upload, emit, canvas-format, capability, or destroy functions again;
8. diagnostics returned by `dvz_wasm_api_diagnostic()` are borrowed strings with the same lifetime
   as the current diagnostic report, valid only until the next bridge call on the same handle or
   destroy;
9. successful emits should leave the diagnostic report empty; failed emits must be surfaced with
   non-empty diagnostics where the scene/DRP2 layer can explain the failure;
10. the supported browser canvas formats are `rgba8unorm` and `bgra8unorm`, mapped to
   `DVZ_FORMAT_R8G8B8A8_UNORM` and `DVZ_FORMAT_B8G8R8A8_UNORM`;
11. `dvz_wasm_api_set_capabilities()` accepts browser-normalized texture dimension, bind-group,
   vertex-buffer, buffer-size, texture-copy alignment, and sample-count limits used by scene
   emission;
12. resize arguments are framebuffer pixel width/height plus device scale; the bridge derives logical
   window size for the scene input router;
13. pointer and wheel positions are CSS-pixel canvas coordinates plus content scale, so high-DPI
   browsers keep controller math in the scene layer while still using framebuffer-sized targets.

## Validation Matrix

Use progressively broader validation:

1. portable native build without Vulkan/canvas;
2. DRP2 semantic tests that do not require vklite;
3. scene emission tests requesting WGSL without vklite execution;
4. Emscripten compile;
5. Node or browser smoke for create/destroy and stream emission;
6. `just webgpu-fixture-preflight`;
7. browser fixture execution;
8. scene point/primitive/image canvas smoke;
9. resize and repeated-frame resource-count checks;
10. native/browser scene-emitted stream comparisons where deterministic.

Retained-runtime browser stress should load a stream once, render repeated frames, require stable
resource counts, and require `refs.open == 0` and `refs.recorded == 0` after each render. Fixture
compatibility, runtime stress, and WASM scene smoke counts should remain reported separately.

Headless browser validation is advisory for WebGPU render proof. Headless Chrome/Dawn can report
external instance loss, including `A valid external Instance reference no longer exists` or
`Instance dropped in popErrorScope`, before the Datoviz scene contract is exercised. The automated
browser smoke may skip those render checks when that condition appears. A visible browser running
`examples/webgpu/examples.html` and the fixture dashboard remains the render validation path for the
current experimental subset.
