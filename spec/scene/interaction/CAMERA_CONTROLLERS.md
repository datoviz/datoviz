# Camera Controllers

This document defines durable scene semantics for 3D camera/navigation controller families:
fly-style navigation, fly pivot gestures, and turntable orbit navigation.

General controller ownership, binding, event routing, and typed state rules are defined in
[CONTROLLERS.md](CONTROLLERS.md) and
[`../decisions/CONTROLLER_BINDING_MODEL.md`](../decisions/CONTROLLER_BINDING_MODEL.md).

## Stack Boundary

Camera controllers are scene interaction features. They do not emit DRP2 commands, own render
targets, or mutate visuals directly.

The intended flow is:

```text
input event or frame tick
  -> scene controller state update
  -> panel camera/model state update
  -> invalidation/redraw request
  -> normal scene FramePlan and DRP2 emission
```

Rendering, frame planning, runtime resources, visual transforms, and DRP2 emission remain unchanged
by the existence of a camera controller.

## Shared Controller Rules

Camera controllers should be scene-owned opaque `DvzController*` handles when implemented under the
accepted controller-binding model. Panels borrow those handles by binding them to `DVZ_DIM_XYZ`.

Rules:

1. controller state is semantic, not backend-specific;
2. input routing is panel-local and supplies viewport/context for each event;
3. shared controller identity synchronizes navigation state across panels;
4. typed POD state get/set APIs are the public inspection and editing surface;
5. derived matrices are not authoritative state;
6. controllers mark panel transform state dirty and request redraw when visible navigation changes;
7. controller state must be instance-scoped, with no file-scope mutable navigation state.

## Fly Controller Semantics

Fly navigation is first-person/free camera navigation through a scene. It updates a panel camera
pose and uses that camera as the final source for view/projection matrices.

The controller owns interactive navigation state such as:

1. position;
2. yaw, pitch, and optional roll;
3. movement mode;
4. speed values;
5. pressed-key state;
6. initial/reset pose;
7. optional pivot state.

Fly movement should be driven from key state and frame `dt`, not OS key repeat cadence.

### Fly Modes

Use one fly controller family with modes rather than multiple first-person controller families.

Recommended modes:

1. `free`: move forward along the full view direction, including pitch;
2. `plane`: keep look pitch but project forward/right movement onto a configured movement plane,
   with vertical motion only through explicit up/down controls.

`free` is useful for volume walkthroughs, 3D meshes, simulation boxes, and debugging spatial data.
`plane` is useful for terrain, microscopy volumes with a preferred depth axis, urban scenes, and
large spatial reconstructions.

### Fly Input Defaults

Recommended first controls:

1. left-drag: yaw/pitch look;
2. `W` or `Up`: move forward;
3. `S` or `Down`: move backward;
4. `A` or `Left`: strafe left;
5. `D` or `Right`: strafe right;
6. `Space`: move up;
7. `Ctrl` or `C`: move down;
8. `Shift`: fast movement;
9. double-click or `R`: reset to initial pose;
10. wheel: move forward/backward or scale speed, chosen explicitly by descriptor or default policy.

Avoid cursor capture in the first slice unless the input layer already has reliable support for
enabling and disabling it. Drag-look is easier to test and matches existing panzoom/arcball input
patterns.

## Fly Pivot Semantics

An orbit pivot is a fly gesture/mode extension, not a separate controller family. It shares the
same camera pose state and lets users temporarily orbit a selected or explicit point while staying
in fly navigation.

Recommended state:

1. `pivot`;
2. `has_pivot`;
3. `pivot_distance`.

Normal fly movement ignores the pivot. Pivot operations add:

1. `look_at_pivot`: reorient toward the pivot without moving;
2. `orbit_pivot`: rotate the camera position around the pivot while looking at it;
3. `dolly_pivot`: move along the camera-pivot line;
4. `clear_pivot`: return to pure fly behavior.

When a pivot is set or changed, the default policy should preserve the camera eye and recompute
orientation from the new pivot:

```text
position stays unchanged
pivot = new world-space point
front = normalize(pivot - position)
yaw/pitch = angles derived from front
pivot_distance = length(pivot - position)
```

This avoids surprising camera jumps when a user selects a new point or object.

## Turntable Controller Semantics

Turntable navigation is stable-up camera orbit navigation for object- and pivot-centered 3D
inspection. It orbits around a fixed pivot on a sphere or spherical cap: horizontal drag rotates
around the stable up axis, vertical drag changes elevation, and wheel changes camera distance.

Turntable is a separate controller family from both arcball and fly:

1. `DvzFly`: first-person/free camera movement through a scene;
2. `DvzFly` with pivot: temporary camera orbit around a selected/picked point while remaining in
   fly navigation;
3. `DvzArcball`: unconstrained model/object rotation;
4. `DvzTurntable`: stable-up camera orbit around a pivot.

The preferred implementation is a scene-owned `DvzController*` family. It may share low-level
helpers with arcball or fly, but it should not be exposed as an arcball option because its state
and camera semantics are different.

## Turntable State

Recommended semantic state:

1. pivot;
2. up vector;
3. distance;
4. yaw;
5. pitch/elevation;
6. yaw, pitch, zoom, and pan speeds;
7. pitch and distance clamps;
8. flags.

Default behavior:

1. fixed-up enabled;
2. pan enabled;
3. roll disabled;
4. yaw wrapping enabled;
5. distance clamping enabled;
6. Y inversion disabled.

Turntable computes camera eye from spherical coordinates around the pivot and targets the pivot.
Panning moves the pivot in the view plane and translates the camera eye by the same delta. A
model-space product-display spin can be added later under a separate name or explicit model-mode
API; it should not blur the primary stable-up camera-orbit contract.

## Turntable Input Defaults

Recommended first controls:

1. left-drag: orbit around pivot, updating yaw and pitch;
2. wheel: dolly by changing distance;
3. middle-drag or right-drag: pan pivot in the view plane when panning is enabled;
4. double-click or `R`: reset to initial pose;
5. explicit API call: set pivot while preserving the current eye, then recompute yaw, pitch, and
   distance from the new pivot.

Roll is not part of the default turntable gesture set. The normal path should remain stable-up and
predictable.

## Pivot Marker Policy

Fly and turntable should share pivot-marker behavior where practical.

The pivot marker is a navigation overlay or annotation aid, not a data visual. It should not affect
scene bounds, picking, visual ordering, color scales, or data export semantics.

Recommended behavior:

1. first implementations may keep pivot state-only if overlay work would broaden the patch too much;
2. a follow-up may show a small crosshair, ring, or point marker transiently after pivot changes;
3. keep the marker visible while an orbit or pivot-pan gesture is active;
4. allow an always-visible/debug option for setup, demos, and diagnostics;
5. hide the marker from screenshots/captures by default unless explicitly enabled.

## Validation Expectations

Focused controller tests should cover:

1. deterministic math helpers before input router integration;
2. reset and initial pose behavior;
3. pitch/elevation clamps avoiding singular or flipped up vectors;
4. free versus planar fly movement;
5. per-frame fly movement from pressed key state and `dt`;
6. turntable orbit preserving distance;
7. turntable pan translating pivot and eye consistently;
8. pivot changes using preserve-eye policy;
9. typed state get/set rejecting wrong controller families;
10. panel binding, resize, and destruction ownership rules once controllers are scene-owned.
