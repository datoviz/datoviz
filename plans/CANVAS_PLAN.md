# Canvas Module Plan (Phase 1)

## Purpose

- Deploys the canvas layer that ties a window surface to swapchains, FrameStreams, and optional video sinks.
- Reuses the existing `src/stream`/`src/video` modules rather than reimplementing encoders; canvases simply attach to `DvzStream` sinks that are already registered (especially `dvz_stream_sink_video()`).
- Records basic CPU timing and stream submission flow while leaving advanced timing (GPU/presentation) for later phases.

## Module layout

```
include/datoviz/
├── canvas.h                 # Canvas public API + draw callback hook
├── stream/
│   └── frame_stream.h       # DvzStream, DvzStreamFrame definitions that Canvas consumes
src/canvas/
├── canvas.c                 # Public API + draw loop integration
├── canvas_stream.c          # FrameStream wiring + sink attachment helpers
├── window_surface.c         # Surface helpers shared with windows, swapchain sink
├── swapchain_sink.c         # Frame sink that presents to VkSurfaceKHR surfaces
├── canvas_internal.h        # Structures shared between canvas implementation and tests
├── tests/
│   └── test_canvas.c        # Lifecycle, swapchain sink, stream binding tests
└── CMakeLists.txt           # OBJECT lib + dependencies (include datoviz, src/common, src/window, src/stream)
```

Canvases own a `DvzStream` (see `plans/CANVAS_PLAN.md` references) that updates per-frame data and attaches the swapchain sink plus optional video sink from `src/video/video_sink.c`. Keep `${PROJECT_SOURCE_DIR}/src/common` and `${PROJECT_SOURCE_DIR}/src/window` on include paths so canvases can reach `_macros.h` and window/internal definitions.

## API snapshot (`include/datoviz/canvas.h`)

```c
typedef struct DvzCanvas DvzCanvas;

typedef struct
{
    DvzWindow* window;
    DvzDevice* device;
    VkFormat color_format;
    bool enable_video_sink;
    size_t timing_history;
} DvzCanvasConfig;

typedef struct
{
    uint64_t frame_id;
    double cpu_submit_us;
    double gpu_complete_us;
    double present_start_us;
    double present_done_us;
} DvzFrameTiming;

typedef void (*DvzCanvasDraw)(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data);

DVZ_EXPORT DvzCanvas* dvz_canvas_create(const DvzCanvasConfig* cfg);
DVZ_EXPORT int dvz_canvas_frame(DvzCanvas* canvas);
DVZ_EXPORT int dvz_canvas_submit(DvzCanvas* canvas);
/* remaining APIs for callbacks, timings, and router proxy omitted for brevity */
```

Canvases derive their size from `DvzWindowSurface` (logical × DPI scale) and do not store width/height in the public config yet. They manage a pool of exportable render targets sized to the swapchain image count, rotate through them every frame, and call `dvz_stream_submit()` via `dvz_canvas_submit()`.

## Step-by-step instructions for agents

1. **Consume the window surface.** Read `plans/WINDOW_PLAN.md` to understand how windows expose `DvzWindowSurface` and input routers. `dvz_canvas_create()` should query the surface, compute the physical extent, and initialize the swapchain sink configuration with the right format/extent.
2. **Attach FrameStream + swapchain sink.** Create a `DvzStream` per canvas, specify the swapchain sink (implemented under `src/canvas/swapchain_sink.c`), and register it via the stream sink registry. `DvzCanvas` should hold a pointer to the stream and update the shared `DvzStreamFrame` each time render targets rotate.
3. **Hook up the video sink.** When `enable_video_sink` is true (or the user supplies `DvzVideoSinkConfig`), call `dvz_stream_sink_video()` and attach it to the canvas stream. Since the video module already exists (`src/video/video_sink.c`), reuse it rather than writing new encoder logic.
4. **Record timing metadata.** Populate `DvzFrameTiming::cpu_submit_us` using host timestamps taken immediately before submitting the stream. Leave GPU/present fields zero for now; future phases (per plan) will fill them once the Vulkan timing extensions are available.
5. **Implement `dvz_canvas_frame()` / `dvz_canvas_submit()`.** `frame()` acquires a render target and records any platform-agnostic draw callbacks. `submit()` should signal the timeline semaphore, refresh exported `DvzStreamFrame` handles/FDs if images rotated, and call `dvz_stream_submit()` with the next timeline value.
6. **Expose input router.** Canvas should provide `dvz_canvas_input()` so renderers can access the router attached to the window. Input goes through the router (see `plans/INPUT_PLAN.md`) before reaching canvas code.
7. **Document tests.** Write tests under `src/canvas/tests/test_canvas.c` that cover canvas lifecycle, swapchain sink binding, stream submission, and video sink toggles. Reuse `testing.h` macros and ensure existing stream/video tests remain untouched.

## Stream & video context

- The stream module (`src/stream/`) already exposes `DvzStream`, `DvzStreamFrame`, sink registry, and `dvz_stream_sink_video()`. Canvases should rely on that existing implementation and not duplicate sink registries.
- `dvz_stream_sink_video()` (see `src/video/video_sink.c`) attaches NVENC/Kvazaar/stub encoders; simply reuse it when the canvas needs recording. Mention this dependency in the plan so agents know video support is ready once a canvas attaches a sink.
- Swapchain-specific presentation code lives in `src/canvas/swapchain_sink.c` and should register itself with the stream sink registry after being implemented.

## Current implementation status (`src/canvas/`)

- `canvas.c` already exposes the public API (`dvz_canvas_config()`, `dvz_canvas_create/destroy()`, `dvz_canvas_frame()`, `dvz_canvas_submit()`, draw callback registration, timings accessor, router proxy). It initializes a per-canvas `DvzStream`, rotates through a small CPU-side `DvzCanvasFramePool`, and records CPU submit timings in `dvz_canvas_timings_*`.
- `canvas_stream.c` wires canvases to the frame-stream module: it registers the swapchain sink backend stub, attaches it, starts/submits the stream, and toggles the optional video sink (`dvz_canvas_stream_enable_video()` delegates to `dvz_stream_sink_video()`).
- `window_surface.c` caches/retrieves `DvzWindowSurface` snapshots so canvases can track extent/format/scale without poking window internals.
- `tests/test_canvas.c` already covers default configuration, frame pool rotation, and timing buffer wraparound. As swapchain/presentation logic comes online, extend these tests rather than replacing them.
- `swapchain_sink.c` is still a stub. The remaining work described below lives in this backend plus new canvas-internal helpers that manage swapchain/offscreen images, synchronization primitives, exported handles, and stream updates.

## Agreed design decisions (Phase 1 swapchain bring-up)

1. **Device extension helper.** Introduce `dvz_device_request_canvas_extensions()` (or similar) that requests the minimum set of extensions canvases need *before* `dvz_device_create()` runs:
   - `VK_KHR_swapchain`
   - `VK_KHR_timeline_semaphore`
   - `VK_KHR_external_memory` + platform export (`VK_KHR_external_memory_fd` on Linux, Win32 equivalent later)
   - `VK_KHR_external_semaphore` + platform export (`VK_KHR_external_semaphore_fd` on Linux)
   Canvas creation must assert these extensions are present to avoid cryptic runtime failures.

2. **Canvas-owned exportable allocator.** Every canvas instantiates its own `DvzVma` via `dvz_device_allocator(device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT, …)` so it can allocate exportable render targets without forcing other modules to use external handles. Destroy the allocator when the canvas dies or rebuilds its swapchain.

3. **Offscreen exportable render targets.** Instead of rendering directly into swapchain images, canvases create one exportable offscreen image per swapchain image (matching extent/format). The draw callback renders into the offscreen image; right before presentation the swapchain sink blits/copies that image into the swapchain image. This guarantees stable export handles for `DvzStreamFrame`/video sinks, even while the swapchain is being recreated.

4. **Synchronization model.**
   - Keep classic per-frame binary semaphores (`image_available`, `render_finished`) plus per-frame fences so WSI works everywhere.
   - Create a single canvas-owned timeline semaphore, export its FD once, and signal it during every graphics submit. `dvz_canvas_stream_submit()` forwards the incremented wait value to all sinks.
   - Store the shared `wait_semaphore_fd` and per-frame `memory_fd`/image handles inside the `DvzCanvasFramePool` entries so stream consumers always see consistent data.

5. **Handle export caching.** Export each offscreen image’s memory FD exactly once at creation time and cache it next to the image/frame entry. Likewise, export the timeline semaphore FD once. When a swapchain rebuild occurs, close/destroy the old handles and re-export the new set before resuming frame submission. Notify sinks via `dvz_stream_update()` (or by reattaching them) so they can re-import the refreshed handles.

6. **Resize/recreate policy.**
   - `dvz_canvas_frame()` already refreshes the `DvzWindowSurface`. Compare extent/format against the cached swapchain configuration; if they changed—or if acquire/present returns `VK_ERROR_OUT_OF_DATE_KHR`/`VK_SUBOPTIMAL_KHR`—mark the swapchain dirty.
   - Pause submissions, wait on in-flight fences, destroy per-frame semaphores/fences/offscreen images, and rebuild the swapchain with the new extent. Recreate the offscreen image array (including exports), reinitialize the frame pool to match the new image count, and recreate the per-frame binary primitives.
   - Handle minimized windows (0×0 extent) by skipping swapchain creation until a valid extent returns.

7. **Frame/stream mapping.** Keep the frame pool sized to `swapchain.image_count` so rotating the pool aligns with `vkAcquireNextImageKHR` indices. Every time the swapchain/offscreen images change, update the frame pool entries (image handles, memory, FDs, wait semaphore FD) before letting `dvz_canvas_stream_start()` or subsequent `dvz_canvas_stream_submit()` calls proceed.

## Phased status update

- **Canvas frame plumbing**: `DvzStreamFrame` now carries the `VkCommandBuffer`, extent, and dirty-handles flag so draw callbacks can record into exportable offscreen images. `dvz_canvas_frame()` begins/ends the command buffer, reissues `dvz_stream_update()` whenever the export handles change, and returns explicit status codes for “wait for surface” vs. real errors.
- **Swapchain sink**: Slots now own command buffers, layout tracking, and the cached swapchain images. Acquisition records transitions/copies that blit the offscreen render target into the WSI image, signals the shared timeline semaphore, and handles resize/out-of-date situations without destroying the stream. Cleanup now frees the new resources safely.
- **Build state**: Compilation is still pending; the current patch leaves the sources consistent but the repo has not been built in the sandbox yet.

## Next steps

1. Run `just build` once the new Vulkan plumbing is ready and a suitable ICD is available; the canvas code depends on Vulkan+timeline extensions and may need adjustments uncovered by the compiler/linker.
2. Extend tests under `src/canvas/tests/` to exercise frame acquisition, `DVZ_CANVAS_FRAME_WAIT_SURFACE`, and stream updates so regressions are caught early.
3. Resume the plan’s Phase 2 items (canvas → vklite/canvas higher layers) once frame submission and the swapchain/video sinks are validated end-to-end.
