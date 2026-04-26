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

The export workflow:

1. set clock mode to `offline`, configure `fps` and total `duration`,
2. attach a video sink to the canvas stream (already supported in the canvas architecture),
3. call the frame advance loop — the scene steps `t` by `dt` each iteration, builds a `FramePlan`,
   emits DRP2, submits, and the video sink captures the output,
4. the loop terminates when `t >= duration`.

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


## Deferred Questions

1. the exact public C API for creating and managing animation objects,
2. whether animation objects are first-class public scene handles or sugar-layer conveniences
   wrapping a lower-level callback registration,
3. loop and playback modes beyond the basic active/stopped model (ping-pong, repeat count),
4. whether a scene-level animation player or timeline object is needed for coordinating multiple
   animations in complex scripted sequences.
