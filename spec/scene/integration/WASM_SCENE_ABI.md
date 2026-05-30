# WASM Scene ABI

Status: experimental design for replacing the browser demo bridge with a reusable scene ABI.

The proven runtime architecture stays unchanged:

```text
C/WASM scene state -> scene FramePlan -> WGSL DRP2 stream -> browser WebGPU runtime -> canvas
```

This document defines the next ABI layer. It does not replace the public native C API and does not
port Vulkan, vklite, canvas, window, stream, or app to WASM.

## Goals

1. Keep scene semantics, visual lowering, controller math, camera state, and DRP2 emission in C/WASM.
2. Keep browser JavaScript responsible for WebGPU adapter/device/context acquisition and DRP2 replay.
3. Expose opaque `uint32_t` handles to JavaScript rather than direct struct access.
4. Provide a small generic object API before adding more visual-specific demo calls.
5. Make ownership, payload lifetime, diagnostics, and capability negotiation explicit.
6. Allow a generated or metadata-driven JS/TS wrapper once the ABI stabilizes.

## Non-Goals

1. Do not bind every native C function directly.
2. Do not expose native pointers, struct layouts, callbacks, or borrowed internal arrays as a stable
   browser contract.
3. Do not make the browser runtime understand Datoviz visual families outside DRP2 commands.
4. Do not optimize transport before the object/lifetime contract is stable.
5. Do not remove `src/wasm/scene_bridge.c` until the generic ABI reaches equivalent smoke coverage.

## Object Model

The generic ABI uses handles for these objects:

| Object | Initial role |
| --- | --- |
| scene | Owns the retained scene, diagnostics, emitted stream, and payload. |
| figure | Retained figure created inside a scene. |
| panel | Retained panel created inside a figure. |
| visual | Retained visual created inside a scene. |
| controller | Retained controller created inside a scene. |

Initial functions should be deliberately small:

```c
uint32_t dvz_wasm_api_scene(uint32_t width, uint32_t height);
uint32_t dvz_wasm_api_figure(uint32_t scene, uint32_t width, uint32_t height);
uint32_t dvz_wasm_api_panel_full(uint32_t figure);
uint32_t dvz_wasm_api_visual(uint32_t scene, uint32_t visual_type, uint32_t flags);
int dvz_wasm_api_panel_add_visual(uint32_t panel, uint32_t visual);
int dvz_wasm_api_visual_set_f32(uint32_t visual, const char* attr, const float* data, uint32_t item_count);
int dvz_wasm_api_visual_set_rgba8(uint32_t visual, const char* attr, const uint8_t* data, uint32_t item_count);
int dvz_wasm_api_visual_set_texture_rgba8(uint32_t visual, const uint8_t* rgba, uint32_t width, uint32_t height);
uint32_t dvz_wasm_api_controller(uint32_t scene, uint32_t controller_type);
int dvz_wasm_api_panel_bind_controller(uint32_t panel, uint32_t controller, uint32_t dims);
int dvz_wasm_api_emit(uint32_t scene, uint32_t figure);
uint32_t dvz_wasm_api_payload_ptr(uint32_t scene);
uint32_t dvz_wasm_api_payload_size(uint32_t scene);
uint32_t dvz_wasm_api_diagnostic_count(uint32_t scene);
uint32_t dvz_wasm_api_diagnostic(uint32_t scene, uint32_t index);
void dvz_wasm_api_scene_destroy(uint32_t scene);
```

## Data Upload Rules

1. JS passes typed-array spans through WASM memory.
2. The C scene layer copies data into retained scene storage before returning.
3. JS may free the temporary WASM allocation immediately after a successful upload.
4. Attribute names are UTF-8 C strings for the first slice; a later ABI may replace them with
   generated numeric attribute ids.
5. Texture uploads start with tightly packed RGBA8 2D data only.

## Payload And Diagnostics Lifetime

1. Emitted payload pointers are borrowed and valid only until the next mutating ABI call on the same
   scene or scene destruction.
2. JS must decode or copy payload bytes before calling another ABI function on that scene.
3. Diagnostics are borrowed with the same lifetime as the current diagnostic report.
4. Successful emits should leave diagnostics empty.
5. Failed emits and rejected operations should return `-1`; detailed diagnostics are required where
   the scene/DRP2 layer can explain the failure.

## Current PoC Compatibility

`src/wasm/scene_bridge.c` remains the demo-specific bridge for:

1. 2D point + primitive + image + mesh + panzoom;
2. 3D mesh + arcball.

The generic ABI should be added beside it and validated independently. Once the generic ABI can
rebuild the same demos, the demo bridge can become a compatibility shim or be removed.

## Migration Plan

1. Add the generic ABI with scene/figure/panel/point support and a smoke test.
2. Add primitive, image, mesh, and controller coverage through generic calls.
3. Add a small JS wrapper over the ABI.
4. Move the browser demos to the JS wrapper.
5. Replace JSON hot-path transport only after the object ABI is stable.
