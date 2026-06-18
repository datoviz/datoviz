# Input Module Plan (Phase 1)

## Why this module?

- Establishes the backend-agnostic router that all windows/canvases rely on for pointer, keyboard, resize, and scale events.
- Meets the Phase 1 requirement for timeline semaphore export/import before canvases submit frames.
- Keeps legacy v0.3 helpers as internal algorithms (click/gesture heuristics, modifier mapping) but routes every event through the new router API.

## Scope & layout

```
include/datoviz/input/
├── input.h           # Aggregator for the router and helpers
├── pointer.h         # DvzPointerEvent definition and enums
├── keyboard.h        # DvzKeyboardEvent definition and scancodes
├── router.h          # DvzInputRouter public API (subscribe/emit)
src/input/
├── input_router.c    # Router implementation, dynamic callback arrays, event dispatch
├── pointer.c         # Event normalization, gesture heuristics, pointer helpers
├── keyboard.c        # Modifier mapping, key translation helpers
├── tests/
│   └── test_input.c   # Router behavior and legacy helper regression tests
└── CMakeLists.txt     # OBJECT library + include dirs
```

Keep `${PROJECT_SOURCE_DIR}/src/common` on the include path so `_macros.h` stays reachable. The router should remain stateless and thread-agnostic; more specialized helpers (gesture state, pressed-key tracking) subscribe to it rather than keeping their own callbacks.

## API snapshot (see `include/datoviz/input/router.h`)

```c
typedef struct DvzInputRouter DvzInputRouter;

DVZ_EXPORT DvzInputRouter* dvz_input_router(void);
DVZ_EXPORT void dvz_input_router_destroy(DvzInputRouter* router);
DVZ_EXPORT void dvz_input_subscribe_pointer(
    DvzInputRouter* router, DvzPointerCallback cb, void* user_data);
DVZ_EXPORT void dvz_input_emit_pointer(DvzInputRouter* router, const DvzPointerEvent* event);
/* keyboard + union-style DvzInputEvent emitters follow the same pattern */
```

## Step-by-step instructions for agents

1. **Design the router internals.** Allocate a single `DvzInputRouter` per window host, manage dynamic callback storage, and make each subscription flag whether it listens for pointer, keyboard, or generic events.
2. **Implement pointer and keyboard emission helpers.** Normalize coordinates to window-relative space, translate backend buttons into `DvzPointerButton`, compute modifier bitmasks, and stamp `timestamp_ns`. Emit pointer/keyboard events through the router API so canvases always see the same structures regardless of GLFW/Qt/Headless backends.
3. **Publish threaded events.** Document whether callbacks run on the backend thread (GLFW main thread, Qt UI thread) so downstream code knows about thread safety. Event emission should not block; backends can queue events and flush them during `dvz_window_host_poll()`.
4. **Port legacy heuristics.** Reuse the v0.3 pointer/keyboard helpers for gesture timing thresholds and modifier mapping, but move them behind the router (e.g., subscribe to pointer events and re-emit gesture tags via `dvz_input_emit_event()` when needed). Do not reintroduce old public APIs like `dvz_mouse_*`.
5. **Write tests.** Cover subscription/callback order, resize/scale event emission, and gesture helper behavior under `src/input/tests/test_input.c`. Use `testing.h` helpers (`TEST_SIMPLE`, `AT`, etc.).
6. **Document cross-module expectations.** Mention in the plan file where windows/canvases hook into the router so future agents understand the dependency order.

   Window hosts (see `plans/WINDOW_PLAN.md`) will create a `DvzInputRouter` per window and hand that instance to canvases—`dvz_canvas_input()` consumes the same router so canvases can subscribe before submitting frames.

## Next steps once Input is in place

- Proceed to the window module plan so hosts can consume the router per window.
- Keep stream/video contexts nearby: windows will feed `DvzWindowSurface` to canvases, and canvases already attach to `dvz_stream_sink_video()` from the existing `src/video` backend.
