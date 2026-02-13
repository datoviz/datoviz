# Architecture (v0.4-dev)

This document describes the current Datoviz v0.4-dev architecture.

It replaces older v0.3-era descriptions and focuses on what is currently built, linked, and tested.


## Goals

- Single shared library target: `datoviz` (`libdatoviz.*`).
- Modular implementation through per-module object libraries.
- Clear separation between public headers (`include/datoviz`) and internal implementation (`src`).
- Unified test runner (`dvztest`) for all active modules.
- Incremental stabilization of the active graphics stack before bringing higher layers online.


## Repository layout

- Public API headers: `include/datoviz/`
- Internal implementation: `src/`
- Unified test framework and runner: `testing/`
- Vendored dependencies: `external/` (treated as read-only unless explicitly required)

The v0.3 code lives in `v0.3/` and is not part of the active v0.4-dev architecture.


## Active modules in libdatoviz

The root source build (`src/CMakeLists.txt`) currently brings these modules into `datoviz`:

- `common`
- `ds`
- `fileio`
- `math`
- `thread`
- `input`
- `window`
- `canvas`
- `stream`
- `video`
- `vk`
- `vklite`

These are added as subdirectories and linked into the shared target as object-library components.

Scaffolding modules (for example `color`, `wasm`, and higher-level renderer/scene/client layers) exist but are
not part of the active v0.4-dev link surface unless explicitly activated.


## Build topology

Datoviz uses:

- One shared target: `datoviz`
- One object library per module: `datoviz_<module>`
- One shared Vulkan entry-point provider: `datoviz_volk`

Compile definitions are centralized in `DVZ_COMPILE_DEFINITIONS` and propagated across source and test targets.
This includes OS/compiler feature flags, validation toggles, and Vulkan configuration (`VK_NO_PROTOTYPES`).


## Public vs internal boundaries

- Public API: `include/datoviz/*.h` and subheaders such as `include/datoviz/vklite/*.h`
- Shared internals: `src/common/_*.h`
- Module internals: files within each `src/<module>/` directory

In v0.4-dev, public headers still rely on shared internal macros/utilities from `src/common`, so module and test
targets keep `src/common` on include paths.


## Runtime architecture (active path)

At runtime, the active rendering path is:

1. `window` creates a backend window/surface and routes input events.
2. `canvas` binds window + device and owns per-frame presentation state.
3. `canvas` owns a `stream` object used to fan out frames to sinks.
4. `canvas_swapchain` sink handles acquire, command-buffer finalization, submit, and present.
5. Optional `video` sink consumes exported frame handles or CPU readback.
6. `vk` provides lower-level Vulkan bootstrap/device/queue/memory primitives.
7. `vklite` provides higher-level wrappers for commands, buffers, images, descriptors, graphics, compute,
   rendering, swapchain, and synchronization.


## Canvas rendering surface area

The canvas API is intentionally small. Rendering code plugs in through one callback and one frame descriptor.

### Core entry points

- Create canvas: `dvz_canvas_create()` (`include/datoviz/canvas.h`)
- Register draw callback: `dvz_canvas_set_draw_callback()` (`include/datoviz/canvas.h`)
- Acquire/update frame: `dvz_canvas_frame()` (`include/datoviz/canvas.h`)
- Submit/present frame: `dvz_canvas_submit()` (`include/datoviz/canvas.h`)
- Access underlying stream/device: `dvz_canvas_stream()` + `dvz_stream_device()` (`include/datoviz/canvas.h`,
  `include/datoviz/stream.h`)

### Draw callback contract

Callback type:

`DvzCanvasDraw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)`

The `DvzStreamFrame` gives the per-frame render handles:

- `VkCommandBuffer command_buffer`
- `VkImageView image_view`
- `VkExtent2D extent`
- exported memory/sync handles (`memory_fd`, `wait_semaphore_fd`) when relevant

Practical contract:

- Record your rendering commands into `frame->command_buffer`.
- Use `frame->image_view` as the color target for dynamic rendering.
- Treat command-buffer begin/end/reset and present as canvas-managed lifecycle.
- Create long-lived GPU objects (pipelines, descriptors, buffers, images, samplers, shaders) outside the
  callback, then bind/use them inside the callback.

### Frame lifecycle

Per frame:

1. `dvz_canvas_frame()` refreshes surface state, prepares stream/sinks, acquires swapchain image, and starts slot
   command recording.
2. Canvas invokes your draw callback with the active `DvzStreamFrame`.
3. `dvz_canvas_submit()` submits the stream.
4. Swapchain sink finalizes recording, submits queue work, signals timeline semaphore, and presents.

Internally, canvas renders into an offscreen image, then copies/blits to the swapchain image during submit/present.

### Extension points around canvas

- Stream sinks: additional backends can be attached through the stream/sink registry (`include/datoviz/stream.h`)
  to consume frame metadata or synchronize external consumers.
- Video capture: optional canvas-managed video sink in external-handle mode or CPU readback mode.


## Test architecture

- Unified test executable: `dvztest` (`testing/dvztest.c`)
- Module tests under: `src/<module>/tests/`
- Test framework: `testing/testing.h` and `testing/testing.cpp`
- Interactive canvas smoke app: `dvz_live_canvas` (`testing/dvz_live_canvas.c`)

The runner composes module test suites into one process and supports optional filtering by name/tag.


## Current status and direction

- Core active modules above are the stabilization focus for v0.4-dev.
- Graphics-path modules (`vk`, `vklite`, `canvas`, `stream`, `video`, `window`) are under active iteration.
- Non-activated higher-level systems remain scaffolded until explicitly brought online.

