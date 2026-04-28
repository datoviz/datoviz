# Window Module Plan (Phase 1)

## Why this module?

- Hosts the GLFW/Qt/headless backends that create windows, expose Vulkan surfaces, and dispatch events through the input router.
- Bridges the event-loop expectations listed in the architecture plan: poll-driven (`dvz_window_host_poll`) or request-frame/backends for non-owning loops (Qt).
- Provides the `DvzWindowSurface` and scale information canvases consume when creating swapchains and attaching streams.

## Scope & layout

```
include/datoviz/
├── window.h               # Facade for DvzWindow/DvzWindowHost
├── window/
│   ├── types.h            # DvzWindowSurface, DvzWindowConfig
│   ├── backend.h          # Backend vtable (probe/create/poll/destroy)
│   └── glfw.h (future)    # GLFW-specific knobs
src/window/
├── window_host.c          # Registry & host helpers, integrates DvzInputRouter
├── backend_glfw.c         # GLFW window creation, surface creation, callbacks
├── backend_qt.c           # Qt stub (event bridge, non-owning loop hooks)
├── backend_headless.c     # Minimal backend for tests/video (no GLFW dependency)
├── tests/
│   └── test_window.c      # Host/poll behavior, backend selection
└── CMakeLists.txt         # OBJECT library + includes
```

Keep `${PROJECT_SOURCE_DIR}/src/common` in the include path so `_macros.h`/helpers stay available; the object library should expose the backend types to canvases but hide implementation details (use `_dvz_` prefixes for helpers).

## Event loop & API snapshot

- **Event loop strategy:** Poll-driven backends call `dvz_window_host_poll(host)` once per frame; the host forwards to each backend’s `poll_events()` and pushes events into the per-window router. External-loop backends (Qt) instead rely on `dvz_window_host_request_frame()` or backend-specific hooks so Datoviz never owns their loop.
- **Input router integration:** Every window owns a `DvzInputRouter`. The backend emits pointer/keyboard/resize/scale events via `dvz_input_emit_*` and canvases subscribe through `dvz_canvas_input()`.
- **Public APIs (see `include/datoviz/window.h`):**

```c
typedef struct DvzWindowHost DvzWindowHost;
typedef struct DvzWindow DvzWindow;

typedef struct
{
    uint32_t width;
    uint32_t height;
    const char* title;
    bool resizable;
    float user_scale;
} DvzWindowConfig;

typedef struct
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR color_space;
    float scale_x;
    float scale_y;
} DvzWindowSurface;
```

## Backend-specific notes

- **GLFW backend:** Guarded by `DVZ_WITH_GLFW` in CMake. Provide a probe that returns `false` when GLFW is unavailable so the rest of libdatoviz can compile without it. Map GLFW callbacks into `DvzPointerEvent`/`DvzKeyboardEvent` and forward handles to Vulkan surface creation.
- **Qt backend (stub for now):** Document expected hooks in `backend_qt.c` and return “not implemented” for `probe`/`create` until its event loop integration exists.
- **Headless backend:** Minimal surface/no surface version for tests/video injection; avoids GLFW dependency and provides events for contexts like automated testing. Useful for `dvz_stream_sink_video()` verification without an actual window.
- **Surface helpers:** Provide cross-module helpers (`window_surface.c`) so canvases can reuse scale/extent data regardless of backend.

## Step-by-step instructions for agents

1. **Implement the host registry.** Make `DvzWindowHost` manage a list of registered backends and windows. `dvz_window_host_poll()` should iterate windows, invoke backend `poll_events()`, and route events into `DvzInputRouter`. Support `dvz_window_host_request_frame()` for external-loop backends already in the plan.
2. **Wire up GLFW backend.** Under `DVZ_WITH_GLFW`, create/destroy windows, create Vulkan surfaces, and translate GLFW events into the pointer/keyboard structs defined in the Input plan. When GLFW is missing, `probe()` returns `false` so builds do not fail.
3. **Document and stub Qt/headless backends.** Qt returns “not implemented” until later work but should have the expected API so future agents can plug it in. Headless backend should compile even without GLFW and expose dummy surfaces for canvas tests.
4. **Expose surface + input to canvases.** Each `DvzWindow` should return a `DvzWindowSurface` with extent/format info and expose its router through `dvz_canvas_input(canvas)`. Store scale factors to convert logical sizes into physical sizes for canvases.
5. **Test the host.** Cover backend selection, poll behavior, resize/scale propagation, and headless fallback in `src/window/tests/test_window.c`. Use the router to verify consumers receive events regardless of backend.
6. **Document dependencies.** Mention in the plan that window work consumes `plans/INPUT_PLAN.md` and feeds `plans/CANVAS_PLAN.md`, ensuring agents follow the recommended order.

## Context for downstream modules

- Input router must be operational before window hosts to receive/forward events.
- Canvases expect `DvzWindowSurface` plus `DvzInputRouter` from their window; they also attach to existing `DvzStream`/`dvz_stream_sink_video()` backends (see `plans/CANVAS_PLAN.md`) so stream/video plumbing can stay untouched.
