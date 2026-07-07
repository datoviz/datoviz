# C Runtime Examples

App lifecycle, native windowing, offscreen capture, recording, replay, and media export examples
belong here. Keep these examples focused on how a Datoviz program is hosted or produces artifacts;
scene-level retained capabilities belong in `../features/`.

- `app_glfw.c`: direct GLFW app/view lifecycle without the scenario runner.
- `multi_window.c`: one app driving two native GLFW windows.
- `offscreen_capture.c`: direct offscreen render-once PNG capture.
- `record_replay.c`: experimental DRP2 frame-stream recording and DVZR replay.
- `video_export.c`: deterministic offscreen MP4 export with `dvz_view_capture_start()`/stop.
