# Visual Local Transform And Orbit Camera

> **Status:** active proposal
> **Updated on:** 2026-06-01
> **Scope:** retained visual-local transforms, separate model-arcball and orbit-camera controller
> semantics, and the `textured_planet.c` showcase cleanup.


## Problem

The first `textured_planet.c` showcase needs two different rotations:

1. user arcball navigation should move the camera around the whole scene, so the planet and static
   stars change together;
2. planet self-spin should rotate only the planet, without moving the star field and without
   rewriting mesh vertices every frame.

A temporary CPU-side position/normal update for planet spin proves the visual effect but is the
wrong architecture. It reuploads geometry for a pure transform change and bypasses the scene
transform pipeline.


## Design Direction

Add a retained visual-local transform stage that applies to every visual before panel/controller
viewing:

```text
visual attributes -> visual-local model -> panel/controller transform -> clip space
```

This transform must be stored in scene state, lowered through the normal frame-plan/common MVP path,
and used consistently for rendering, capture, and query/picking paths.

Keep object/model arcball and camera orbit as separate controller concepts, not as target modes on
one public arcball descriptor:

1. `DvzArcball` remains the fixed-camera object/model arcball controller;
2. a new orbit-camera controller, tentatively `DvzOrbitCamera`, uses the same pointer feel but
   changes the panel camera/view transform around a pivot;
3. both controllers may share internal arcball gesture math, but the public semantics stay separate.

This avoids hiding two different ownership models behind one enum. Model/object transforms belong
to visuals. View/projection transforms belong to panels and their camera controllers.


## Public API Candidate

Visual transform:

```c
int dvz_visual_set_transform(DvzVisual* visual, const mat4 transform);
int dvz_visual_clear_transform(DvzVisual* visual);
```

Rules:

1. clear means identity;
2. transform is retained on the visual, not on one panel attachment;
3. all panel attachments of the same visual observe the same local transform;
4. updates mark transform/uniform state dirty, not geometry or attribute buffers dirty;
5. query and picking passes use the same transform;
6. per-panel visual transforms are deferred until there is a concrete multi-panel need.

Orbit-camera controller candidate:

```c
typedef struct DvzOrbitCamera DvzOrbitCamera;

struct DvzOrbitCameraDesc
{
    float width;
    float height;
    int flags;
    vec3 pivot;
};

DvzController* dvz_orbit_camera(DvzScene* scene, const DvzOrbitCameraDesc* desc);
```

Rules:

1. `DvzArcball` changes model/object transform state and does not mutate panel view/projection;
2. `DvzOrbitCamera` changes effective panel view/camera orbit state and does not mutate visual
   model transforms;
3. the first orbit pivot is explicit in the descriptor or defaults to the panel camera target;
4. orbit-camera mode is not a replacement for turntable, fly, or stable-up camera navigation;
5. turntable remains the preferred stable-up orbit controller.


## Visual Family Semantics

The local transform should apply broadly, not only to mesh:

| Family | Effect |
| --- | --- |
| point, pixel, marker | transform anchor position; screen/pixel size stays screen-space |
| segment, path, vector | transform geometric positions/endpoints |
| primitive, mesh, sphere | transform geometry; normals use the transform's linear part |
| image | transform image placement/quad |
| volume/slice | transform proxy/ray domain consistently |
| text, labels | transform anchor in visual/data space; glyph size/layout remains screen-space |
| fixed overlays | apply visual-local transform, then skip controller transform as usual |

This keeps screen-sized visuals ergonomic while still allowing object placement, rotation, and
animation.


## Transform Composition

Object/model arcball:

```text
M = arcball_model * visual_local_model
clip = P * V * M * position
```

Orbit camera:

```text
M = visual_local_model
V = orbit_camera_view * panel_camera_view
clip = P * V * M * position
```

Fixed overlays:

```text
M = visual_local_model
V/P = fixed overlay mapping
```


## Implementation Plan

1. Add visual-local transform storage, set/clear API, dirtying, and common MVP lowering.
2. Make MVP emission effectively per visual/draw where model differs:
   - panel/controller state supplies view/projection;
   - visual state supplies the retained local model;
   - fixed overlays still apply visual-local model while skipping panel controller transforms.
3. Ensure all visual families that consume the shared MVP path observe the transform.
4. Add tests for retained state, emitted non-identity model matrices, clear-to-identity behavior,
   two visuals in one panel with different local transforms, and textured mesh transform support
   without `instance_transform`.
5. Keep `DvzArcball` as the object/model arcball controller, but route its model effect through the
   same per-visual/per-draw model path instead of treating it as view/projection state.
6. Add a new orbit-camera controller, tentatively `DvzOrbitCamera`, sharing internal arcball gesture
   math where practical and composing its result into the effective panel view around the pivot.
7. Update query, picking, bounds, and synthetic query render plans so they use the same visual-local
   model as the render path.
8. Revert or supersede the CPU-vertex planet spin in `textured_planet.c`.
9. Update `textured_planet.c` to use:
   - orbit-camera controller for user navigation;
   - visual-local transform animation for planet self-spin;
   - static world-space star shell;
   - default Phong material without a standard-material switch.
10. Update transform/controller specs after implementation settles.


## Open Decisions

Settled for the next implementation slice:

1. orbit pivot defaults to the panel camera target when no explicit pivot is provided;
2. orbit-camera pan/zoom initially preserve the existing arcball interaction feel;
3. `dvz_visual_bounds()` should account for retained visual-local transforms;
4. visual-local transform composition is `final_model = arcball_model * visual_local_model` for
   object/model arcball and `final_model = visual_local_model` for orbit camera.

Still to settle during implementation:

1. public naming: `DvzOrbitCamera` is the current preference;
2. whether object/model arcball binds to one visual, a selected visual set, or a temporary panel-level
   model scope for migration;
3. exact internal split of shared arcball gesture math versus controller-specific state.


## Validation

Narrow validation before promotion:

```sh
direnv exec . just test scene
direnv exec . just example-c textured_planet 3
git diff --check
```

Focused tests should prove:

1. object/model arcball changes model state, not panel view/projection;
2. orbit-camera changes view/camera state, not visual model matrix;
3. visual-local transform affects rendering through MVP, not attribute reupload;
4. stars remain static while planet self-spin affects only the planet visual;
5. picking/query paths use the same visual-local transform as the render path.
