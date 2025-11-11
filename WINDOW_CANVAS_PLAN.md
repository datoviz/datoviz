<!--
This document is meant to guide future Codex agents when implementing the upcoming input/window/canvas stack.
Keep it under version control and update it as the design evolves.
-->

# 🚧 Input/Window/Canvas Architecture Plan (Datoviz v0.4-dev)

## 1. Goals

- Provide backend-agnostic input/event abstractions that work with GLFW, Qt, or other hosts.
- Introduce a window module capable of hosting multiple backends (GLFW first, Qt later) and exposing Vulkan surfaces.
- Layer a canvas module on top of the new window + FrameStream infrastructure so rendering can target windows, video encoders, or other sinks simultaneously.
- Keep existing video-specific code inside `src/video/` while all generic presentation logic lives under the new `stream` module added earlier.

## 1.1 Delivery Phases

- **Phase 1 (implement now)**
  - Input router with pointer, keyboard, and window-resize/scale events plus timeline semaphore export/import.
  - Window host, GLFW backend, Qt stub (non-owning event loop), and surface helpers.
  - Canvas module in charge of surface/swapchain creation, FrameStream wiring, swapchain sink, and optional video sink hookup.
  - Basic timing (CPU timestamps) and single-view rendering (only `views[0]` populated even though the structs allow more).
- **Phase 2+ (future work)**
  - Additional backends (Qt event-loop integration, SDL2, headless, VR), multi-view rendering, high-precision display timing, metadata payloads, VR sinks, etc.
  - These features are described below for architectural readiness but must not be implemented until explicitly scheduled.

## 2. Module Overview & File Layout

```
datoviz/
├── include/datoviz/
│   ├── input/
│   │   ├── input.h                # Aggregator
│   │   ├── pointer.h              # DvzPointerEvent, button/modifier enums
│   │   ├── keyboard.h             # DvzKeyboardEvent, scancode enums
│   │   └── router.h               # DvzInputRouter API (subscribe/emit)
│   ├── window.h                   # Public DvzWindow/DvzWindowHost facade
│   ├── window/
│   │   ├── types.h                # DvzWindowSurface, DvzWindowConfig
│   │   ├── backend.h              # Backend vtable (probe/create/poll/destroy)
│   │   └── glfw.h (later)         # Optional helper for GLFW-specific knobs
│   └── canvas.h                   # Canvas API layered on FrameStreams
├── src/input/
│   ├── input_router.c             # Router implementation
│   ├── pointer.c / keyboard.c     # Helpers (normalize coordinates, etc.)
│   ├── tests/
│   │   └── test_input.c
│   └── CMakeLists.txt
├── src/window/
│   ├── window_host.c              # Backend registry + shared helpers
│   ├── backend_glfw.c             # GLFW backend (window creation, events)
│   ├── backend_qt.c               # Qt event bridge (stub until implemented)
│   ├── backend_sdl.c              # Placeholder for SDL2 or headless backends
│   ├── tests/
│   │   └── test_window.c
│   └── CMakeLists.txt
├── src/canvas/
│   ├── window_surface.c           # Surface helpers shared across canvases
│   ├── swapchain_sink.c           # Frame sink that presents to VkSurfaceKHR
│   ├── canvas.c                   # Canvas public API, draw loop
│   ├── canvas_internal.h          # Internal structs shared with tests
│   ├── canvas_stream.c            # FrameStream wiring + sink attachment
│   ├── tests/
│   │   └── test_canvas.c
│   └── CMakeLists.txt
└── WINDOW_CANVAS_PLAN.md          # This document
```

- `src/window/backend_glfw.c` will depend on GLFW; wrap usage behind CMake option `DVZ_WITH_GLFW`. Provide a stub backend that returns `false` from `probe()` when GLFW is unavailable so the rest of the system can still compile without GLFW.
- Qt backend will arrive later; for now document the expected API in `backend_qt.c` and leave it returning “not implemented” until the integration layer exists.
- Additional backends we want to support long-term include SDL2, direct Wayland/XCB hosts, Win32-specific windows, headless surfaces, and browser/WebGPU bridges. Keep the registry flexible so these can be plugged in without touching higher layers.
- Surface creation, swapchain management, FrameStream wiring, and presentation code live under `src/canvas/`; the window module exposes enough information (surfaces, scale, input) without taking a dependency on Vulkan.

## 3. API Specification

### Event Loop Strategy

- Poll-driven backends (GLFW, SDL) rely on `dvz_window_host_poll(host)`; applications call it once per frame. The host invokes the backend’s `poll_events()` (e.g., `glfwPollEvents`) and dispatches queued events to every window’s router.
- External-loop backends (Qt) don’t want Datoviz to own the loop. Provide `dvz_window_host_request_frame(host)` (or backend-specific hooks) so Qt can request a render; the backend forwards Qt events directly without `poll()`.
- Because canvases/renderers can run under either model, they should never assume control of the loop; they simply react to events and schedule work using the host API.

### 3.1 Input (Header: `include/datoviz/input/router.h` etc.)

```c
typedef enum
{
    DVZ_BUTTON_NONE = 0,
    DVZ_BUTTON_LEFT = 1,
    DVZ_BUTTON_RIGHT = 2,
    DVZ_BUTTON_MIDDLE = 4,
} DvzPointerButton;

typedef enum
{
    DVZ_KEY_UNKNOWN = 0,
    DVZ_KEY_ESCAPE,
    DVZ_KEY_SPACE,
    /* ... extend as needed ... */
} DvzKey;

typedef struct
{
    double x;                // window-relative
    double y;
    double dx;
    double dy;
    double scroll_x;
    double scroll_y;
    DvzPointerButton buttons;
    uint32_t modifiers;      // ALT/CTRL/SHIFT flags
    uint64_t timestamp_ns;
} DvzPointerEvent;

typedef struct
{
    DvzKey key;
    uint32_t scancode;
    bool press;              // true=press, false=release
    uint32_t modifiers;
    uint64_t timestamp_ns;
} DvzKeyboardEvent;

typedef enum
{
    DVZ_INPUT_POINTER,
    DVZ_INPUT_KEYBOARD,
    DVZ_INPUT_WINDOW_RESIZE,
    DVZ_INPUT_WINDOW_SCALE,
} DvzInputEventType;

typedef struct
{
    DvzInputEventType type;
    union
    {
        DvzPointerEvent pointer;
        DvzKeyboardEvent keyboard;
        struct
        {
            uint32_t width;
            uint32_t height;
        } resize;
        struct
        {
            float scale_x;
            float scale_y;
        } scale;
    } data;
} DvzInputEvent;

typedef void (*DvzPointerCallback)(const DvzPointerEvent* event, void* user_data);
typedef void (*DvzKeyboardCallback)(const DvzKeyboardEvent* event, void* user_data);
typedef void (*DvzInputEventCallback)(const DvzInputEvent* event, void* user_data);

typedef struct DvzInputRouter DvzInputRouter;

DVZ_EXPORT DvzInputRouter* dvz_input_router(void);
DVZ_EXPORT void dvz_input_router_destroy(DvzInputRouter* router);
DVZ_EXPORT void dvz_input_subscribe_pointer(
    DvzInputRouter* router, DvzPointerCallback cb, void* user_data);
DVZ_EXPORT void dvz_input_subscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback cb, void* user_data);
DVZ_EXPORT void dvz_input_subscribe_event(
    DvzInputRouter* router, DvzInputEventCallback cb, void* user_data);
DVZ_EXPORT void dvz_input_emit_pointer(DvzInputRouter* router, const DvzPointerEvent* event);
DVZ_EXPORT void dvz_input_emit_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event);
DVZ_EXPORT void dvz_input_emit_event(DvzInputRouter* router, const DvzInputEvent* event);
```

- Routers support multiple subscribers. Internally they store a dynamic array of callbacks.
- Backends (GLFW/Qt/others) own a router per window.
- Pointer, keyboard, resize, and scale events are required in Phase 1; other types can piggyback on the tagged `DvzInputEvent` union later. Document whether callbacks run on the backend’s thread (GLFW main thread, Qt UI thread) so subscribers know if they must be thread-safe.

### 3.2 Window Module (Header: `include/datoviz/window.h`)

```c
typedef struct DvzWindowHost DvzWindowHost;
typedef struct DvzWindow DvzWindow;
typedef struct DvzWindowBackend DvzWindowBackend;

typedef struct
{
    uint32_t width;
    uint32_t height;
    const char* title;
    bool resizable;
    float user_scale;     // optional multiplier applied on top of OS scale
} DvzWindowConfig;

typedef struct
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR color_space;
    float scale_x;           // device pixel ratio vs logical width
    float scale_y;
} DvzWindowSurface;

struct DvzWindowBackend
{
    const char* name;
    bool (*probe)(void);
    DvzWindow* (*create)(DvzWindowHost* host, const DvzWindowConfig* cfg);
    void (*destroy)(DvzWindow* window);
    void (*poll_events)(DvzWindowHost* host);
    bool (*surface)(DvzWindow* window, VkInstance instance, DvzWindowSurface* out);
    DvzInputRouter* (*input_router)(DvzWindow* window);
};

DVZ_EXPORT DvzWindowHost* dvz_window_host_create(void);
DVZ_EXPORT void dvz_window_host_destroy(DvzWindowHost* host);
DVZ_EXPORT int dvz_window_host_use_backend(DvzWindowHost* host, const char* name);
DVZ_EXPORT DvzWindow* dvz_window_create(DvzWindowHost* host, const DvzWindowConfig* cfg);
DVZ_EXPORT void dvz_window_destroy(DvzWindow* window);
DVZ_EXPORT void dvz_window_host_poll(DvzWindowHost* host);
DVZ_EXPORT void dvz_window_host_request_frame(DvzWindowHost* host);
DVZ_EXPORT bool dvz_window_surface(DvzWindow* window, VkInstance instance, DvzWindowSurface* out);
DVZ_EXPORT DvzInputRouter* dvz_window_input(DvzWindow* window);
DVZ_EXPORT void dvz_window_get_scale(DvzWindow* window, float* scale_x, float* scale_y);
DVZ_EXPORT void dvz_window_get_size(DvzWindow* window, uint32_t* width, uint32_t* height);
```

- The host stores a pointer to the active backend (GLFW for now). Registry logic (similar to frame sinks) lives in `window_host.c`.
- `backend_glfw.c` implements the vtable using GLFW calls:
  - `probe()` initializes GLFW (once).
  - `create()` spawns a window, stores `GLFWwindow*`, attaches callbacks that forward to the router.
  - `poll_events()` calls `glfwPollEvents()`.
  - `surface()` calls `glfwCreateWindowSurface`.
  - `input_router()` returns the router tied to the window.

#### Qt backend and VkInstance ownership

- Qt support must keep Datoviz in charge of creating the Vulkan instance/device so we can share one `VkInstance` (and associated `DvzDevice`) across GLFW, Qt, headless, etc. Backends never allocate their own instances; instead, `dvz_window_surface()` receives the already-created `VkInstance` from the caller and uses it to build the surface.
- The Qt backend therefore wraps the passed-in `VkInstance` inside a lightweight `QVulkanInstance` by calling `QVulkanInstance::setVkInstance(instance, vkGetInstanceProcAddr)`. That object is then attached to the `QWindow` via `QWindow::setVulkanInstance(&qt_inst)` (a.k.a. `setVulkanInstance()` in Qt shorthand). Because the `QVulkanInstance` simply references Datoviz’s instance, Qt will not try to create or destroy Vulkan state on its own.
- When the application asks for a surface, the backend calls `QVulkanInstance::surfaceForWindow(window)` (which internally uses the already-set Vulkan instance) and copies the resulting `VkSurfaceKHR` plus DPI/scale information into `DvzWindowSurface`. This keeps lifetime ownership consistent: Datoviz still tears down the `VkInstance`/`VkDevice`, while Qt merely hosts the native window and forwards events.
- Qt’s event loop is still externally driven (see §3.1). The backend only bridges input callbacks and surface creation; all device/queue selection stays inside `src/vk/` and `src/canvas/`, so users who embed Datoviz in a Qt application retain full control over which physical device, extensions, and validation layers are enabled.

### 3.3 Canvas Module (Header: `include/datoviz/canvas.h`)

```c
typedef struct DvzCanvas DvzCanvas;

typedef struct
{
    DvzWindow* window;
    DvzDevice* device;
    VkFormat color_format;
    bool enable_video_sink;
    size_t timing_history;     // number of frame timing entries to retain (0 = default ring size)
} DvzCanvasConfig;

typedef struct
{
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    VkDeviceSize size;
    int memory_fd;
    int wait_semaphore_fd;
} DvzFrameView;

typedef struct
{
    uint32_t view_count;              // Phase 1 only requires view_count=1, but the array is future-proof.
    DvzFrameView views[4];            // left/right/multiview; keep upper-bound small for now.
    VkSemaphore timeline_semaphore;   // Export/import mandatory in Phase 1.
    uint64_t wait_value;
    VkSemaphore binary_semaphore;
    VkFence render_fence;
    VkQueue render_queue;
    const void* metadata;             // Optional per-frame data (e.g., VR poses, timing payloads) - future work.
    size_t metadata_size;
} DvzFrameStreamResources;

typedef struct
{
    uint64_t frame_id;
    double cpu_submit_us;             // host clock timestamp when submit happened
    double gpu_complete_us;           // derived from timeline semaphore / calibrated timestamps
    double present_start_us;          // when vkQueuePresentKHR was queued
    double present_done_us;           // actual presentation time (if VK_GOOGLE_display_timing available)
} DvzFrameTiming;

typedef void (*DvzCanvasDraw)(DvzCanvas* canvas, const DvzFrameStreamResources* frame, void* user_data);

DVZ_EXPORT DvzCanvas* dvz_canvas_create(const DvzCanvasConfig* cfg);
DVZ_EXPORT void dvz_canvas_destroy(DvzCanvas* canvas);
DVZ_EXPORT void dvz_canvas_set_draw_callback(DvzCanvas* canvas, DvzCanvasDraw cb, void* user_data);
DVZ_EXPORT int dvz_canvas_frame(DvzCanvas* canvas);              // acquire + execute user draw
DVZ_EXPORT int dvz_canvas_submit(DvzCanvas* canvas);             // flush + dvz_frame_stream_submit
DVZ_EXPORT const DvzFrameTiming*
dvz_canvas_timings(DvzCanvas* canvas, size_t* count);            // returns pointer to ring buffer
DVZ_EXPORT DvzInputRouter* dvz_canvas_input(DvzCanvas* canvas);  // proxy to window input
```

- `DvzCanvasConfig` intentionally omits width/height; canvases derive physical extent from their window. If (later) we support headless/offscreen canvases, those fields can be reintroduced for the window-less path only.
- Canvas owns a `DvzFrameStream` created with the window’s physical extent (logical width/height multiplied by per-window DPI scale factors). Update `include/datoviz/stream/frame_stream.h` so the official `DvzFrameStreamResources` definition matches the struct above; all sinks/tests must consume the same definition.
- During `dvz_canvas_create`, it attaches sinks:
  - Mandatory swapchain sink (see below) bound to the window surface.
- Optional video sink if `enable_video_sink` is true or `canvas->cfg.video_sink_config` is provided.
- Canvas keeps a pool of exportable render targets sized according to the swapchain image count (usually `surface_caps.minImageCount + 1`, clamped to `maxImageCount`) and at physical resolution (logical × scale). Each call to `dvz_canvas_frame()` rotates through the available frames/views, letting the user record commands. `dvz_canvas_submit()` signals the timeline, updates `DvzFrameStreamResources` (including view metadata), and calls `dvz_frame_stream_submit()`.
- Timing roadmap: Phase 1 records CPU submit time plus the timeline semaphore value. Future work will add GPU completion timestamps (`vkGetCalibratedTimestampsEXT`) and presentation timestamps (`VK_GOOGLE_display_timing`) so experiments can correlate GPU presentation with external instrumentation at microsecond resolution.
- `DvzFrameTiming` already exposes the future-ready fields so later work can populate them without breaking ABI; leave them zeroed until the advanced timing path lands.
- Multiple canvases/windows: `DvzWindowHost` can create many `DvzWindow` objects, each with its own canvas. All canvases may share the same `DvzDevice` (multiple swapchains per device) or use separate devices. The window host’s event loop (`dvz_window_host_poll`) propagates GLFW/Qt events to every window’s input router; each canvas then renders and submits independently. Backend loops that are externally driven (Qt) simply forward events to the matching router, keeping canvases isolated from one another.
- DPI/scaling: window backends report per-monitor scale factors (e.g., via `glfwGetWindowContentScale` on macOS). These values live in `DvzWindowSurface.scale_x/scale_y` and should update when the window moves between monitors or the OS scale changes. Canvas uses the scale to size swapchains/render targets, while input routers pre-scale pointer coordinates so logical units remain consistent across displays. Support an optional user override factor in `DvzWindowConfig` to multiply the OS scale (e.g., forced 150% UI). When scale changes, the backend emits a resize/scale event so canvases can rebuild swapchains.
- Resizing: window backends emit resize callbacks (GLFW `glfwSetWindowSizeCallback`, Qt `resizeEvent`) and update internal logical + physical extents. These events must propagate via the router so canvases know to recreate FrameStream render targets and swapchains. While a resize is pending, canvas rendering/submission should pause until new resources (swapchain, exportable images) are ready, then resume using the updated sizes/scales.

### 3.4 Swapchain Sink (Header: `include/datoviz/canvas/swapchain_sink.h`)

```c
typedef struct
{
    DvzWindow* window;
    VkDevice device;
    VkQueue graphics_queue;
    uint32_t queue_family;
    VkFormat format;
    VkExtent2D extent;
} DvzSwapchainSinkConfig;

DVZ_EXPORT const DvzFrameSinkBackend* dvz_swapchain_sink_backend(void);
DVZ_EXPORT DvzSwapchainSinkConfig dvz_swapchain_sink_default_config(DvzWindow* window, DvzDevice* device);
```

- Backend state holds: `VkSwapchainKHR`, per-image command buffers, acquire/present semaphores, and fences.
- `start()` creates the swapchain using the optimal number of images derived from `VkSurfaceCapabilitiesKHR` (rather than hardcoding triple buffering), sets up image views, and records the blit/composite command buffers that move FrameStream images into swapchain images. Queue selection honors `vkGetPhysicalDeviceSurfaceSupportKHR`; if graphics and present families differ, two queues plus cross-queue semaphores are used.
- `submit()`:
  1. Acquire next swapchain image with per-image semaphore/fence.
  2. Wait on the FrameStream timeline semaphore/Fd value exported by the renderer.
  3. Submit the recorded blit command buffer on the graphics queue. If a dedicated present queue is required, signal an extra semaphore that the present queue then waits on.
  4. Call `vkQueuePresentKHR` on the present-capable queue.
- `stop()/destroy()` tear down all swapchain resources.
- Error handling: if `vkAcquireNextImageKHR` or `vkQueuePresentKHR` returns `VK_ERROR_OUT_OF_DATE_KHR` or `VK_SUBOPTIMAL_KHR`, flag the sink to rebuild its swapchain/images/views before the next submit (same path triggered by resize events).

## 4. Tests

1. **Input module (`src/input/tests/test_input.c`)**
   - Create a router, subscribe dummy callbacks (pointer, keyboard, generic), emit pointer/keyboard/resize/scale events, ensure callbacks run in FIFO order.
   - Validate modifier bitmasks and default router state.

2. **Window module (`src/window/tests/test_window.c`)**
    - Mock backend (no GLFW) that simulates create/poll/destroy cycle; ensures host attaches/detaches cleanly.
    - When GLFW is available (guard with `DVZ_WITH_GLFW`), spawn a hidden window, pump a few events, create/destroy a surface.
    - Add DPI tests: feed fake scale values and ensure `dvz_window_surface()` returns the expected scale, and that scale-change events trigger callbacks.
    - Add resize tests: simulate backend resize events and verify notifications propagate to canvases/sinks.

3. **Canvas module (`src/canvas/tests/test_canvas.c`)**
    - Use a mock window backend that provides a dummy surface and swapchain sink. Verify canvas creates a FrameStream, attaches the sink, and calls the draw callback.
    - Optional integration test (GLFW + headless swapchain) gated behind `DVZ_WITH_GLFW`.
    - Add resize scenarios: trigger a mock resize, ensure the canvas rebuilds its FrameStream resources/sink configuration before rendering resumes.
    - Timing scenarios: mock the CPU clock and timeline semaphore query to validate that `dvz_canvas_timings()` reports consistent values and enforces the configured history length. GPU/display timestamps remain future work.
    - Timeline semaphores: ensure the canvas exports a timeline semaphore FD/handle and that the mock swapchain sink waits on it before presenting.

4. **Swapchain sink (`src/canvas/tests/test_swapchain_sink.c`)**
   - When Vulkan validation layers & a real GPU are available, instantiate the sink with a headless surface (e.g., via GLFW with hidden window), render a simple color, ensure `vkQueuePresentKHR` succeeds.
   - Simulate `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` (e.g., by resizing mid-test) to verify the sink marks itself for swapchain recreation before the next submit.

All new tests should be registered in `testing/dvztest.c` and added to the unified runner.

## 5. Implementation Plan

1. **Input module bring-up**
   - Add headers under `include/datoviz/input/`.
   - Implement router in `src/input/input_router.c` with dynamic arrays for subscribers.
   - Register module in `src/CMakeLists.txt` (as OBJECT library `datoviz_input`) and link it into `datoviz`.
   - Write unit tests (`test_input.c`) and add to `testing/dvztest.c`.

2. **Window core + registry**
   - Define `DvzWindowBackend` interface (`include/datoviz/window/backend.h`) and host helpers (`src/window/window_host.c`).
   - Create registry similar to `dvz_frame_sink_backend_pick`, with built-in entries for GLFW (if compiled) and a stub backend.
   - Implement the GLFW backend:
     - Manage GLFW init/term reference counts.
     - Store `GLFWwindow*` inside `DvzWindow`.
     - Hook GLFW callbacks to emit pointer/keyboard events via the router.
     - Create Vulkan surface on demand.
   - Add `DVZ_WITH_GLFW` option + CMake logic to locate GLFW and define/skip the backend accordingly.
- Provide tests that use a fake backend (no GLFW) to validate the host/registry logic.

3. **Swapchain sink**
   - Implement `dvz_swapchain_sink_backend()` in `src/canvas/swapchain_sink.c`.
   - Register it inside `src/stream/sink_registry.c` (alongside the video sink).
   - Extend `DvzFrameStreamResources` (in `include/datoviz/stream/frame_stream.h`) with the view/timeline fields defined above so every sink shares the same struct:
     - Timeline semaphore handle + wait value (exported via `vkGetSemaphoreFdKHR` on Linux) so sinks can wait before consuming frames.
     - Optional fallback binary semaphore/fence for platforms without timeline support.
   - Store queue handles/family indices in the swapchain sink config so sinks can handle graphics vs present queues cleanly.
   - Unit tests: stub queue/swapchain to verify state transitions; real GPU test optional (guarded).

4. **Canvas layer**
   - Implement `dvz_canvas_create()`:
       - Fetch window surface info.
       - Create exportable render targets (color images + memory) sized using physical resolution (logical extent × scale) and rebuild them when scale changes.
     - Create a FrameStream whose width/height come from the window surface; off-screen canvases (if/when introduced) may override these values explicitly.
     - Attach swapchain sink (and optional video sink).
   - Provide APIs to set draw callbacks, pump frames, submit to the stream, and access input routers.
   - Record per-frame CPU timestamps (Phase 1) and capture the timeline semaphore wait values; leave hooks for future GPU/display timing.
   - Add tests with mock sinks verifying the canvas rotates frames and calls callbacks.

5. **Documentation & Integration**
   - Update `ARCHITECTURE.md` to mention the new modules and their responsibilities.
   - Expand `AGENTS.md` with guidance on using the input/window/canvas system.
   - Ensure `just build` / `just test` cover the new code (add `datoviz_input`, `datoviz_window`, `datoviz_canvas` to the root CMake graph and the test runner).

## 6. Notes & Constraints

- Keep platform-specific code isolated; input + canvas must not include GLFW headers directly.
- Aim for minimal dependencies: input depends only on `common`; window depends on `input` (no Vulkan); canvas depends on `window` + `stream` + `vk` (since it owns surfaces and swapchains).
- Provide sensible fallbacks when backend probes fail (e.g., headless builds without GLFW should still compile and run non-window tests).
- Before merging future backends (Qt, SDL2, headless, etc.), ensure the registry and data structures are flexible enough (void pointers, backend-specific data slots).
- Synchronization expectations:
  - Renderers must signal a timeline semaphore per frame; FrameStream passes the handle + value to every sink.
  - Export/import (e.g., `vkGetSemaphoreFdKHR`) is required in Phase 1 so swapchain/video sinks can wait on renderer work from other processes or queues.
  - Video sink imports the semaphore (or waits on a CPU-side fence) before reading the image, then runs independently (CUDA/NVENC or CPU encoding).
  - Swapchain sink waits on the same timeline value, performs the copy on its graphics queue, then uses binary semaphores to bridge to the present queue when needed.
  - Multiple canvases/windows each maintain their own timeline/semaphore sets, so cross-window synchronization is unnecessary unless the application explicitly shares resources.
- DPI & scaling expectations:
  - Backends report per-window scale factors; canvases store them and rebuild swapchains/render targets when scale changes.
  - Input routers emit pointer positions in logical units (after dividing by scale) so camera/navigation code works uniformly on Retina, 4K, or standard monitors.
  - Provide optional user overrides (e.g., `DvzWindowConfig.user_scale`) to multiply the OS-reported scale for custom UI sizing.
- Resizing expectations:
  - Window backends notify canvases when logical size or DPI scale changes; canvases temporarily halt rendering, destroy old swapchains/render targets, and recreate them with the new extents.
  - Swapchain sinks must handle `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` by recreating their swapchains on demand.
  - Tests should cover resize propagation to ensure no rendering occurs against destroyed swapchains.
- Event-loop expectations:
  - Poll-based backends implement `poll_events()` so `dvz_window_host_poll()` drives them once per frame.
  - Backends living inside foreign loops expose `dvz_window_host_request_frame()` (or similar) so Datoviz can be driven externally without owning the loop.
  - **Python/IPython/Jupyter note (future)**: embed integrations should hook `dvz_window_host_poll()` into the interpreter loop (e.g., `PyOS_InputHook`, IPython input hook manager, asyncio callbacks used by Jupyter). No C-level work is needed now, but ensure the host API stays non-blocking and re-entrant.
- Input extensibility:
  - Pointer, keyboard, resize, and scale events ship in Phase 1, but the router/subscription API must remain open to touch, pen, gesture, controller, and VR pose events. Extend the tagged `DvzInputEvent` union rather than adding ad-hoc pathways per backend.
- VR readiness:
  - The multi-view `DvzFrameStreamResources` (view_count + metadata) allows future VR sinks (OpenXR/SteamVR) to consume per-eye images and pose data without breaking existing sinks.
  - VR backends register through the same window backend interface; instead of returning a `VkSurfaceKHR`, they expose XR swapchain descriptors via metadata and pair with a VR frame sink that submits frames to the XR runtime.
  - VR inputs (headset pose, controllers) route through `DvzInputRouter` using the same extensibility hooks, so no new event systems are needed.
- Precision timing:
  - Phase 1: record CPU timestamps and the timeline semaphore values that reflect GPU completion order.
  - Future: add calibrated GPU timestamps and `VK_GOOGLE_display_timing` support so swapchain sinks can expose actual presentation times. All timing history should remain accessible through `dvz_canvas_timings()`.

## 7. Future VR Integration (Future Work Only)

- **Window backend**: add an OpenXR (or similar) backend that plugs into the existing `DvzWindowBackend` registry. Instead of returning a `VkSurfaceKHR`, it publishes XR swapchain descriptors plus per-eye view/projection matrices via the metadata blob attached to `DvzFrameStreamResources`.
- **FrameStream usage**: renderers write into multiview exportable images (one per eye). `view_count`/`views[]` already expose these images to sinks; the VR sink simply consumes view 0/1 (left/right) and submits them to the XR runtime via `xrEndFrame`.
- **Sinks**: implement `dvz_frame_sink_vr` that waits on the same timeline semaphore, performs any necessary layout transitions, and calls the XR runtime’s `xrEndFrame`/`xrWaitFrame`. Because it conforms to the generic sink interface, it can coexist with video encoders or debug windows if needed.
- **Input**: headset pose and controllers emit events via `DvzInputRouter`, reusing the extensible event system described earlier (e.g., new `DvzPoseEvent`, `DvzControllerEvent` structs).
- **Timing**: VR runtimes often supply predicted/presented timestamps. Store them in the frame metadata so scientific users can correlate headset presentation with external lab equipment using the same `dvz_canvas_timings()` API.

Following this phased plan keeps the new abstractions consistent with Datoviz’s modular architecture and gives future agents a clear roadmap for implementing the input/window/canvas stack. Update this file whenever major design decisions change.
