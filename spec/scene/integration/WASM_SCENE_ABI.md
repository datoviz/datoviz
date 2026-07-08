# WASM Scene ABI

Status: experimental design for replacing the browser demo bridge with a reusable scene ABI.

The proven runtime architecture stays unchanged:

```text
C/WASM scene state -> scene frame artifact -> setup/update/frame packet spans ->
browser WebGPU runtime -> canvas
```

The artifact owns the backend-neutral DRP2 stream snapshot. WASM exposes packet spans as borrowed
views of the current artifact for browser execution, and exposes JSON only as a debug or fixture
projection of that artifact stream snapshot.

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
5. Do not optimize payload transport until the object/lifetime contract is stable.

## Object Model

The generic ABI uses handles for these objects:

| Object | Initial role |
| --- | --- |
| scene | Owns the retained scene, diagnostics, and current emitted artifact handle. |
| figure | Retained figure created inside a scene. |
| panel | Retained panel created inside a figure. |
| visual | Retained visual created inside a scene. |
| controller | Retained controller created inside a scene. |

Initial functions should be deliberately small:

```c
uint32_t dvz_wasm_api_scene(uint32_t width, uint32_t height);
uint32_t dvz_wasm_api_scenario_count(void);
uint32_t dvz_wasm_api_scenario_id(uint32_t index);
uint32_t dvz_wasm_api_scenario_title(uint32_t index);
uint32_t dvz_wasm_api_scenario_width(uint32_t index);
uint32_t dvz_wasm_api_scenario_height(uint32_t index);
double dvz_wasm_api_scenario_fps(uint32_t index);
uint32_t dvz_wasm_api_scenario_requirements(uint32_t index);
int dvz_wasm_api_scenario_create(uint32_t scene, uint32_t index);
uint32_t dvz_wasm_api_scenario_figure(uint32_t scene);
int dvz_wasm_api_scenario_frame(uint32_t scene, double t, double dt);
uint32_t dvz_wasm_api_figure(uint32_t scene, uint32_t width, uint32_t height);
uint32_t dvz_wasm_api_panel_full(uint32_t figure);
uint32_t dvz_wasm_api_visual(uint32_t scene, uint32_t visual_type, uint32_t flags);
uint32_t dvz_wasm_api_buffer(uint32_t scene, uint32_t usage, uint32_t stride, uint32_t byte_size);
int dvz_wasm_api_buffer_set_data(uint32_t buffer, const void* data, uint32_t byte_size);
int dvz_wasm_api_panel_add_visual(uint32_t panel, uint32_t visual);
int dvz_wasm_api_visual_set_f32(uint32_t visual, const char* attr, const float* data, uint32_t item_count);
int dvz_wasm_api_visual_set_rgba8(uint32_t visual, const char* attr, const uint8_t* data, uint32_t item_count);
int dvz_wasm_api_visual_set_attr_buffer(
    uint32_t visual, const char* attr, uint32_t buffer, uint32_t byte_offset, uint32_t item_count);
int dvz_wasm_api_visual_set_texture_rgba8(uint32_t visual, const uint8_t* rgba, uint32_t width, uint32_t height);
int dvz_wasm_api_visual_set_material(uint32_t visual, uint32_t model, ...material scalars...);
int dvz_wasm_api_visual_set_segment_caps(uint32_t visual, uint32_t start_cap, uint32_t end_cap);
int dvz_wasm_api_visual_set_path_caps(uint32_t visual, uint32_t start_cap, uint32_t end_cap);
int dvz_wasm_api_visual_set_path_join(uint32_t visual, uint32_t join, float miter_limit);
uint32_t dvz_wasm_api_controller(uint32_t scene, uint32_t controller_type);
int dvz_wasm_api_panel_bind_controller(uint32_t panel, uint32_t controller, uint32_t dims);
int dvz_wasm_api_emit(uint32_t scene, uint32_t figure);
uint32_t dvz_wasm_api_payload_ptr(uint32_t scene);
uint32_t dvz_wasm_api_payload_size(uint32_t scene);
int dvz_wasm_api_emit_packets(uint32_t scene, uint32_t figure);
int dvz_wasm_api_release_packets(uint32_t scene);
int dvz_wasm_api_packet_status(uint32_t scene);
uint32_t dvz_wasm_api_packet_ptr(uint32_t scene, uint32_t kind);
uint32_t dvz_wasm_api_packet_size(uint32_t scene, uint32_t kind);
uint32_t dvz_wasm_api_packet_arena_ptr(uint32_t scene, uint32_t kind);
uint32_t dvz_wasm_api_packet_arena_size(uint32_t scene, uint32_t kind);
uint32_t dvz_wasm_api_resource_version(uint32_t scene);
uint32_t dvz_wasm_api_frame_index(uint32_t scene);
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
5. Buffer-backed attributes use scene-owned buffers with 32-bit WASM byte sizes and offsets in this
   experimental browser ABI.
6. Texture uploads start with tightly packed RGBA8 2D data only.

## Portable Scenario Rules

The scenario ABI is the first bridge from reusable C examples to the browser host:

1. `dvz_wasm_api_scenario_count()` and metadata accessors expose the compiled scenario registry.
2. `dvz_wasm_api_scenario_create()` initializes one scenario inside an existing scene and must be
   called before manual object construction on that scene.
3. Unsupported requirement bits fail before partial rendering and report diagnostics.
4. `dvz_wasm_api_scenario_figure()` returns the retained figure created by scenario init.
5. `dvz_wasm_api_scenario_frame()` is driven by browser `requestAnimationFrame`; it runs the
   scenario frame callback and may coexist with an older emitted artifact because artifact memory is
   immutable until explicit release.
6. The first promoted scenario is `features_timer_animation`; broad live-example coverage is still
   an RC target, not current support.

## Packet, Payload, And Diagnostics Lifetime

1. Each successful `dvz_wasm_api_emit_packets()` or `dvz_wasm_api_emit()` creates a new current
   scene frame artifact, replacing any previous artifact for that scene.
2. `dvz_wasm_api_emit_packets()` is the browser runtime path. It exposes setup, update, and frame
   binary DRP2 packets plus one payload arena per packet kind from the current frame artifact.
3. Packet and arena pointers are borrowed from the current frame artifact and valid only until
   `dvz_wasm_api_release_packets()`, the next emit call on the same scene, or scene destruction.
4. JS must decode, execute, or copy packet spans immediately and must not retain packet or arena
   WASM views after artifact release.
5. `dvz_wasm_api_resource_version()` and `dvz_wasm_api_frame_index()` report the counters captured
   by the current frame artifact when one is live; after release they report the scene counters.
   JavaScript should use these values to detect stale packet sets and reset/recreate retained
   browser runtime resources when resource versions change incompatibly.
6. Retained scene mutation is legal immediately after artifact creation because the artifact owns
   its stream snapshot, payload bytes, and packet spans. Mutations affect later emits only.
7. `dvz_wasm_api_emit()` remains a debug and fixture-export path. Its JSON payload pointer is
   borrowed with the same lifetime rules and is not a browser render path.
8. Diagnostics are borrowed with the same lifetime as the current diagnostic report or frame
   artifact.
9. Successful emits should leave diagnostics empty.
10. Failed emits and rejected operations should return `-1`; detailed diagnostics are required where
    the scene/DRP2 layer can explain the failure.

## Current PoC Compatibility

The demo-specific `src/wasm/scene_bridge.c` path has been retired. The generic ABI now covers:

1. 2D buffer-backed point/pixel positions + marker + styled segment/path + primitive + image +
   low-level atlas glyph + semantic bitmap text + mesh + panzoom;
2. 3D sphere + textured material mesh + arcball;
3. one portable C scenario, `features_timer_animation`, with browser-driven frame callbacks and
   retained point updates.

The browser pages and `just wasm-scene-smoke` use the generic `dvz_wasm_api_*` object ABI.

## Migration Plan

1. Done: add the generic ABI with scene/figure/panel/point support and a smoke test.
2. Done: add scene buffers, buffer-backed point/pixel positions, segment/path cap-join controls,
   primitive, image, low-level atlas glyph, semantic bitmap text, basic/textured/material mesh,
   basic sphere, panzoom, camera, and arcball coverage through generic calls.
3. Done: add a small JS wrapper over the ABI.
4. Done: move the browser demos to the JS wrapper and retire the demo-specific bridge.
5. Done: replace JSON hot-path transport with split binary DRP2 packets and payload arenas.
6. Done: remove the legacy direct-payload JSON ABI exports.
7. Done: add the first portable scenario registry/frame ABI for `features_timer_animation`.
