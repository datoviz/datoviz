# Input/Window/Canvas Plan Index

This repository now keeps the Phase 1 guidance in three focused files so Codex agents can work sequentially without wading through unrelated sections. Follow the numbered order below and keep the referenced helpers (stream/video) in mind as you implement each module.

1. **Phase 1 – Input** → `plans/INPUT_PLAN.md`  
   - Build the router, pointer/keyboard emitters, and legacy-helper wrappers described there. Tests live under `src/input/tests/`.

2. **Phase 1 – Window** → `plans/WINDOW_PLAN.md`  
   - Consume the router per window, expose Vulkan surfaces, and wrap GLFW/Qt/headless backends while emitting events into the router. This plan points to the window-specific files and the expected event-loop strategy.

3. **Phase 1 – Canvas** → `plans/CANVAS_PLAN.md`  
   - Create canvases that pull from windows, attach swapchain/video sinks, and submit frames through the existing `src/stream`/`src/video` machinery. Follow the detailed steps in that file before touching swapchain sinks.

### Context

- Stream (`src/stream/*`) and video (`src/video/*`) already exist; canvases should reuse their `DvzStream`, sink registry, and `dvz_stream_sink_video()` implementations rather than reengineering those layers.
- Keep future phases (multi-view, advanced timing, Qt loop integration beyond the stub, etc.) in mind but do not implement them until explicitly requested.
