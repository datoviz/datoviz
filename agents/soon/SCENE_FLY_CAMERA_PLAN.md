# Scene Fly Camera Plan

> **Execution Status**
> - **Status:** `PLANNED`
> - **Updated on:** `2026-05-16`
> - **Purpose:** stage the v0.4 fly/FPS/pivot camera controller work without changing the
>   scene -> frame-plan -> DRP2 runtime contract.


## Goals

Add a v0.4 scene navigation controller that reuses the existing panel-owned `DvzCamera` as the
single source for view/projection matrices. The controller should own interactive navigation state
and update the camera, while rendering, frame planning, runtime resources, visual transforms, and
DRP2 emission remain unchanged.

The first implementation should port the useful v0.3 `DvzFly` math and API ideas, but it should
not copy the v0.3 event/app wiring directly. v0.4 uses `DvzInputRouter`, panel-owned controllers,
and retained app frames; movement must be driven from key state and frame `dt`, not from OS key
repeat cadence.


## Design Direction

Model the fly controller as a view controller:

```c
panel->camera   /* projection and final view matrix source */
panel->fly      /* input/controller state that updates panel->camera */
```

The scene apply-MVP path should continue to use the camera first, then optional model-space
controllers:

```c
if (panel->camera != NULL)
    dvz_camera_mvp(panel->camera, out);
else if (panel->panzoom != NULL)
    dvz_panzoom_mvp(panel->panzoom, out);
if (panel->arcball != NULL)
    dvz_arcball_mvp(panel->arcball, out);
```

This means `DvzFly` should not emit DRP2 commands, own render targets, or mutate visuals. It should
compute `eye`, `target`, and `up`, then call `dvz_camera_set_view()`.


## v0.3 Reuse

Useful pieces from `v0.3/src/scene/fly.c`:

1. State shape: `position`, `yaw`, `pitch`, `roll`, initial state, and viewport size.
2. Direction math: derive front/right/up from yaw, pitch, roll, and world-up.
3. Pose helpers: `initial()`, `initial_lookat()`, `reset()`, `get_position()`, `get_lookat()`,
   `set_lookat()`, and `get_up()`.
4. Direct motion helpers: `move_forward()`, `move_right()`, `move_up()`, `rotate()`, and `roll()`.
5. Interaction ideas: left-drag look, wheel dolly, double-click reset, and optional pivot orbit.

Do not reuse directly:

1. Direct `calloc()` / `FREE()` calls; use `dvz_calloc()` and `dvz_free()`.
2. Old mouse/keyboard event types; use `DvzPointerEvent`, `DvzKeyboardEvent`, and
   `DvzInputRouter`.
3. Immediate movement on keyboard press/repeat as the primary live path; use pressed-key state plus
   per-frame `dt`.
4. v0.3 transform/app update plumbing.


## Controller Modes

Use one controller family rather than separate controllers for closely related first-person
navigation:

```c
typedef enum
{
    DVZ_FLY_MODE_FREE = 0,
    DVZ_FLY_MODE_PLANE = 1,
} DvzFlyMode;
```

`DVZ_FLY_MODE_FREE` moves forward along the full view direction, including pitch. This is the
classic free-flight camera and is useful for volume walkthroughs, 3D meshes, simulation boxes, and
debugging spatial data.

`DVZ_FLY_MODE_PLANE` keeps look pitch but projects forward/right movement onto a movement plane,
normally XZ with Y-up. Vertical motion occurs only through explicit up/down controls. This is the
scientific-visualization equivalent of FPS navigation and is useful for terrain, LiDAR, microscopy
volumes with a preferred depth axis, urban/building scenes, and large spatial reconstructions.


## Orbit Pivot

An orbit pivot is useful, but it should not be a third independent controller in the first pass.
It is best treated as an optional fly gesture/mode because it shares the same camera pose state and
often needs to coexist with fly navigation.

Recommended behavior:

1. `DvzFly` stores an optional `pivot`, `has_pivot`, and `pivot_distance`.
2. Normal fly movement ignores the pivot. `WASD`, arrows, and mouse-look continue to update the
   camera from position/yaw/pitch as `target = position + front`.
3. A public setter can move the pivot on demand:

   ```c
   void dvz_fly_pivot(DvzFly* fly, vec3 pivot);
   void dvz_fly_clear_pivot(DvzFly* fly);
   bool dvz_fly_has_pivot(const DvzFly* fly);
   void dvz_fly_look_at_pivot(DvzFly* fly);
   ```

4. A later scene-level helper can set the pivot from selection, picked point, panel center,
   visual bounds, or scene bounds.
5. Right-drag or a modifier-drag can orbit around the active pivot while preserving distance.
6. When no pivot is set, orbit can use `position + front * pivot_distance` or be disabled,
   depending on flags.

When a pivot is set or changed, the default policy should preserve the camera eye and recompute
orientation from the new pivot:

```text
position stays unchanged
pivot = new world-space point
front = normalize(pivot - position)
yaw/pitch = angles derived from front
pivot_distance = length(pivot - position)
target = pivot while look-at-pivot or orbit mode is active
```

This avoids a surprising camera jump when the user clicks an object, selects a point, or asks the
controller to orbit a new location. A later optional policy can preserve yaw/pitch/distance instead
and move the eye immediately around the new pivot, but that should not be the default.

The pivot enables extra operations, not a permanent different movement mode:

1. `look_at_pivot`: reorient toward the pivot without moving.
2. `orbit_pivot`: rotate camera position around the pivot while looking at it.
3. `dolly_pivot`: move along the camera-pivot line.
4. `clear_pivot`: return to pure free-fly behavior.

This gives users a hybrid navigation style: fly through the scene, set a pivot from a selected
object or probe point, then orbit that point without switching controllers.


## Pivot Marker

The pivot should be optionally visible, but it should not be shown permanently by default.

Recommended first behavior:

1. Keep the first implementation state-only if overlay plumbing would distract from the controller
   core.
2. Add a transient marker in a follow-up: show a small crosshair or ring for about one second after
   the pivot changes.
3. Keep the marker visible while an orbit-pivot gesture is active.
4. Allow an explicit always-visible/debug mode.
5. Hide the marker for captures/screenshots unless the user explicitly enables it.

The marker should be treated as a navigation overlay or annotation aid, not as a normal data visual.
It should not affect scene bounds, picking, visual ordering, or exported data semantics.


## Turntable

Turntable navigation should not be part of `DvzFly`. It is an object/pivot rotation interaction,
not a free camera translation interaction.

Recommended direction:

1. Keep turntable as an arcball variant or option.
2. Add a dedicated arcball mode/flag later, for example:

   ```c
   DVZ_ARCBALL_FLAGS_TURNTABLE
   DVZ_ARCBALL_FLAGS_FIXED_UP
   ```

3. In turntable mode, horizontal drag rotates around world-up and vertical drag adjusts elevation
   around the pivot, with pitch/elevation clamped to avoid flipping.
4. Keep the existing constrained-axis arcball support separate: constrained arcball locks to one
   arbitrary axis, while turntable is a stable world-up orbit model.

This preserves the distinction:

1. `DvzFly`: camera position/orientation navigation.
2. `DvzFly` with pivot: camera orbit around a selected point as a hybrid camera gesture.
3. `DvzArcball` turntable mode: model/pivot rotation with a stable up direction.


## Input Defaults

Initial controls:

1. Left-drag: yaw/pitch look.
2. `W` or `Up`: move forward.
3. `S` or `Down`: move backward.
4. `A` or `Left`: strafe left.
5. `D` or `Right`: strafe right.
6. `Space`: move up.
7. `Ctrl` or `C`: move down.
8. `Shift`: fast movement.
9. Double-click or `R`: reset to initial pose.
10. Wheel: move forward/backward or scale speed; choose one explicitly in the first patch.

Avoid cursor capture in the first slice unless the window/input layer already has reliable support
for enabling and disabling it. Drag-look is easier to test and matches the current panzoom/arcball
input pattern.


## Options To Reserve

The first API should leave room for:

1. `invert_y` / invert mouse.
2. `fixed_up` to suppress roll and keep a stable horizon.
3. `speed`, `fast_multiplier`, and `slow_multiplier`.
4. `wheel_action`: dolly, speed-scale, or FOV.
5. Optional bounds clamp to keep navigation inside a scene or volume.
6. Optional damping/smoothing, off by default for deterministic tests.
7. Camera bookmarks or named poses.
8. `look_at_selection` / `pivot_from_selection` helpers once picking and selection semantics are
   stable enough for this coupling.


## Stage 1: Core Math And Public Header

Add `include/datoviz/scene/fly.h` and `src/scene/fly.c`.

Implement:

1. `DvzFly` state, flags, mode, viewport size, pose, initial pose, speed values, key-state bits,
   optional pivot, and optional camera pointer.
2. Pose math from v0.3, updated for v0.4 allocation and style rules.
3. Deterministic movement helpers independent from input callbacks.
4. `dvz_fly_apply_camera()` or an equivalent internal helper that writes `eye`, `target`, `up` to
   a `DvzCamera`.

Focused tests:

1. Default pose looks down `-Z`.
2. `initial_lookat()` computes yaw/pitch correctly.
3. Pitch clamp prevents singular up/front vectors.
4. Free movement follows full front.
5. Plane movement projects forward/right onto the configured plane.
6. Reset restores pose and clears transient key/interaction state.


## Stage 2: Input Router Integration

Add:

1. `dvz_fly_pointer()`
2. `dvz_fly_keyboard()`
3. `dvz_fly_connect()`
4. `dvz_fly_disconnect()`
5. `dvz_fly_update(DvzFly* fly, double dt)`

Keyboard handlers should set/release key state. `dvz_fly_update()` should apply movement based on
current key state, speed, modifiers, and `dt`.

Focused tests:

1. WASD and arrow keys set equivalent movement bits.
2. Release clears movement bits.
3. Shift changes speed while pressed.
4. Pointer drag updates yaw/pitch.
5. Double-click or reset key restores the initial pose.


## Stage 3: Panel Ownership

Update scene panel ownership:

1. Add `DvzFly* fly` to `DvzPanel`.
2. Destroy it in `dvz_panel_destroy()`.
3. Resize it in `dvz_figure_resize()`.
4. Add `dvz_panel_set_fly(DvzPanel* panel, DvzInputRouter* router, const DvzFlyDesc* desc)` or a
   comparable descriptor-based API.
5. Add `dvz_panel_fly(DvzPanel* panel)`.
6. Ensure setting fly creates or reuses a panel camera.

The public API should prefer descriptors over a growing argument list.

Focused tests:

1. Panel owns and returns the fly controller.
2. Destroying the panel destroys fly without leaking ownership.
3. Resize updates fly viewport and camera aspect.
4. Fly and camera compose through `_scene_panel_apply_mvp()`.


## Stage 4: App Frame Update

Integrate `dvz_fly_update()` into the app/scene frame path once per frame for panels with an
attached fly controller and camera.

Requirements:

1. Use an app-owned frame delta, clamped to a conservative max to avoid huge jumps after stalls.
2. Mark frame emission dirty or ensure the next emitted MVP reflects the updated camera pose.
3. Keep update state instance-scoped; no file-scope mutable controller state.

Focused tests:

1. Offscreen/app frame loop advances movement while a key is held.
2. Releasing the key stops movement.
3. Runtime reuse does not accumulate controller state across unrelated figures or panels.


## Stage 5: Pivot Orbit

Add optional pivot helpers after the base fly controller is stable.

Implement:

1. `dvz_fly_pivot()`
2. `dvz_fly_clear_pivot()`
3. `dvz_fly_has_pivot()`
4. `dvz_fly_look_at_pivot()`
5. `dvz_fly_orbit()` as a direct math helper
6. Optional pointer gesture for pivot orbit
7. Optional transient pivot marker state, or a clean extension point for a later overlay marker

Focused tests:

1. Orbit preserves distance to pivot.
2. Orbit updates camera look direction toward pivot.
3. Clearing pivot returns to normal fly behavior.
4. Moving the pivot on demand updates subsequent orbit center without affecting current position
   unexpectedly.
5. Setting a new pivot with the preserve-eye policy recomputes yaw, pitch, and pivot distance.
6. Pivot marker state, if included, becomes transiently visible after pivot changes and while
   orbiting.


## Stage 6: Turntable Arcball Follow-Up

Implement turntable as an `arcball` enhancement, not as part of fly.

Planned work:

1. Add an arcball turntable flag or mode.
2. Store turntable yaw/elevation state separately from unconstrained quaternion drag state if this
   keeps the implementation simpler and more stable.
3. Add tests for stable world-up, elevation clamp, reset, and model matrix output.
4. Add an example comparing arcball, turntable, and fly navigation on the same mesh scene.


## Stage 7: Example And Smoke Validation

Add a focused GLFW example, likely `examples/c/hello_fly_glfw.c`, using a mesh or point cloud scene.

Validation checklist:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. Focused input tests if split into a narrower runner
5. Bounded GLFW smoke, for example `./build/examples/c/hello_fly_glfw 60`

For graphics-path validation, prefer the same unsandboxed/`direnv exec .` approach used by existing
Vulkan/GLFW tests when needed.
