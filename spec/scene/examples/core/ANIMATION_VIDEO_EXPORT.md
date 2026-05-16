# Example: Animation And Video Export

> **Agent Pickup**
> - **Category:** `core`
> - **Implementation target:** Polished demo concept; implement in stages so the first slice can run with bounded resources.
> - **Data policy:** Public/downloaded assets require cache metadata and an offline fallback or reduced fixture.
> - **Preprocessing:** Usually required; specify source download, conversion, decimation/packing, and generated cache files.
> - **Validation:** Manual visual checklist plus bounded smoke command; add screenshot/readback validation when feasible.


## Summary

Build a staged animation example around a 3D scatter scene with opacity transition, camera fly-through,
and per-frame point updates, then export the offline-clock sequence to video. The brief describes 50,000
scatter points and camera keyframes, but the first practical slice should use deterministic synthetic point
data and bounded resource counts before any larger asset or cache path is introduced. Start by proving the
scene clock, animation handles, dirty updates, and frame emission path; then add the video export workflow.
Validation should combine the manual visual checklist with a bounded smoke command, plus screenshot or
readback checks where the runtime supports them.


This example traces the animation system through a 3D scatter scene with a camera fly-through
exported to video, combining all three animation types.

It also shows how the same scene setup runs in real-time interactive mode by switching the
clock.


## Owning Specs

This example should be read against:

1. `../../interaction/ANIMATION.md` for animation handles, construction, loop modes, clock modes, and the video
   export workflow,
2. `../../pipeline/FRAME_LIFECYCLE.md` for the step ordering: events → controllers → animations → invalidation
   → plan,
3. `../../interaction/CONTROLLERS.md` for the relationship between controllers and animations,
4. `../../pipeline/INVALIDATION_AND_CACHING.md` for how animation-driven state changes enter the dirty-scope
   pipeline,
5. `../../pipeline/FRAME_PLAN.md` for the resulting `FramePlan` shape.


## Scene Setup

1. one scene,
2. one 3D panel with a `Camera3DController`,
3. one `marker` visual — 50 000 3D scatter points,
4. three animations:
   - `fade_in`: `dvz_anim_transition` fading marker alpha from `0.0` to `1.0` over `1.0 s`,
   - `fly`: `dvz_anim_camera_path` flying the camera through five keyframes over `10.0 s`,
   - `stream`: `dvz_anim_timer` updating a subset of point positions every frame,
5. scene clock in offline mode at `60 fps` for video export.


## Animation Construction

```text
// 1. Opacity fade-in: 0 → 1 over the first second
fade_in = dvz_anim_transition(scene, &(DvzAnimTransitionDesc){
    .target   = {.visual = markers, .param = DVZ_PARAM_ALPHA},
    .from     = 0.0f,
    .to       = 1.0f,
    .duration = 1.0,
    .easing   = DVZ_EASING_EASE_OUT,
    .loop     = DVZ_LOOP_ONCE,
})

// 2. Camera fly-through: five keyframes over 10 seconds
DvzCameraKeyframe kf[5] = {
    {.t = 0.0, .position = {0, 0, 5},  .target = {0,0,0}},
    {.t = 2.5, .position = {3, 2, 4},  .target = {0,0,0}},
    {.t = 5.0, .position = {5, 0, 2},  .target = {0,0,0}},
    {.t = 7.5, .position = {3,-2, 4},  .target = {0,0,0}},
    {.t =10.0, .position = {0, 0, 5},  .target = {0,0,0}},
}
fly = dvz_anim_camera_path(scene, panel, &(DvzAnimCameraPathDesc){
    .keyframes   = kf,
    .n_keyframes = 5,
    .loop        = DVZ_LOOP_ONCE,
})

// 3. Per-frame streaming update: fires every frame
stream = dvz_anim_timer(scene, 0.0, on_stream_tick, user_data)
// on_stream_tick writes updated positions for a subset of markers each frame

// Clock: offline mode for video export
dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE)
dvz_scene_set_fps(scene, 60)

// Start all animations at t = 0
dvz_anim_start(fade_in, 0.0)
dvz_anim_start(fly,     0.0)
dvz_anim_start(stream,  0.0)
```


## Frame Lifecycle Trace

### Frame 0 — t = 0.000 s

Clock advances: `t = 0.000`.

Animation update step:

1. `fade_in` is active: `u = 0.0`, eased `u' = 0.0`, alpha written = `0.0`.
   Marks `VisualPropsDirty` for markers.
2. `fly` is active: camera interpolated at `t = 0.0` → position `{0, 0, 5}`, target `{0,0,0}`.
   Marks `PanelTransformDirty`.
3. `stream` fires: `on_stream_tick` writes new positions for batch 0.
   Marks `VisualDataDirty` for markers.

Invalidation resolution:

- `VisualDataDirty` → normalized marker data is stale → `UploadNode` required,
- `VisualPropsDirty` → marker parameter block is stale → `UploadNode` required,
- `PanelTransformDirty` → panel transform push required, no data reupload.

`FramePlan`:

```text
UploadNode  → normalized marker positions (batch 0 updated)
UploadNode  → marker parameter block (alpha = 0.0)
RenderNode  → panel: markers
```


### Frame 37 — t = 0.617 s  (mid fade-in, camera in motion)

Clock advances: `t += 1/60` each step, reaching `t ≈ 0.617`.

Animation update step:

1. `fade_in` is active: `u = 0.617`, `ease_out(0.617) ≈ 0.853`, alpha written = `0.853`.
   Marks `VisualPropsDirty`.
2. `fly` is active: camera interpolated between keyframe 0 and keyframe 1.
   Marks `PanelTransformDirty`.
3. `stream` fires: writes updated positions for batch 37.
   Marks `VisualDataDirty`.

`FramePlan`:

```text
UploadNode  → normalized marker positions (batch 37)
UploadNode  → marker parameter block (alpha = 0.853)
RenderNode  → panel: markers
```

Same shape as frame 0; costs are stable across the fly-through.


### Frame 60 — t = 1.000 s  (fade-in completes)

Animation update step:

1. `fade_in`: `u = 1.0`, alpha = `1.0`. Animation reaches `t_end`, stops automatically.
   Marks `VisualPropsDirty` for the final value.
2. `fly`: still active, camera between keyframe 0 and keyframe 1.
   Marks `PanelTransformDirty`.
3. `stream`: fires as usual.
   Marks `VisualDataDirty`.

After this frame `fade_in` is inactive. Subsequent frames will not produce a style-block upload
unless another animation or user action changes marker properties.

`FramePlan`:

```text
UploadNode  → normalized marker positions (batch 60)
UploadNode  → marker parameter block (alpha = 1.0)   ← final write; no further parameter uploads
RenderNode  → panel: markers
```


### Frame 600 — t = 10.000 s  (fly completes, stream still running)

Animation update step:

1. `fade_in`: inactive.
2. `fly`: `t = 10.0` reaches the final keyframe, camera at `{0, 0, 5}`. Animation stops.
   Marks `PanelTransformDirty` for the final position.
3. `stream`: fires as usual.
   Marks `VisualDataDirty`.

`FramePlan`:

```text
UploadNode  → normalized marker positions (batch 600)
RenderNode  → panel: markers
```

No style-block upload (alpha unchanged since frame 60).
No camera keyframe upload — camera path derived resources are not re-uploaded per frame;
the camera state is a panel-local transform updated in place.


## Video Export Loop

The application drives the frame loop explicitly in offline mode:

```text
dvz_scene_set_clock_mode(scene, DVZ_CLOCK_OFFLINE)
dvz_scene_set_fps(scene, 60)
// attach video sink to canvas (canvas-layer concern, not scene-layer)
dvz_canvas_attach_video_sink(canvas, &sink_desc)

while (dvz_scene_clock_time(scene) < 10.0) {
    dvz_scene_frame(scene)   // step clock, build FramePlan, emit DRP2, capture frame
}

dvz_canvas_finalize_video(canvas)
```

The scene's responsibility is limited to:

1. advancing `t` by `dt = 1/60` each call,
2. running the animation update step,
3. building and emitting a `FramePlan`.

Frame capture, encoding, and file output are canvas and sink concerns.
The total frame count is exactly `ceil(duration * fps) = 600` frames, regardless of GPU speed.


## Switching To Real-Time Interactive Mode

The same scene and animations work interactively by switching the clock:

```text
dvz_scene_set_clock_mode(scene, DVZ_CLOCK_REALTIME)
// the rendering loop now drives frames via vsync or on-demand redraw
// stream timer fires at the registered period relative to wall-clock t
// camera fly-through runs at the same authored speed
```

No other change is needed. The animation descriptors, loop modes, and `t_start` values are
unchanged. The clock mode is the only difference between offline export and live display.


## FramePlan Shape Summary

| Phase | UploadNodes | RenderNodes |
|---|---|---|
| Frames 0–59 (fade-in + fly + stream) | marker positions, marker style | panel |
| Frames 60–599 (fly + stream) | marker positions | panel |
| Frame 600 (stream only) | marker positions | panel |
| Frames 601+ (stream only, steady) | marker positions | panel |


## DRP2 Categories Implied

1. resource writes for dirty normalized marker data each frame (streaming),
2. resource writes for the marker parameter block during the fade-in window only,
3. panel transform push each frame (camera in motion),
4. render-pass lifecycle for the 3D panel,
5. draw commands for markers,
6. readback or copy to the video sink each frame (canvas-layer concern),
7. queue submission.


## Pressure On The Spec

This example checks that:

1. three concurrent animations of different types coexist without interference,
2. `fade_in` stopping after `t_end` correctly removes `VisualPropsDirty` from subsequent
   frames — no spurious style uploads after the fade completes,
3. `fly` stopping at the final keyframe does not produce further `PanelTransformDirty` — the
   camera rests at the authored end position,
4. `stream` (open-ended timer) continues independently after the other two animations stop,
5. the offline clock produces exactly `ceil(duration * fps)` frames — deterministic output,
6. switching to real-time mode requires only a clock-mode change, not a scene rebuild,
7. video capture is a canvas-layer concern — the scene emits `FramePlan` identically in both
   modes and is unaware of encoding.
