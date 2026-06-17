# Animation

Animate a visual by updating its data every frame using a per-frame callback.

## Overview

Datoviz renders continuously when the app is in continuous schedule mode. Register a frame
callback with `dvz_view_set_frame_callback` to update visual data before each frame is
presented. For declarative keyframe or rotation animations, the scene animation track API
(`dvz_anim_visual_transform`) handles interpolation automatically.

## Example

=== "C"

    ```c
    #include <math.h>
    #include <stdint.h>
    #include "datoviz/scene.h"

    #define N 8

    typedef struct {
        DvzVisual* point;
        float positions[N * 3];
        uint8_t colors[N * 4];
        float diameters[N];
        double time;
    } AnimState;

    static void on_frame(DvzView* view, void* user_data)
    {
        AnimState* s = (AnimState*)user_data;
        s->time += 1.0 / 60.0;

        /* recompute positions on a sine wave */
        for (int i = 0; i < N; i++) {
            float u = (float)i / (N - 1);
            float phase = 6.2832f * (u + 0.2f * (float)s->time);
            s->positions[3*i+0] = -0.8f + 1.6f * u;
            s->positions[3*i+1] = 0.4f * sinf(phase);
            s->positions[3*i+2] = 0.0f;
            s->diameters[i] = 20.0f + 14.0f * (0.5f + 0.5f * cosf(phase));
        }

        dvz_visual_set_data(s->point, "position", s->positions, N);
        dvz_visual_set_data(s->point, "diameter", s->diameters, N);
    }

    int main(void) {
        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* visual */
        AnimState state = {0};
        state.point = dvz_point(scene, 0);
        for (int i = 0; i < N; i++) {
            state.colors[4*i+0] = 80;
            state.colors[4*i+1] = 160;
            state.colors[4*i+2] = 240;
            state.colors[4*i+3] = 255;
        }
        dvz_visual_set_data(state.point, "color", state.colors, N);
        dvz_panel_add_visual(panel, state.point, NULL);

        /* app — continuous mode triggers on_frame every rendered frame */
        DvzAppConfig cfg = dvz_app_config();
        cfg.schedule_mode = DVZ_APP_SCHEDULE_CONTINUOUS;
        DvzApp* app = dvz_app_with_config(scene, &cfg);
        DvzView* view = dvz_view_glfw(app, figure, 800, 600, "Animation");

        /* register per-frame callback */
        dvz_view_set_frame_callback(view, on_frame, &state);

        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Set `schedule_mode` to `DVZ_APP_SCHEDULE_CONTINUOUS` in the app config before calling
`dvz_app_with_config`. Without this, the frame loop idles between input events and the callback
fires only on user interaction.

Register an animation callback with `dvz_view_set_frame_callback`. The callback receives the
view and a user data pointer. It runs after the previous frame's GPU work has been submitted,
so scene mutations (new data uploads via `dvz_visual_set_data`) take effect on the next
presented frame.

Inside the callback, recompute the data that changes each frame and upload it with
`dvz_visual_set_data`. Only upload attributes that actually changed — uploading unchanged
attributes is harmless but wastes bandwidth.

Destroy in the usual order: `dvz_app_destroy` then `dvz_scene_destroy`.

## Common patterns / Variants

**Track-based rotation** — for smooth keyframed or constant-speed rotation without a manual
callback, use the scene animation track API:

```c
DvzTrackRotationDesc rot = dvz_track_rotation_desc();
rot.axis[0] = 0.0f; rot.axis[1] = 1.0f; rot.axis[2] = 0.0f;
rot.speed_rad_per_sec = 1.0f;
DvzTrack* track = dvz_track_rotation(&rot);

DvzTransformMotionDesc motion = dvz_transform_motion_desc();
motion.rotation = track;
DvzAnimation* anim = dvz_anim_visual_transform(scene, visual, &motion);
dvz_anim_start(anim, 0.0);
```

The scene clock drives the track automatically; no frame callback is needed.

**Fixed frame count** — to render exactly N frames (e.g. for video export) pass N to
`dvz_app_run` instead of 0:

```c
dvz_app_run(app, 120);   /* render 120 frames then stop */
```

## See also

- [Video export](video-export.md) — record frames to a video file
- [Update visual data](update-visual-data.md) — partial and full attribute uploads
- [3D navigation](3d-navigation.md) — arcball and orbit camera controllers
