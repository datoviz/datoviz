# C Runtime Examples

App lifecycle, native windowing, offscreen capture, recording, replay, and media export examples
belong here. Keep these examples focused on how a Datoviz program is hosted or produces artifacts;
scene-level retained capabilities belong in `../features/`.

- `app_glfw.c`: direct GLFW app/view lifecycle without the scenario runner.
- `offscreen_capture.c`: direct offscreen render-once PNG capture.
- `record_replay.c`: experimental DVZR app recording and live replay.
- `video_export.c`: experimental runner-backed live, live-record, and offscreen-record modes.
