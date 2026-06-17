# Export Videos

Write an animation to a video output.

## Task Workflow

Create the scene and animation state, run a bounded frame sequence, update visual data each frame,
and write frames through the video-export path shown in the canonical example.

## Minimal Workflow

```c
DvzAppCaptureConfig capture = dvz_app_capture_config();
capture.flags = DVZ_APP_CAPTURE_VIDEO;
capture.directory = "captures";
capture.basename = "animation";
capture.fps = 60.0;

DvzView* view = dvz_view_offscreen(app, figure, width, height);
dvz_view_capture_start(view, &capture);

for (uint32_t frame = 0; frame < frame_count; frame++)
{
    update_scene(frame);
    dvz_app_run(app, 1);
}

dvz_view_capture_stop(view);
```

Do not hand-roll a separate renderer for video; reuse the app/offscreen frame path.


## Important Details

Video export is native-only in the current manifest. Keep frame count, frame size, and random seeds
fixed for reproducible output.

## Common Mistakes

- Recording an interactive run loop with non-deterministic timing when a fixed frame loop is needed.
- Updating scene state after capture instead of before capture.
- Assuming browser WebGPU live routes can export native video directly.

## See Also

- [Animate a scene](animation.md)
- [Save screenshots](capture-an-image.md)
- [Record and replay frame streams](replay-dvzr.md)

??? example "Related examples"

    - [Video Export](../examples/gallery/features/feature_video_export.md) - Source: `examples/c/features/video_export.c`
    - [Timer Animation](../examples/gallery/features/feature_timer_animation.md) - Source: `examples/c/features/timer_animation.c`
