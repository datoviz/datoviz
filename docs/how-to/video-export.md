# Export Videos

Write an animation to a video output.

Use video export when a deterministic native frame sequence should be encoded as a raster movie.
For replayable frame streams, use DVZR recording instead.

## Task Workflow

Create the scene and animation state, run a bounded frame sequence, update visual data each frame,
and write frames through the video-export path shown in the canonical example.

## Recommended Example Path

The canonical example uses the native scenario runner. The default run records a 120-frame
offscreen video, and the explicit offscreen mode lets you choose the frame count:

```sh
./build/examples/c/runtime/video_export
./build/examples/c/runtime/video_export --offscreen-record 120
```

Use `--live-record N` when you want to display a GLFW view while recording an offscreen capture
view. Use `--live` only for interactive viewing without recording.


## Minimal Capture Sequence

At the app level, create an offscreen view, start video capture, update scene state before each
frame, render a bounded number of frames, then stop capture:

```c
DvzAppCaptureConfig capture = dvz_app_capture_config();
capture.flags = DVZ_APP_CAPTURE_VIDEO;
capture.directory = "captures";
capture.basename = "animation";
capture.fps = 60.0;
capture.video_capture_mode = DVZ_VIDEO_CAPTURE_CPU_READBACK;

DvzView* view = dvz_view_offscreen(app, figure, width, height);
if (view == NULL)
    return -1;

dvz_view_capture_start(view, &capture);

for (uint32_t frame = 0; frame < frame_count; frame++)
{
    update_scene(frame);
    dvz_app_run(app, 1);
}

dvz_view_capture_stop(view);
```

Do not hand-roll a separate renderer for video; reuse the app/offscreen frame path.


## Frame Timing

Make the animation a pure function of the frame index or scenario time when reproducibility matters.
Do not record an unbounded interactive loop for release artifacts or tests.

```c
for (uint32_t frame = 0; frame < frame_count; frame++)
{
    double t = (double)frame / capture.fps;
    update_scene_at_time(t);
    dvz_app_run(app, 1);
}
```

Update retained scene data before rendering the frame that should contain the update.


## Backends And Modes

| Setting | Use |
| --- | --- |
| `DVZ_VIDEO_CAPTURE_CPU_READBACK` | Portable offscreen path; reads screenshot pixels and encodes them on the CPU. |
| `auto` video mode | Lets Datoviz choose the configured video path. |
| external video mode | Use only when the environment and backend support the external encoder path. |

CPU video capture uses the same sRGB RGBA8 screenshot pixel contract as PNG capture. It is a movie
of rendered frames, not a scientific linear-float export.


## Environment Capture

Use environment variables when examples or tools should opt into capture without changing source
code:

```sh
DVZ_CAPTURE=mp4 \
DVZ_CAPTURE_DIR=captures \
DVZ_CAPTURE_BASENAME=animation \
DVZ_CAPTURE_FPS=60 \
DVZ_CAPTURE_VIDEO_MODE=cpu \
./build/examples/c/features/timer_animation 120
```

`DVZ_CAPTURE` accepts `mp4`, `video`, `dvzr`, `png`, `all`, or false-like values such as `off` and
`none`.


## Important Details

Video export is native-only in the current manifest. Keep frame count, frame size, and random seeds
fixed for reproducible output.

Call `dvz_view_capture_stop()` before destroying the app or scene. This closes the encoder and
finalizes the output file.

Choose output directories intentionally. Generated videos should stay out of source control unless
the release or docs pipeline explicitly expects that artifact.

## Common Mistakes

- Recording an interactive run loop with non-deterministic timing when a fixed frame loop is needed.
- Updating scene state after capture instead of before capture.
- Assuming browser WebGPU live routes can export native video directly.
- Forgetting to stop capture before destroying the view or app.
- Leaving output paths implicit, then looking for the video in the wrong working directory.
- Treating encoded videos as source files instead of generated artifacts.

## See Also

- [Animate a scene](animation.md)
- [Save screenshots](capture-an-image.md)
- [Record and replay frame streams](replay-dvzr.md)

??? example "Related examples"

    - [Video Export](../examples/gallery/runtime/feature_video_export.md) - Source: `examples/c/runtime/video_export.c`
    - [Timer Animation](../examples/gallery/features/feature_timer_animation.md) - Source: `examples/c/features/timer_animation.c`
