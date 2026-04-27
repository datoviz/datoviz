# Scene Animation And Frame Scheduling

This document defines the scene-level animation model and frame scheduling strategy for Datoviz
v0.4.


## Purpose

The animation system serves two distinct but related goals:

1. **Interactive animations** — scene properties evolve over time in response to a running clock,
   for example live data streaming, smooth transitions, or timer-driven updates.
2. **Scripted video export** — a pre-authored animation is rendered frame by frame at a controlled
   rate, decoupled from wall-clock time, to produce a video file.

Both goals share the same scene clock and animation primitives.
They differ only in how the clock advances.


## Position

The animation system sits:

1. inside the scene layer, above DRP2,
2. before invalidation resolution in the frame lifecycle,
3. alongside controllers — both feed the invalidation system but are distinct concepts.

Animations are scene-owned objects.
They are not runtime concepts and do not require DRP2-level support.


## Core Rule

Animations evolve scene-owned properties over time.
Controllers respond to external interaction events.
Both mark the scene dirty when they produce a change.
Both feed the same invalidation and redraw pipeline.

The scene clock is the single source of time for all animations.
It may run in real-time or offline mode, but it is always the scene's own abstraction — not a raw
platform timer.


## Scene Clock

The scene has an internal clock that tracks the current animation time `t`.

The runtime provides a wall-clock timestamp at the start of each frame.
The scene clock decides what to do with it based on its current mode.


### Real-Time Mode

The clock advances with wall-clock time.

```
t_scene += t_wall_delta
```

Used for interactive sessions, live data streaming, and responsive transitions.
The frame rate is governed by the rendering loop and vsync policy.


### Offline Mode

The clock advances by a fixed `dt = 1 / fps` per frame, regardless of wall-clock time.

```
t_scene += dt   # fixed step, e.g. 1/60
```

Used for scripted video export.
The scene renders as fast as the GPU allows — no vsync, no frame rate cap.
A 30-second animation at 60 fps produces exactly 1800 frames in however long the GPU takes.

The offline clock is deterministic: given the same initial state and the same sequence of fixed
steps, the output is always identical.


### Clock Parameters

| Parameter | Description |
|---|---|
| `mode` | `realtime` or `offline` |
| `fps` | frames per second, used for `dt` in offline mode and for timer period resolution |
| `t` | current animation time, in seconds |
| `t_start` | time origin, default `0.0` |


## Animation Objects

An animation is a scene-owned object that drives one or more scene properties as a function of
time.

An animation has:

1. a start time `t_start` within the scene clock,
2. an optional end time `t_end` (open-ended animations run until explicitly stopped),
3. a normalized time `u = (t - t_start) / duration` in `[0, 1]` during the active window,
4. an optional easing function applied to `u`,
5. an update callback or property binding that receives `u` and writes new values to scene state.

When an animation is active it marks the scene dirty every frame it produces a non-trivial update.
When all animations are inactive and no interaction is pending, no redraw is triggered.


## Animation Handles

Animations are first-class scene objects with stable handles.

This means:

1. an animation is created and owned by the scene,
2. it is returned as an opaque handle (`DvzAnimation*`),
3. the caller may start, stop, or destroy it through that handle,
4. the scene owns the lifecycle and destroys all animations when the scene is destroyed.

This is the same ownership model as controllers (`DvzController*`).
The handle is required for the caller to stop an open-ended animation, retrieve its state, or
cancel it before the scene is destroyed.


## Animation Construction

Three type-specific constructors cover the primary animation use cases.

```text
// Timer callback: fires every frame while active
// callback signature: void cb(DvzAnimation*, double t, void* user_data)
anim = dvz_anim_timer(scene, period_s, callback, user_data)

// 2-point property transition: interpolates from_val to to_val over duration seconds
// target is a typed descriptor identifying the scene property to animate
anim = dvz_anim_transition(scene, &(DvzAnimTransitionDesc){
    .target       = target_desc,      // identifies the property (see below)
    .from         = from_val,
    .to           = to_val,
    .duration     = 2.0,
    .easing       = DVZ_EASING_EASE_OUT,
    .loop         = DVZ_LOOP_ONCE,    // see Loop Modes below
    .repeat_count = 0,                // ignored for DVZ_LOOP_ONCE
})

// Camera path: animates a panel's camera through an ordered list of keyframes
anim = dvz_anim_camera_path(scene, panel, &(DvzAnimCameraPathDesc){
    .keyframes    = keyframes,
    .n_keyframes  = n_keyframes,
    .loop         = DVZ_LOOP_ONCE,
    .repeat_count = 0,
})
```

A generic `dvz_anim_create(scene, type, &desc)` may exist internally but is not the
user-facing default.

**Property target descriptors** — `DvzAnimTarget` identifies what a transition animates.
For the first spec the supported targets are:

1. a named visual parameter (e.g. `{.visual = v, .param = DVZ_PARAM_ALPHA}`),
2. a panzoom domain bound (e.g. `{.controller = panzoom, .dim = DVZ_DIM_X, .bound = DVZ_BOUND_MAX}`),
3. the scene clock's own time offset (for scripted synchronization).

General property-path expressions or multi-track keyframe curves are explicitly out of scope.
The 2-point transition model covers non-camera property animation.


## Loop Modes

Finite animations (those with a `duration`) support three loop modes declared at construction:

| Value | Behavior |
|---|---|
| `DVZ_LOOP_ONCE` | plays once then stops; default |
| `DVZ_LOOP_REPEAT` | restarts from the beginning each cycle |
| `DVZ_LOOP_PINGPONG` | reverses direction on each cycle |

`repeat_count` controls how many cycles run before the animation stops automatically.
`0` means infinite — the animation runs until `dvz_anim_stop()` or `dvz_anim_destroy()`.

For `DVZ_LOOP_PINGPONG` the normalized time `u` runs `0→1` on odd cycles and `1→0` on even
cycles. The easing function is applied to the reversed `u` as well, so the curve is mirrored.

Timer callbacks do not use loop modes — they are inherently open-ended and are stopped
explicitly.


## Animation Lifecycle

```text
dvz_anim_start(anim, t_start)   // schedule start at scene-clock time t_start; 0 = immediate
dvz_anim_stop(anim)             // deactivate; retains handle for restart or inspection
dvz_anim_destroy(anim)          // release resources; handle becomes invalid
```

An animation that reaches `t_end` stops itself automatically.
An open-ended timer animation (no `t_end`) runs until `dvz_anim_stop()` or `dvz_anim_destroy()`.

Stopping an animation does not revert scene state — the last value written remains.

The scene clock controls when animations advance; individual animations do not have their own
clocks.


## Resolved C API Decisions

**Handles** — animations are first-class `DvzAnimation*` handles, not sugar-layer callbacks.
The handle is required to stop or destroy open-ended animations.

**Constructors** — type-specific: `dvz_anim_timer()`, `dvz_anim_transition()`,
`dvz_anim_camera_path()`. Each constructor takes a descriptor struct or explicit parameters
matching the animation type.

**Property targets** — typed `DvzAnimTarget` descriptor rather than a string property path.
Targets are limited to named visual parameters, controller domain bounds, and the clock offset
for the initial spec. A general property-path system is not introduced.

**Lifecycle** — `dvz_anim_start()` / `dvz_anim_stop()` / `dvz_anim_destroy()` with clear
semantics: stop retains state, destroy releases resources.

**Loop and playback modes** — resolved. `DVZ_LOOP_ONCE` / `DVZ_LOOP_REPEAT` / `DVZ_LOOP_PINGPONG`
declared at construction; `repeat_count = 0` means infinite. See Loop Modes section.

**Scene-level timeline** — deferred. Coordinating multiple animations by setting their
`t_start` values explicitly is sufficient for the initial spec.


### Easing Functions

Easing functions map normalized time `u ∈ [0, 1]` to a shaped `u' ∈ [0, 1]`.

The minimum required set:

| Name | Shape |
|---|---|
| `linear` | identity |
| `ease_in` | slow start, fast end |
| `ease_out` | fast start, slow end |
| `ease_in_out` | slow start, slow end, fast middle |
| `cubic_in`, `cubic_out`, `cubic_in_out` | cubic variants of the above |

Additional easing functions may be added without breaking the contract.

Easing functions apply to **2-point property transitions**: a start value, an end value, a
duration, and a curve.
This is effectively a 2-keyframe system and covers the majority of non-camera animation needs
without requiring a general multi-track timeline.

Example: fade an opacity from `1.0` to `0.0` over `2.0` seconds with `ease_out`.


### Timer Callbacks

A timer callback is the lowest-level animation primitive.

The user registers a callback that fires every frame (or every N frames) while the animation is
active.
The callback receives the current clock time `t` and is responsible for updating scene state
directly.

This is the v0.3 `@app.timer(period=1/60)` model, preserved as a sugar-layer convenience.
In v0.4 the timer is registered against the scene clock rather than the app event loop, but the
user-facing pattern is unchanged.

Timer callbacks are appropriate when:

1. the update logic is data-driven rather than property-interpolation-driven,
2. the user wants full control over what happens each frame,
3. the animation is open-ended (live streaming, real-time data).

Example: streaming spike data, updating positions from a buffer every frame.


## Camera Path Keyframes

Camera path animation is a first-class animation type.

A camera path is defined as an ordered list of keyframes:

| Field | Description |
|---|---|
| `t` | time in seconds |
| `position` | camera position, `vec3` |
| `target` | look-at target or orientation, `vec3` or quaternion |
| `up` | up vector, `vec3`, default `(0, 1, 0)` |

The scene interpolates between adjacent keyframes:

1. **position**: linear interpolation (`lerp`),
2. **orientation**: spherical linear interpolation (`slerp`) when expressed as a quaternion,
3. **easing**: an optional per-segment easing function may be applied to the normalized segment
   time.

Camera path keyframes are the primary authoring primitive for scripted fly-throughs in video
export.
They are more ergonomic than writing a parametric camera function for complex 3D paths.

General multi-track keyframe animation (animating arbitrary scene properties with per-keyframe
curve handles) is explicitly out of scope.
Easing-based 2-point transitions cover that need for non-camera properties.


## Redraw Scheduling

The scene renders on demand.
It does not render continuously when nothing is changing.

An active animation triggers redraws by marking the scene dirty each frame it produces a change.
This integrates with the existing invalidation system in `INVALIDATION_AND_CACHING.md` — an
animation update is just a source of scene-state change like any other.

Redraw scheduling rules:

1. if at least one animation is active, the scene requests a redraw for the next frame,
2. if no animations are active and no interaction is pending, no redraw is scheduled,
3. in offline mode, the scene always advances by one step and renders — the "idle" concept does
   not apply.


## Video Export

Offline clock mode is the foundation for GPU-accelerated video export.

The export workflow, from the application's perspective:

1. application attaches a video sink to the canvas stream — this is a canvas-level operation,
   not a scene concern,
2. application sets the scene clock to `offline` mode and configures `fps` and total `duration`,
3. application drives the frame loop — each iteration the scene steps `t` by `dt`, builds a
   `FramePlan`, emits DRP2, and the DRP2 runtime submits; the video sink captures the output,
4. the loop terminates when `t >= duration`.

The scene's only responsibility in this workflow is the clock and the `FramePlan`.
Everything about canvas, sinks, and capture is the application's job.

The user authors the animation through any combination of:

1. timer callbacks that update visual data each frame,
2. easing-based property transitions,
3. camera path keyframes.

The offline clock ensures the output is deterministic regardless of GPU speed.

Frame rate and duration are the only required export parameters.
Output format and codec are handled by the video sink layer, not the scene layer.


## Relationship To Controllers

Controllers and animations are distinct but feed the same pipeline:

| Concept | Driven by | Typical lifetime |
|---|---|---|
| Controller | external input event (mouse, keyboard) | persistent, panel-owned |
| Animation | scene clock | bounded or open-ended |

Both produce scene-state changes.
Both mark the scene dirty.
Both are updated in the frame lifecycle before invalidation resolution.

A controller may trigger an animation — for example, a double-click launches a smooth zoom
transition driven by an easing animation — but the animation then runs independently of further
input.


## Frame Lifecycle Integration

Animation update is step 3 of the frame lifecycle defined in `FRAME_LIFECYCLE.md`:

```
1. ingest events
2. update controllers
3. update animations        ← here
4. resolve invalidation
5. validate
6. capability adaptation
7. build FramePlan
8. emit DRP2
...
```

Animation update must complete before invalidation resolution so that any scene-state changes
produced by animations are included in the current frame's dirty scope.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `FRAME_LIFECYCLE.md` | animation update is step 3 of the frame flow |
| `INVALIDATION_AND_CACHING.md` | animation changes feed the dirty-scope resolution |
| `CONTROLLERS.md` | controllers and animations are distinct but both mark scene dirty |
| `REQUIREMENTS.md` | animation and scheduling listed as scene-owned responsibilities |
| `RUNTIME_SERVICE_SKETCH.md` | runtime provides wall-clock timestamp; scene clock consumes it |
