# Architecture (v0.4-dev)

This document describes the current Datoviz v0.4-dev architecture.

It replaces older v0.3-era descriptions and focuses on what is currently built, linked, and tested.


## Goals

- Single shared library target: `datoviz` (`libdatoviz.*`).
- Modular implementation through per-module object libraries.
- Clear separation between public headers (`include/datoviz`) and internal implementation (`src`).
- Unified test runner (`dvztest`) for all active modules.
- Incremental stabilization of the active graphics stack while bringing higher layers online through
  DRP2 rather than backend-specific shortcuts.


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
- `drp2`
- `scene`

These are added as subdirectories and linked into the shared target as object-library components.

Scaffolding modules (for example `color`, `wasm`, text/gui, and broader renderer/client layers) exist but are
not part of the active v0.4-dev link surface unless explicitly activated.


## Build topology

Datoviz uses:

- One shared target: `datoviz`
- One object library per module: `datoviz_<module>`
- One shared Vulkan entry-point provider: `datoviz_volk`
- Optional layer shared targets for narrower linking, including `datoviz_core`, `datoviz_vk`, and
  `datoviz_canvas`

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

The active higher-level path is:

1. `scene` owns user-facing scene objects, capability snapshots, diagnostics, and frame plans.
2. `scene` emits `DvzDrp2CommandStream` objects.
3. `drp2` validates and executes command streams.
4. The native `drp2` runtime maps commands to `vklite`.
5. When rendering through canvas, the runtime records into a borrowed `DvzStreamFrame` target supplied by
   the canvas draw callback.


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
- Focused test executables include `dvztest_core`, `dvztest_drp2`, `dvztest_scene`, `dvztest_vk`,
  `dvztest_canvas`, and `dvztest_integration` when their target dependencies are enabled.
- Module tests under: `src/<module>/tests/`
- Test framework: `testing/testing.h` and `testing/testing.cpp`
- Interactive canvas smoke app: `dvz_live_canvas` (`testing/dvz_live_canvas.c`)

The runner composes module test suites into one process and supports optional filtering by name/tag.


## Active examples

The C examples intentionally cover different layers:

- `examples/c/hello_point.c`: minimal high-level scene + app path.
- `examples/c/hello_scatter.c`: point visual with non-trivial per-item arrays.
- `examples/c/raw_triangle.c`: vklite commands recorded into a `DvzCanvas` frame callback.
- `examples/c/raw_triangle_drp2.c`: hand-written DRP2 command stream executed by the native runtime.


## Planned Python binding architecture

The Python API will be layered above `libdatoviz` in three tiers:

```
┌─────────────────────────────────┐
│  Python sugar layer             │  datoviz/*.py        (pure Python)
│  ergonomics, NumPy, defaults    │
├─────────────────────────────────┤
│  Generated ctypes binding       │  datoviz/_ctypes.py  (auto-generated)
│  1:1 with C API, no compilation │
├─────────────────────────────────┤
│  C core                         │  libdatoviz.so
│  all logic lives here           │
└─────────────────────────────────┘
```

### v0.3 binding strategy (carried forward to v0.4)

v0.3 used a code-generation pipeline that is retained for v0.4:

1. `tools/parse_headers.py` — pyparsing-based parser that reads all `include/datoviz/**/*.h`
   headers and emits a `build/headers.json` description of defines, enums, structs, and all
   `DVZ_EXPORT`-marked functions with their doxygen docstrings.

2. `tools/build_ctypes.py` — reads `headers.json` and generates `datoviz/_ctypes.py`, a pure
   Python ctypes binding file. It maps C types to ctypes and NumPy dtypes, emits `argtypes` and
   `restype` for every exported function, and converts doxygen docstrings to NumPy-style
   docstrings. The output requires no compilation — it loads `libdatoviz.so` at runtime via
   `ctypes.CDLL()`.

3. `datoviz/_ctypes.py` — the generated file. Never edited by hand; always regenerated from
   headers. In v0.3 this file was ~15 000 lines covering the full public API.

The sugar layer (`datoviz/*.py`) sits above `_ctypes.py` and adds Python ergonomics: keyword
arguments, NumPy array coercion, context managers, inline colormap shortcuts. It contains no
scene logic.

### v0.4 adaptation

The same three-tier strategy applies in v0.4. The generator scripts may need updates to handle
v0.4 header conventions (new type names, new struct patterns, updated `DVZ_EXPORT` usage), but
the pipeline architecture stays the same. The C API must remain an FFI-friendly target: opaque
handles, descriptor structs, explicit lifecycle, no raw function pointers in public structs.

See `spec/scene/IMPLEMENTATION_BRIDGE.md` for the full design rationale.


## Current status and direction

- Core and graphics-path modules are now the foundation rather than the only active work.
- `drp2` and `scene` are active default-build modules with a first vertical slice in place.
- Near-term development should harden the current point-based scene path — now including retained
  point rendering across frames, repeated partial updates, multi-panel figures, and per-panel
  runtime viewport/scissor handling — add the next minimal visual families (`primitive` for
  topology-driven rendering, then image/texture), keep examples current, and preserve the
  runtime boundary:
  scene emits DRP2; DRP2 runtime executes through vklite/canvas; scene does not own backend lifecycles.
