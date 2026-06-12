# Scene Animation

This document defines the v0.4 scene animation direction. The preferred high-level model is
track-based: reusable typed time functions are evaluated by scene-owned animations and applied to
scene targets through adapters.

Older transition-specific and camera-path-specific API sketches are superseded by this document.
Keep `dvz_anim_timer()` and `dvz_anim_phase()` as low-level escape hatches, but new examples and
documentation should prefer tracks and adapters once they land.


## Core Model

The animation system has three separate concepts:

```text
track      = pure typed function of local animation time
animation  = scene-clock lifecycle and running state
adapter    = applies evaluated track values to a scene target
```

A track does not know about scenes, frames, controllers, visuals, or cameras. It can be evaluated
manually and reused by multiple animations.

An animation is scene-owned. It maps scene-clock time to local track time, starts/stops/destroys
cleanly, marks the scene dirty when it changes state, and owns interaction policy.

An adapter is target-specific glue. Examples include applying a rotation track to a visual transform,
or applying eye/target tracks to a camera view.

Do not expose "trajectory" as a public API concept. A spatial trajectory is just a `DvzTrack` whose
type is `vec2` or `vec3`.


## Scene Clock

The scene clock is the single source of time for all animations. It is a scene abstraction, not a
raw platform timer.

In real-time mode, the clock advances from wall-clock deltas:

```text
t_scene += t_wall_delta
```

Use real-time mode for interactive sessions, live streaming, and responsive transitions.

In offline mode, the clock advances by a fixed step:

```text
t_scene += 1 / fps
```

Use offline mode for deterministic video export. A 30-second animation at 60 fps produces exactly
1800 frames regardless of GPU speed.

Public clock controls remain:

```c
void dvz_scene_set_clock_mode(DvzScene* scene, DvzSceneClockMode mode);
void dvz_scene_set_fps(DvzScene* scene, double fps);
double dvz_scene_clock_time(const DvzScene* scene);
double dvz_scene_clock_dt(const DvzScene* scene);
bool dvz_scene_has_active_animations(const DvzScene* scene);
```


## Track API

`DvzTrack` is a scene-independent typed function:

```text
local time -> float / vec2 / vec3 / vec4 / quat
```

Public constructors should use descriptors with `struct_size` and `flags` for ABI growth.

```c
typedef struct DvzTrack DvzTrack;

typedef enum
{
    DVZ_TRACK_FLOAT,
    DVZ_TRACK_VEC2,
    DVZ_TRACK_VEC3,
    DVZ_TRACK_VEC4,
    DVZ_TRACK_QUAT,
} DvzTrackType;

typedef enum
{
    DVZ_TRACK_REPEAT_NONE,
    DVZ_TRACK_REPEAT_LOOP,
    DVZ_TRACK_REPEAT_PINGPONG,
} DvzTrackRepeat;

typedef enum
{
    DVZ_TRACK_INTERP_STEP,
    DVZ_TRACK_INTERP_LINEAR,
    DVZ_TRACK_INTERP_CATMULL_ROM,
    DVZ_TRACK_INTERP_CUBIC_HERMITE,
    DVZ_TRACK_INTERP_SLERP,
    DVZ_TRACK_INTERP_MONOTONE_CUBIC,
} DvzTrackInterpolation;

typedef enum
{
    DVZ_TRACK_TOPOLOGY_OPEN,
    DVZ_TRACK_TOPOLOGY_CLOSED,
} DvzTrackTopology;
```

Required first-wave constructors:

```c
DvzTrack* dvz_track_constant(const DvzTrackConstantDesc* desc);
DvzTrack* dvz_track_linear(const DvzTrackLinearDesc* desc);
DvzTrack* dvz_track_keyframes(const DvzTrackKeyframesDesc* desc);
DvzTrack* dvz_track_circle2(const DvzTrackCircle2Desc* desc);
DvzTrack* dvz_track_circle3(const DvzTrackCircle3Desc* desc);
DvzTrack* dvz_track_rotation(const DvzTrackRotationDesc* desc);

bool dvz_track_eval(const DvzTrack* track, double t, void* out);
void dvz_track_destroy(DvzTrack* track);
```

`dvz_track_rotation()` returns a quaternion track. Circle and rotation constructors use radians and
`speed_rad_per_sec`. Linear tracks and keyframes use units per second as implied by their values and
time coordinates.


## Keyframes

Keyframes must be descriptor-based from the first implementation so interpolation, tangent,
ownership, and extrapolation behavior can grow without signature churn.

```c
typedef enum
{
    DVZ_TRACK_TANGENT_AUTO,
    DVZ_TRACK_TANGENT_FLAT,
    DVZ_TRACK_TANGENT_USER,
} DvzTrackTangentMode;

struct DvzTrackKeyframesDesc
{
    uint32_t struct_size;
    uint32_t flags;

    DvzTrackType type;
    uint32_t count;
    const double* times;
    const void* values;

    DvzTrackRepeat repeat;
    DvzTrackTopology topology;
    DvzTrackInterpolation interpolation;
    DvzTrackTangentMode tangents;

    float tension;
    const void* in_tangents;
    const void* out_tangents;
};
```

Implementation order:

1. `STEP`, `LINEAR`, and `CATMULL_ROM` for float and vector tracks.
2. `SLERP` for quaternion keyframes and rotation tracks.
3. `CUBIC_HERMITE` when user tangents are needed.
4. `MONOTONE_CUBIC` for scalar tracks where overshoot is harmful, such as opacity, slice index, or
   time cursor values.

Constructors should copy input arrays by default. Add a borrow flag only if profiling shows copying
keyframes is a real cost.

Closed keyframe topology does not infer an implicit closing segment. Authors provide explicit timing
by duplicating the first value at the final timestamp. `DVZ_TRACK_TOPOLOGY_CLOSED` then makes
Catmull-Rom interpolation use cyclic control points at the first and final segments so the duplicated
endpoint has the correct tangent context.


## Track Time

Tracks evaluate in animation-local time, not absolute scene time.

`dvz_anim_start(animation, t_start)` maps the scene clock to local `t = 0` at the requested start
point. This makes repeating animations restart predictably and allows the same track to be reused by
multiple animations with different start times.

Repeat behavior belongs to the track because it defines how local time maps to values:

| Mode | Behavior |
|---|---|
| `DVZ_TRACK_REPEAT_NONE` | evaluate once over the authored domain; clamp or stop by descriptor policy |
| `DVZ_TRACK_REPEAT_LOOP` | wrap local time into the authored domain |
| `DVZ_TRACK_REPEAT_PINGPONG` | alternate forward and backward cycles |


## Animation API

Animations are first-class scene objects with stable handles:

```c
void dvz_anim_start(DvzAnimation* animation, double t_start);
void dvz_anim_stop(DvzAnimation* animation);
void dvz_anim_destroy(DvzAnimation* animation);
void dvz_anim_set_speed(DvzAnimation* animation, float speed);
```

Stopping an animation does not revert scene state. The last value written remains.

The generic track adapter is the lowest-level high-level primitive:

```c
typedef void (*DvzTrackApplyCallback)(
    DvzAnimation* animation,
    double t,
    const void* value,
    void* user_data);

DvzAnimation* dvz_anim_track(
    DvzScene* scene,
    const DvzTrack* track,
    DvzTrackApplyCallback callback,
    void* user_data);
```

This should replace example-local `dvz_anim_phase()` callbacks when the work is simply "evaluate a
typed time function and apply it". Keep `dvz_anim_timer()` and `dvz_anim_phase()` for unusual
procedural cases, live data, and compatibility during the rewrite.


## Transform Motion

Visual/object motion should be declarative. The transform adapter composes optional translation,
rotation, and scale tracks and writes the retained visual-local transform.

```c
struct DvzTransformMotionDesc
{
    uint32_t struct_size;
    uint32_t flags;

    const DvzTrack* translation; // vec3, optional
    const DvzTrack* rotation;    // quat, optional
    const DvzTrack* scale;       // vec3, optional

    vec3 pivot;
    DvzTransformOrder order;
};

DvzTransformMotionDesc dvz_transform_motion_desc(void);

DvzAnimation* dvz_anim_visual_transform(
    DvzScene* scene,
    DvzVisual* visual,
    const DvzTransformMotionDesc* desc);
```

Define transform order explicitly before public exposure. The default should match retained
visual-local transform semantics. Pivot behavior must be tested.

Example: rotating a planet or molecule:

```c
DvzTrack* spin = dvz_track_rotation(&(DvzTrackRotationDesc){
    .axis = {0, 0, 1},
    .speed_rad_per_sec = 0.12f,
});

DvzTransformMotionDesc motion = dvz_transform_motion_desc();
motion.rotation = spin;

DvzAnimation* anim = dvz_anim_visual_transform(scene, visual, &motion);
```


## Camera Motion

Camera motion composes tracks rather than using a special flyover or camera-path API.

```c
typedef enum
{
    DVZ_CAMERA_UP_FIXED,
    DVZ_CAMERA_UP_WORLD,
    DVZ_CAMERA_UP_TRACK,
} DvzCameraUpMode;

struct DvzCameraMotionDesc
{
    uint32_t struct_size;
    uint32_t flags;

    const DvzTrack* eye;    // vec3
    const DvzTrack* target; // vec3

    DvzCameraUpMode up_mode;
    vec3 up;
    const DvzTrack* up_track; // vec3, optional
};

DvzCameraMotionDesc dvz_camera_motion_desc(void);

DvzAnimation* dvz_anim_camera_motion(
    DvzScene* scene,
    DvzCamera* camera,
    const DvzCameraMotionDesc* desc);
```

`DVZ_CAMERA_UP_WORLD` should stabilize roll by projecting the world-up vector onto the current view
plane and falling back to a valid perpendicular vector near singularities.

Example: a tilted offset planet flyover, opposite the planet spin:

```c
DvzTrack* eye = dvz_track_circle3(&(DvzTrackCircle3Desc){
    .center = {0.0f, -0.45f, 0.25f},
    .normal = {0.25f, 0.0f, 1.0f},
    .radius = 2.8f,
    .phase = 0.0f,
    .speed_rad_per_sec = -0.025f,
});

DvzTrack* target = dvz_track_constant(&(DvzTrackConstantDesc){
    .type = DVZ_TRACK_VEC3,
    .value = (float[3]){0, 0, 0},
});

DvzCameraMotionDesc motion = dvz_camera_motion_desc();
motion.eye = eye;
motion.target = target;
motion.up_mode = DVZ_CAMERA_UP_WORLD;
glm_vec3_copy((vec3){0, 0, 1}, motion.up);

DvzAnimation* flyover = dvz_anim_camera_motion(scene, camera, &motion);
```


## Interaction Policy

Interaction policy belongs to animations, not tracks. Tracks describe values; policies describe
ownership conflicts between animations and controllers.

```c
typedef enum
{
    DVZ_ANIM_INTERACTION_CONTINUE,
    DVZ_ANIM_INTERACTION_STOP,
    DVZ_ANIM_INTERACTION_PAUSE,
    DVZ_ANIM_INTERACTION_RESUME_AFTER_IDLE,
} DvzAnimInteractionPolicy;

void dvz_anim_set_interaction_policy(
    DvzAnimation* animation,
    DvzController* controller,
    DvzAnimInteractionPolicy policy,
    double idle_s);
```

The first implementation may support only `CONTINUE` and `STOP`. Add pause/resume-after-idle only
when a real UI needs it.

For `textured_planet`, the default camera flyover should stop on the first orbit-camera pointer or
wheel interaction so scripted motion never fights user control.


## Redraw Scheduling

Animations update before invalidation resolution:

```text
1. ingest events
2. update controllers
3. update animations
4. resolve invalidation
5. validate
6. adapt capabilities
7. build FramePlan
8. emit DRP2
```

Rules:

1. An active animation marks the scene dirty when it writes a new value.
2. If no animation is active and no interaction is pending, the scene does not request redraws.
3. In offline mode, the scene advances by one fixed step and renders; idle scheduling does not
   apply.


## Video Export

Offline clock mode is the foundation for deterministic video export.

Workflow:

1. The application attaches a video sink to the canvas stream.
2. The application sets the scene clock to offline mode and configures `fps`.
3. The application drives the frame loop for the desired number of frames.
4. Each frame advances the scene clock, updates animations, builds a `FramePlan`, emits DRP2, and
   lets the canvas/video layer capture output.

The scene owns only the clock, animation update, and frame-plan consequences. Codecs, files, and
video sinks remain canvas/app concerns.


## Scientific Visualization Pressure Tests

These examples should be expressible without custom callbacks except where noted:

| Use case | Expression |
|---|---|
| Electrophysiology playback cursor | linear float track -> generic track adapter updates cursor x |
| Microscopy scan reticle | keyframed vec2 track -> marker or overlay position |
| Tomography or brain slice sweep | pingpong float track -> volume slice parameter |
| Oceanography drifter | keyframed vec3 track -> visual transform translation |
| Molecular rotation | rotation quat track -> visual transform rotation |
| Moving volume probe | keyframed vec3 track -> marker transform and optional readout callback |
| Camera following a simulated particle | keyframed vec3 target + eye track -> camera motion |
| Planet showcase | rotation track -> planet spin; tilted circle3 eye + fixed target -> camera flyover |


## Implementation Plan

Aggressively replace v0.4-era callback-heavy examples once the track API lands.

1. Add internal `DvzTrack` storage and evaluators in `src/scene/interaction/`.
2. Add public declarations in `include/datoviz/scene/animation.h` or a focused
   `include/datoviz/scene/track.h` included by the umbrella scene header.
3. Add tests for constant, linear, repeat, pingpong, circle2, circle3, rotation, and keyframe
   interpolation. Cover local-time restart behavior.
4. Add `dvz_anim_track()` and verify lifecycle, start/stop/restart, destruction, and scene dirty
   marking.
5. Add `dvz_anim_visual_transform()` and tests for translation, rotation, scale, pivot, and order.
6. Add `dvz_anim_camera_motion()` and tests for eye/target evaluation, world-up stabilization, and
   invalid track type rejection.
7. Add the first `dvz_anim_set_interaction_policy()` slice with controller-interaction stop.
8. Rewrite `examples/c/showcases/textured_planet.c`:
   - planet spin uses `dvz_track_rotation()` plus `dvz_anim_visual_transform()`;
   - default camera flyover uses `dvz_track_circle3()` plus `dvz_anim_camera_motion()`;
   - flyover stops on orbit-camera interaction;
   - Mars texture remains required, with no procedural Mars fallback.
9. Sweep scene examples for `dvz_anim_phase()` callbacks that only implement scalar, rotation,
   translation, cursor, slice, or camera motion. Replace them with tracks and adapters.
10. Keep object spin expressed as a rotation track applied through the visual transform adapter,
    with interaction policy handled by `DvzAnimation`. The old `dvz_anim_arcball_spin()` public API
    has been removed from the v0.4 surface.


## Relationship To Other Specs

| Document | Relationship |
|---|---|
| `FRAME_LIFECYCLE.md` | animation update runs before invalidation resolution |
| `INVALIDATION_AND_CACHING.md` | animation writes feed dirty-scope resolution |
| `CONTROLLERS.md` | controllers and animations are distinct scene-state producers |
| `CAMERA_CONTROLLERS.md` | camera motion animation must not fight interactive controllers |
| `core/RUNTIME_BOUNDARY.md` | scene clock consumes runtime wall-clock timestamps |
