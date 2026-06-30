# Record and Replay Frame Streams

Use DVZR frame streams for reproducible rendering and portability checks.

## Task Workflow

Record the command stream emitted by an app view, then replay it through the same runtime boundary.
Use this when debugging frame execution, backend portability, or a regression that happens after the
scene has already lowered to DRP2. For ordinary plotting or scene authoring, start from the scene
and visual examples instead.

DVZR recordings are directories, not single files. Keep the whole `*.dvzr/` directory together with
the runtime, platform, and asset assumptions used to create it.

## Choose The Replay Path

| Need | Use |
| --- | --- |
| Record an offscreen or GLFW app view and replay it in another app view | `dvz_view_record_start()` and `dvz_view_replay_start()` |
| Replay a saved recording into a live native window | `examples/c/lab/replay_dvzr.c` |
| Execute a recording directly against a DRP2 runtime | `dvz_drp2_recording_open()` plus `dvz_drp2_recording_execute_*()` |
| Build or inspect hand-written DRP2 command streams | `examples/c/advanced/raw_triangle_drp2.c` |

## Minimal Call Sequence

For app-level recording, start recording before the frame you want to capture, render frames, then
stop recording before replaying the directory.

```c
DvzView* view = dvz_view_offscreen(app, figure, width, height);

dvz_view_record_start(view, "capture.dvzr");
dvz_view_render_once(view);
dvz_view_record_stop(view);

DvzFigure* replay_figure = dvz_figure(scene, width, height, 0);
DvzView* replay = dvz_view_offscreen(app, replay_figure, width, height);

dvz_view_replay_start(replay, "capture.dvzr");
dvz_view_replay_set_paced(replay, false);
dvz_view_render_once(replay);
dvz_view_replay_stop(replay);
```

Use `dvz_view_replay_frame_count()` when replaying a multi-frame recording into `dvz_app_run()`.
Use `dvz_view_replay_set_loop()`, `dvz_view_replay_set_paced()`, and
`dvz_view_replay_set_speed()` for live inspection.

```c
dvz_view_replay_start(view, "capture.dvzr");
dvz_view_replay_set_loop(view, true);
dvz_view_replay_set_paced(view, true);
dvz_view_replay_set_speed(view, 0.5);
dvz_app_run(app, 0);
dvz_view_replay_stop(view);
```

## Native Example Commands

Build the examples first:

```sh
just build
```

Record an offscreen scene, replay it, and write comparison PNGs:

```sh
./build/examples/c/runtime/record_replay
```

The feature example writes `record_replay.dvzr/`, `record_replay_original.png`, and
`record_replay_replay.png` next to the executable.

Record with the lab example and replay the same directory through the standalone replay tool:

```sh
./build/examples/c/lab/record_dvzr
./build/examples/c/lab/replay_dvzr ./build/examples/c/lab/record_dvzr.dvzr
```

Useful replay options:

```sh
./build/examples/c/lab/replay_dvzr --fast path/to/recording.dvzr
./build/examples/c/lab/replay_dvzr --loop --speed 0.5 path/to/recording.dvzr
./build/examples/c/lab/replay_dvzr --frames 120 path/to/recording.dvzr
```

## Lower-Level DRP2 Recording

Use the DRP2 recording API when you already have a `DvzDrp2CommandStream` or are writing backend
tests. This bypasses scene and app-view convenience plumbing.

```c
DvzDrp2RecordingInfo info = dvz_drp2_recording_info();
info.width = width;
info.height = height;
info.backend_hint = "vklite";

dvz_drp2_recording_write_stream("stream.dvzr", stream, &info);

DvzDrp2Recording* recording = dvz_drp2_recording_open("stream.dvzr");
DvzDrp2ValidationResult result = dvz_drp2_recording_execute_all(recording, runtime);
dvz_drp2_recording_close(recording);
```

For multi-frame streams, open a `DvzDrp2Recorder`, append timestamped streams with
`dvz_drp2_recorder_write_stream()`, then close it with `dvz_drp2_recorder_close()`.


## Important Details

DVZR is a frame-stream/runtime concern. Use ordinary scene examples for application code unless the
task is replay, validation, or backend debugging.

Replay depends on the recorded command surface and runtime capabilities. A recording made against
one branch, backend, device capability set, or asset layout may not replay meaningfully elsewhere.
When saving a recording as evidence, record the commit, platform, backend, dimensions, and command
that produced it.

The portable part of a recording is the DRP2 command stream and payload blobs. Development-only raw
fallback command records may be ABI-local and should not be treated as long-term public artifacts.

DVZR is useful after you have isolated the problem to frame execution. If the question is "did I
attach the right visual, scale, controller, or callback?", debug the retained scene first.

## Common Mistakes

- Treating a replay stream as editable scene source.
- Recording with hidden local asset paths.
- Debugging high-level visual behavior through DRP2 before checking the scene example.
- Keeping only one file from a `*.dvzr/` recording directory.
- Replaying old recordings after changing the command schema or runtime assumptions without noting
  the source commit.

## See Also

- [Debug rendering output](debug-rendering.md)
- [Deploy WebGPU examples to the browser](deploy-to-web.md)
- [Profile rendering performance](profile-performance.md)
- [DRP2 reference](../reference/drp2/index.md)
- [DRP2 C API](../reference/c-api/drp2.md)

??? example "Related examples"

    - [Record Replay](../examples/gallery/runtime/feature_record_replay.md) - Source: `examples/c/runtime/record_replay.c`
    - [Raw Triangle DRP2](../examples/gallery/advanced/advanced_raw_triangle_drp2.md) - Source: `examples/c/advanced/raw_triangle_drp2.c`
