# Scene Turntable Controller Plan

> **Execution Status**
> - **Status:** `PLANNED`
> - **Updated on:** `2026-05-16`
> - **Purpose:** stage a stable-up orbit controller for v0.4 scene panels without mixing it with
>   fly-camera navigation or changing the scene -> frame-plan -> DRP2 runtime contract.


## Goals

Add turntable navigation for object- and pivot-centered 3D inspection. Turntable should feel like a
stable world-up orbit: horizontal drag rotates around the up axis, vertical drag changes elevation,
and wheel or drag changes camera distance. It is useful for mesh, volume, terrain, molecule,
simulation, and other 3D scientific views where users need predictable orientation and a stable
horizon.

Turntable should not be implemented as a DRP2 feature, visual feature, or runtime feature. It is a
scene interaction/controller feature that updates either a camera pose or a model transform before
the existing MVP emission path runs.


## Controller Boundary

Turntable is closer to `arcball` than `fly`.

Use this distinction:

1. `DvzFly`: first-person/free camera movement through a scene.
2. `DvzFly` with pivot: temporary camera orbit around a selected/picked point while remaining in
   fly navigation.
3. `DvzArcball`: unconstrained model/object rotation.
4. `DvzTurntable`: stable-up pivot orbit, either as an arcball mode or as a small sibling
   controller that shares arcball panel plumbing.

The preferred first implementation is an arcball-family controller mode, not a fly option. The
reason is that turntable semantics are pivot-centered inspection semantics, while fly semantics are
position/orientation navigation semantics.


## API Shape

Prefer a descriptor-based API:

```c
typedef enum
{
    DVZ_TURNTABLE_MODE_CAMERA = 0,
    DVZ_TURNTABLE_MODE_MODEL = 1,
} DvzTurntableMode;

typedef struct DvzTurntableDesc
{
    DvzTurntableMode mode;

    vec3 pivot;
    vec3 up;

    float distance;
    float yaw;
    float pitch;

    float yaw_speed;
    float pitch_speed;
    float zoom_speed;
    float pan_speed;

    float min_pitch;
    float max_pitch;
    float min_distance;
    float max_distance;

    int flags;
} DvzTurntableDesc;
```

Public entry points can be either:

```c
DvzTurntableDesc dvz_turntable_desc(void);
DvzTurntable* dvz_panel_set_turntable(
    DvzPanel* panel, DvzInputRouter* router, const DvzTurntableDesc* desc);
DvzTurntable* dvz_panel_turntable(DvzPanel* panel);
```

or, if the implementation lands directly inside `DvzArcball`:

```c
DvzArcballDesc dvz_arcball_desc(void);
void dvz_arcball_set_turntable(DvzArcball* arcball, const DvzTurntableDesc* desc);
```

Keep the first public API narrow. It is acceptable to start with camera-mode turntable only if
model-mode composition would complicate the first patch.


## Flags And Options

Planned flags:

```c
DVZ_TURNTABLE_FLAGS_NONE
DVZ_TURNTABLE_FLAGS_FIXED_UP
DVZ_TURNTABLE_FLAGS_INVERT_Y
DVZ_TURNTABLE_FLAGS_ALLOW_PAN
DVZ_TURNTABLE_FLAGS_ALLOW_ROLL
DVZ_TURNTABLE_FLAGS_WRAP_YAW
DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE
```

Default behavior:

1. `FIXED_UP` enabled.
2. `ALLOW_PAN` enabled.
3. `ALLOW_ROLL` disabled.
4. `WRAP_YAW` enabled.
5. `CLAMP_DISTANCE` enabled.
6. `INVERT_Y` disabled.

Default parameters:

1. `up = {0, 1, 0}`.
2. `pivot = {0, 0, 0}`.
3. `distance = 3`.
4. `yaw = -pi / 2` or the yaw derived from the current camera pose.
5. `pitch = 0`.
6. `min_pitch = -pi / 2 + epsilon`.
7. `max_pitch = +pi / 2 - epsilon`.
8. `min_distance` greater than zero.
9. `max_distance` finite but large enough for scene-scale data.


## Pivot Semantics

The pivot is the world-space point the turntable orbits. It should be explicit and movable.

Camera-mode turntable:

1. State stores `pivot`, `distance`, `yaw`, `pitch`, and `up`.
2. Each update computes the camera eye from spherical coordinates around `pivot`.
3. The camera target is `pivot`.
4. The camera up vector is the configured stable up, corrected as needed near the pitch clamp.
5. Panning moves the pivot in the current view plane and translates the camera eye by the same
   delta, preserving distance and orientation.

Model-mode turntable:

1. State stores `pivot`, `yaw`, `pitch`, and `up`.
2. The model matrix rotates around `pivot`.
3. The camera can stay fixed.
4. This is closer to the current arcball model-matrix role, but pivot handling needs explicit
   translate-to-pivot / rotate / translate-back composition.

Pivot can be moved on demand by:

1. User API: `dvz_turntable_pivot(turntable, pivot)`.
2. Picking: set pivot to a picked point or selected object center.
3. Bounds: set pivot to scene, visual, or selected-object bounds center.
4. View center: set pivot to the point under the panel center when depth/probe information is
   available.

When the pivot changes, there are two valid policies:

1. Preserve eye position and recompute `distance`, `yaw`, and `pitch` from `eye - pivot`.
2. Preserve yaw/pitch/distance and move the eye so it orbits the new pivot immediately.

Default should be policy 1 because selecting a new pivot should not cause the camera to jump.
Expose policy 2 later only if a concrete workflow needs it.


## Pivot Marker

The pivot should be visible only when it helps the user understand navigation state. It should not
be a permanent default visual.

Recommended behavior:

1. First implementation may keep the pivot state-only if overlay work would broaden the patch too
   much.
2. Follow-up implementation should show a small crosshair, ring, or point marker transiently after
   the pivot changes.
3. Keep the marker visible while the user is actively orbiting or panning the pivot.
4. Add an always-visible/debug option for camera setup, demos, and diagnostics.
5. Hide the marker from screenshots/captures by default unless explicitly enabled.

The marker should be an overlay/navigation annotation rather than a normal data visual. It should
not affect scene bounds, picking, visual ordering, color scales, or data export semantics.

Fly and turntable should share the same pivot-marker policy where practical so users get consistent
feedback when changing pivots across controller modes.


## Input Defaults

Initial controls:

1. Left-drag: orbit around pivot, updating yaw and pitch.
2. Wheel: dolly by changing distance.
3. Middle-drag or right-drag: pan pivot in the view plane when `ALLOW_PAN` is enabled.
4. Double-click or `R`: reset to initial pose.
5. `Shift`: faster orbit/pan/dolly.
6. `Ctrl`: slower or fine-grained movement.

Avoid roll by default. If roll is enabled, bind it to an explicit modifier gesture so the normal
turntable path remains stable-up and predictable.


## Stage 1: Math Core

Implement pure deterministic helpers before router integration:

1. Descriptor defaults.
2. Eye-from-pivot spherical conversion.
3. Pose-from-eye-and-pivot conversion.
4. Pitch clamp and yaw wrap.
5. Distance clamp.
6. View-plane pan delta from pixel drag.
7. Optional model-matrix pivot rotation if model mode is included in the first patch.

Focused tests:

1. Default pose looks at pivot with expected distance.
2. Horizontal orbit preserves distance and changes yaw.
3. Vertical orbit clamps pitch and avoids flipping.
4. Dolly clamps distance.
5. Panning translates pivot and eye consistently.
6. Changing pivot with preserve-eye policy does not move the camera eye.


## Stage 2: Public Controller And Panel Ownership

Add the public header and panel ownership once the math tests are stable.

Implementation tasks:

1. Add `include/datoviz/scene/turntable.h` if implemented as a separate controller.
2. Include it from `include/datoviz/scene.h`.
3. Add `DvzTurntable* turntable` to `DvzPanel` only if separate from arcball.
4. Destroy and resize the controller in the same panel lifecycle paths as panzoom/arcball/camera.
5. Ensure camera-mode turntable creates or reuses a panel camera.
6. Keep model-mode turntable compatible with the existing model-matrix controller composition.

Focused tests:

1. Panel creates and returns a turntable.
2. Panel destruction releases it.
3. Resize updates viewport-dependent pan scale.
4. MVP output changes as expected after an orbit.


## Stage 3: Input Router Integration

Add pointer and keyboard handling:

1. Subscribe through `DvzInputRouter`.
2. Use union input events so gesture-derived drag/double-click events are available.
3. Filter pointer events by panel viewport if the controller is panel-owned.
4. Keep interaction state instance-scoped.

Focused tests:

1. Drag inside viewport updates yaw/pitch.
2. Drag outside viewport is ignored.
3. Wheel changes distance.
4. Reset restores initial pose.
5. Pan gesture moves pivot and eye together.


## Stage 4: Pivot Sources

Add helper APIs after base turntable behavior is stable:

1. Set pivot from explicit point.
2. Set pivot from scene or visual bounds.
3. Set pivot from a pick/probe result when available.
4. Optionally expose a user callback for custom pivot resolution.
5. Add transient pivot marker state or a clean extension point for a later overlay marker.

These helpers should remain scene-layer conveniences. The core controller should only know about a
world-space point.


## Stage 5: Examples And Validation

Add an example that makes the difference between arcball, turntable, and fly clear.

Validation checklist:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. Focused controller tests
5. Bounded GLFW smoke if an interactive example is added

The first turntable patch should stay CPU/test-heavy. Vulkan/GLFW validation is only needed once
the controller is wired into an app example or frame loop.
