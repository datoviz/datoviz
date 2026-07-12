# Export Videos

Write rendered frames to a video output.

Use video export when native rendered frames should be encoded as a raster movie. Use the
deterministic offscreen path for tests, documentation, and release artifacts. Use live app
recording when you need to capture an interactive native window. For replayable frame streams, use
DVZR recording instead.

## Task Workflow

Choose the capture workflow first:

| Workflow | Use it for | View type | Reproducibility |
| --- | --- | --- | --- |
| Offscreen video export | Tests, docs, gallery clips, batch rendering | `dvz_view_offscreen()` | High when frame count, size, data, and time are fixed. |
| Live app recording | Native demos, controller interaction, manual workflows | `dvz_view_window()` | Depends on window size, user input, timing, and platform. |

Both workflows use `dvz_view_capture_start()` and `dvz_view_capture_stop()`. The difference is
whether frames come from a fixed offscreen sequence or from a visible app view.

## Recommended Example Path

The canonical example uses the app capture API directly. The default run records a 120-frame
offscreen video with the portable CPU-readback capture mode, and `--frames` lets you choose the
frame count. Build the source checkout first; a successful run reports the generated video path.
The complete checked source is `examples/c/runtime/video_export.c`.

```sh
./build/examples/c/runtime/video_export
./build/examples/c/runtime/video_export --frames 240
```

Some repository examples also accept command-line capture flags for documentation and release
checks. Application code should use the app capture API shown below.


## Portable CPU-Readback Capture

For deterministic export, create an offscreen view, start video capture, update scene state before
each frame, render a bounded number of frames, then stop capture. This function-body excerpt assumes
valid app/figure state and an `update_scene()` helper:

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

if (dvz_view_capture_start(view, &capture) != 0)
    return -1;

for (uint32_t frame = 0; frame < frame_count; frame++)
{
    update_scene(frame);
    dvz_app_run(app, 1);
}

if (dvz_view_capture_stop(view) != 0)
    return -1;
```

Do not hand-roll a separate renderer for video; reuse the app/offscreen frame path.


## Live App Recording

To record a visible native example, attach capture to the window-backed view before entering the app
loop, then stop capture after the loop returns. This is a function-body excerpt for the less
reproducible interactive path:

```c
DvzAppCaptureConfig capture = dvz_app_capture_config();
capture.flags = DVZ_APP_CAPTURE_VIDEO;
capture.directory = "captures";
capture.basename = "live-demo";
capture.fps = 60.0;
capture.video_capture_mode = DVZ_VIDEO_CAPTURE_CPU_READBACK;

DvzView* view = dvz_view_window(app, figure, width, height, "Datoviz");
if (view == NULL)
    return -1;

if (dvz_view_capture_start(view, &capture) != 0)
    return -1;
dvz_app_run(app, 0);
if (dvz_view_capture_stop(view) != 0)
    return -1;
```

This records the frames presented by that view. Window resizes, device scale, controller input, and
wall-clock timing can change the output, so do not use an unbounded live run for deterministic
release artifacts. For a reproducible live smoke, pass a finite frame count to `dvz_app_run()` and
drive animation from frame index or fixed scenario time.


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
| `DVZ_VIDEO_CAPTURE_CPU_READBACK` | Portable path; reads screenshot pixels and submits CPU RGBA frames to the encoder. |
| `DVZ_VIDEO_CAPTURE_EXTERNAL` | Advanced GPU interop path; the encoder consumes the rendered image through external memory/semaphore handles. |
| `DVZ_VIDEO_CAPTURE_AUTO` | Lets Datoviz choose the configured capture path. |

CPU video capture uses the same sRGB RGBA8 screenshot pixel contract as PNG capture. It is a movie
of rendered frames, not a scientific linear-float export.

The CPU-readback path is the documented default for portable examples. It is slower than GPU
interop but easier to validate across machines.


## Advanced Optional NVENC Path

NVENC is NVIDIA's hardware video encoder. In Datoviz, the NVENC path is an optional advanced
backend for NVIDIA systems that can encode rendered frames on the GPU through external
memory/semaphore interop. The point is to avoid the usual GPU-to-CPU readback and CPU-to-encoder
upload used by portable capture, so large or high-frame-rate videos can be much faster and involve
zero or few extra copies when the Vulkan, CUDA, and NVENC interop path is available.

This speedup only exists with the right hardware, driver, and build configuration. Without proper
NVIDIA/NVENC support, use the CPU-readback path. NVENC is not required for the canonical offscreen
video example.

Use NVENC only with the external capture mode:

```c
DvzAppCaptureConfig capture = dvz_app_capture_config();
capture.flags = DVZ_APP_CAPTURE_VIDEO;
capture.video_capture_mode = DVZ_VIDEO_CAPTURE_EXTERNAL;
capture.video_backend = "nvenc";
```

The same selection is available through environment variables in examples or tools that use
`dvz_app_capture_config_from_env()`:

```sh
DVZ_CAPTURE=mp4 \
DVZ_CAPTURE_VIDEO_MODE=external \
DVZ_CAPTURE_VIDEO_BACKEND=nvenc \
./build/examples/c/features/timer_animation --video 120
```

NVENC requires a build with CUDA/NVENC support, an NVIDIA driver and GPU that expose NVENC, and the
external-memory/semaphore path needed by the native runtime. If Datoviz is built without that
support, selecting `nvenc` reports the backend as unavailable rather than making the portable
CPU-readback example depend on NVIDIA hardware.


## Environment Capture

Use environment variables when examples or tools should opt into capture without changing source
code:

```sh
DVZ_CAPTURE=mp4 \
DVZ_CAPTURE_DIR=captures \
DVZ_CAPTURE_BASENAME=animation \
DVZ_CAPTURE_FPS=60 \
DVZ_CAPTURE_VIDEO_MODE=cpu \
./build/examples/c/features/timer_animation --video 120
```

`DVZ_CAPTURE` accepts `mp4`, `video`, `dvzr`, `png`, `all`, or false-like values such as `off` and
`none`. Use `DVZ_CAPTURE_VIDEO_MODE=external` plus `DVZ_CAPTURE_VIDEO_BACKEND=nvenc` only on
systems where the NVENC path is expected to work.


## Important Details

Video export is native-only in the current manifest. Browser WebGPU routes do not export native
videos directly.

Keep frame count, frame size, and random seeds fixed for reproducible offscreen output. Record live
window captures separately from deterministic exports in docs and release evidence.

Call `dvz_view_capture_stop()` before destroying the app or scene. This closes the encoder and
finalizes the output file.

Choose output directories intentionally. Treat encoded videos as generated outputs unless your
project explicitly stores them as documentation assets.

## Common Mistakes

- Treating live window recording and deterministic offscreen export as the same workflow.
- Recording an interactive run loop with non-deterministic timing when a fixed frame loop is needed.
- Updating scene state after capture instead of before capture.
- Selecting `nvenc` without external capture mode or without a CUDA/NVENC-capable build and system.
- Assuming browser WebGPU live routes can export native video directly.
- Forgetting to stop capture before destroying the view or app.
- Leaving output paths implicit, then looking for the video in the wrong working directory.
- Treating encoded videos as hand-written source assets.

## See Also

- [Animate a scene](animation.md)
- [Save screenshots](screenshots.md)
- [Record and replay frame streams](record-replay.md)

??? example "Related examples"

    - [Video Export](../examples/gallery/runtime/runtime_video_export.md) - Source: `examples/c/runtime/video_export.c`
    - [Timer Animation](../examples/gallery/features/features_timer_animation.md) - Source: `examples/c/features/timer_animation.c`
