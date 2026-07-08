# Record and Replay Frame Streams

Use Datoviz recordings when you need to capture rendered frames and replay them later for debugging
or release checks. This is an advanced diagnostic workflow; for ordinary visualization code, start
with [Create a scene](create-a-scene.md), [Open an interactive window](create-a-window.md), or
[Render offscreen](render-offscreen.md).

## Task Workflow

Record frames from an app view, keep the generated `*.dvzr/` directory, then replay it in another
view. Use this after you have already checked that the scene, visual data, panel attachment, and
controller setup are correct.

DVZR recordings are directories, not single files. Keep the whole directory together with the
Datoviz commit, platform, GPU/backend, dimensions, and command used to create it.

## What Is Being Recorded?

Record/replay does not save a video or scene objects. It saves the DRP2 commands and payload bytes
sent to the runtime for each frame, then feeds them back to a runtime later.

```text
scene -> DRP2 commands -> runtime -> pixels
                   |
                   v
              DVZR recording
```

The app-level API hides the command stream. Call `dvz_view_record_start()`, render one or more
frames, stop recording, then call `dvz_view_replay_start()` on another view.

A current `*.dvzr/` directory contains `manifest.json`, `stream.jsonl`, and optional `blobs/`
payload files. `stream.jsonl` is a JSON Lines debug/recording view. Runtime hot paths use typed
packets, not JSON.

For command-stream details, use [DRP2 command streams](../advanced/drp2-command-streams.md).

## Replay Path

| Need | Use |
| --- | --- |
| Record an offscreen or GLFW app view and replay it in another app view | `dvz_view_record_start()` and `dvz_view_replay_start()` |
| Replay a saved recording into a live native window | the same app replay calls on a window-backed view |
| Inspect lower-level command streams | [DRP2 command streams](../advanced/drp2-command-streams.md) |

## App-Level Recording Fragment

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

Public code should start from the runtime recording example and the app replay functions above.

## Advanced Recording API

Use lower-level recording functions only when you already work with Datoviz command streams or are
writing backend tests. Those APIs bypass the app-view helpers above, so they are documented in the
[DRP2 command streams](../advanced/drp2-command-streams.md) and the generated
[DRP2 C API](../reference/c-api/drp2.md).

## Important Details

DVZR is a diagnostic format, not editable scene source. Use ordinary scene examples for application
code unless the task is replay, validation, or backend debugging.

Replay depends on the recorded frame commands and the machine that replays them. A recording made
against one branch, backend, device capability set, or asset layout may not replay meaningfully
elsewhere. When saving a recording as evidence, record the commit, platform, backend, dimensions,
and command that produced it.

The portable part of a recording is the command stream and payload data. Development-only fallback
records may be local to one branch or runtime version and should not be treated as long-term public
artifacts.

Use DVZR after isolating a problem to frame execution. For visual attachment, scale, controller, or
callback bugs, debug the retained scene first.

## Common Mistakes

- Treating a replay stream as editable scene source.
- Recording with hidden local asset paths.
- Debugging visual behavior through a recording before checking the scene example.
- Keeping only one file from a `*.dvzr/` recording directory.
- Replaying old recordings after changing Datoviz versions or runtime assumptions without noting
  the source commit.

## See Also

- [Debug rendering output](debug-rendering.md)
- [Deploy Datoviz scenes to the web](deploy-to-web.md)
- [Profile rendering performance](profile-performance.md)
- [DRP2 command streams](../advanced/drp2-command-streams.md)
- [DRP2 C API](../reference/c-api/drp2.md)

??? example "Related examples"

    - [Record Replay](../examples/gallery/runtime/runtime_record_replay.md) - Source: `examples/c/runtime/record_replay.c`
    - [Raw Triangle DRP2](../examples/gallery/advanced/advanced_raw_triangle_drp2.md) - Source: `examples/c/advanced/raw_triangle_drp2.c`
