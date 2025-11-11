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
│   ├── window_surface.c           # Surface creation, swapchain caps
│   ├── backend_glfw.c             # GLFW backend (window creation, events)
│   ├── backend_qt.c               # Qt event bridge (stub until implemented)
│   ├── backend_sdl.c              # Placeholder for SDL2 or headless backends
│   ├── tests/
│   │   └── test_window.c
│   └── CMakeLists.txt
├── src/canvas/
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

## 3. API Specification

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

typedef void (*DvzPointerCallback)(const DvzPointerEvent* event, void* user_data);
typedef void (*DvzKeyboardCallback)(const DvzKeyboardEvent* event, void* user_data);

typedef struct DvzInputRouter DvzInputRouter;

DVZ_EXPORT DvzInputRouter* dvz_input_router(void);
DVZ_EXPORT void dvz_input_router_destroy(DvzInputRouter* router);
DVZ_EXPORT void dvz_input_subscribe_pointer(
    DvzInputRouter* router, DvzPointerCallback cb, void* user_data);
DVZ_EXPORT void dvz_input_subscribe_keyboard(
    DvzInputRouter* router, DvzKeyboardCallback cb, void* user_data);
DVZ_EXPORT void dvz_input_emit_pointer(DvzInputRouter* router, const DvzPointerEvent* event);
DVZ_EXPORT void dvz_input_emit_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event);
```

- Routers support multiple subscribers. Internally they store a dynamic array of callbacks.
- Backends (GLFW/Qt/others) own a router per window.

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
} DvzWindowConfig;

typedef struct
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR color_space;
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
DVZ_EXPORT bool dvz_window_surface(DvzWindow* window, VkInstance instance, DvzWindowSurface* out);
DVZ_EXPORT DvzInputRouter* dvz_window_input(DvzWindow* window);
```

- The host stores a pointer to the active backend (GLFW for now). Registry logic (similar to frame sinks) lives in `window_host.c`.
- `backend_glfw.c` implements the vtable using GLFW calls:
  - `probe()` initializes GLFW (once).
  - `create()` spawns a window, stores `GLFWwindow*`, attaches callbacks that forward to the router.
  - `poll_events()` calls `glfwPollEvents()`.
  - `surface()` calls `glfwCreateWindowSurface`.
  - `input_router()` returns the router tied to the window.

### 3.3 Canvas Module (Header: `include/datoviz/canvas.h`)

```c
typedef struct DvzCanvas DvzCanvas;

typedef struct
{
    DvzWindow* window;
    DvzDevice* device;
    uint32_t width;
    uint32_t height;
    VkFormat color_format;
    bool enable_video_sink;
} DvzCanvasConfig;

typedef struct
{
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize size;
    int memory_fd;
    int wait_semaphore_fd;
} DvzCanvasFrame;

typedef void (*DvzCanvasDraw)(DvzCanvas* canvas, const DvzCanvasFrame* frame, void* user_data);

DVZ_EXPORT DvzCanvas* dvz_canvas_create(const DvzCanvasConfig* cfg);
DVZ_EXPORT void dvz_canvas_destroy(DvzCanvas* canvas);
DVZ_EXPORT void dvz_canvas_set_draw_callback(DvzCanvas* canvas, DvzCanvasDraw cb, void* user_data);
DVZ_EXPORT int dvz_canvas_frame(DvzCanvas* canvas);              // acquire + execute user draw
DVZ_EXPORT int dvz_canvas_submit(DvzCanvas* canvas);             // flush + dvz_frame_stream_submit
DVZ_EXPORT DvzInputRouter* dvz_canvas_input(DvzCanvas* canvas);  // proxy to window input
```

- Canvas owns a `DvzFrameStream` created with the window’s extent/format.
- During `dvz_canvas_create`, it attaches sinks:
  - Mandatory swapchain sink (see below) bound to the window surface.
  - Optional video sink if `enable_video_sink` is true or `canvas->cfg.video_sink_config` is provided.
- Canvas keeps a triple-buffer set of exportable images (color targets) and their timeline values. Each call to `dvz_canvas_frame()` rotates through them, letting the user record commands. `dvz_canvas_submit()` signals the timeline, updates `DvzFrameStreamResources`, and calls `dvz_frame_stream_submit()`.

### 3.4 Swapchain Sink (Header: `include/datoviz/window/swapchain_sink.h`)

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
- `start()` creates the swapchain (triple-buffered), image views, and pre-recorded command buffers that blit from the FrameStream image to the swapchain image.
- `submit()`:
  1. Acquire next swapchain image.
  2. Wait on the FrameStream timeline/semaphore from `DvzFrameStreamResources`.
  3. Submit the recorded blit command buffer, ensuring layout transitions from `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` to `PRESENT_SRC_KHR`.
  4. Present.
- `stop()/destroy()` tear down all swapchain resources.

## 4. Tests

1. **Input module (`src/input/tests/test_input.c`)**
   - Create a router, subscribe dummy callbacks, emit pointer/keyboard events, ensure callbacks run in FIFO order.
   - Validate modifier bitmasks and default router state.

2. **Window module (`src/window/tests/test_window.c`)**
   - Mock backend (no GLFW) that simulates create/poll/destroy cycle; ensures host attaches/detaches cleanly.
   - When GLFW is available (guard with `DVZ_WITH_GLFW`), spawn a hidden window, pump a few events, create/destroy a surface.

3. **Canvas module (`src/canvas/tests/test_canvas.c`)**
   - Use a mock window backend that provides a dummy surface and swapchain sink. Verify canvas creates a FrameStream, attaches the sink, and calls the draw callback.
   - Optional integration test (GLFW + headless swapchain) gated behind `DVZ_WITH_GLFW`.

4. **Swapchain sink (`src/window/tests/test_swapchain_sink.c`)**
   - When Vulkan validation layers & a real GPU are available, instantiate the sink with a headless surface (e.g., via GLFW with hidden window), render a simple color, ensure `vkQueuePresentKHR` succeeds.

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
   - Implement `dvz_swapchain_sink_backend()` in `src/window/swapchain_sink.c`.
   - Register it inside `src/stream/sink_registry.c` (alongside the video sink).
   - Extend `DvzFrameStreamResources` if needed (e.g., add `VkQueue queue; VkFence fence; VkSemaphore semaphore;` so sinks have the synchronization primitives required to wait/present).
   - Unit tests: stub queue/swapchain to verify state transitions; real GPU test optional (guarded).

4. **Canvas layer**
   - Implement `dvz_canvas_create()`:
     - Fetch window surface info.
     - Create exportable render targets (color images + memory).
     - Create FrameStream with default config (width/height/format).
     - Attach swapchain sink (and optional video sink).
   - Provide APIs to set draw callbacks, pump frames, submit to the stream, and access input routers.
   - Add tests with mock sinks verifying the canvas rotates frames and calls callbacks.

5. **Documentation & Integration**
   - Update `ARCHITECTURE.md` to mention the new modules and their responsibilities.
   - Expand `AGENTS.md` with guidance on using the input/window/canvas system.
   - Ensure `just build` / `just test` cover the new code (add `datoviz_input`, `datoviz_window`, `datoviz_canvas` to the root CMake graph and the test runner).

## 6. Notes & Constraints

- Keep platform-specific code isolated; input + canvas must not include GLFW headers directly.
- Aim for minimal dependencies: input depends only on `common`; window depends on `input` + `stream` (for swapchain sink) but not on higher-level modules.
- Provide sensible fallbacks when backend probes fail (e.g., headless builds without GLFW should still compile and run non-window tests).
- Before merging future backends (Qt, SDL2, headless, etc.), ensure the registry and data structures are flexible enough (void pointers, backend-specific data slots).

Following this plan keeps the new abstractions consistent with Datoviz’s modular architecture and gives future agents a clear roadmap for implementing the input/window/canvas stack. Update this file whenever major design decisions change.
