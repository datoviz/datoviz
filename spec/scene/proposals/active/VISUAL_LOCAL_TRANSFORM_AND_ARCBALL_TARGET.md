# Visual Local Transform And Arcball Target

> **Status:** active proposal
> **Updated on:** 2026-06-01
> **Scope:** retained visual-local transforms, arcball model/camera target semantics, and the
> `textured_planet.c` showcase cleanup.


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

Add an explicit arcball target mode:

```c
typedef enum DvzArcballTarget
{
    DVZ_ARCBALL_TARGET_MODEL = 0,
    DVZ_ARCBALL_TARGET_CAMERA_ORBIT = 1,
} DvzArcballTarget;
```

Default remains `MODEL` so existing examples and tests keep current behavior. `CAMERA_ORBIT` is an
opt-in mode for scenes such as textured planets where the arcball represents camera movement around
a pivot instead of object/model rotation.


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

Arcball target:

```c
struct DvzArcballDesc
{
    float width;
    float height;
    int flags;
    DvzArcballTarget target;
};
```

Rules:

1. `DVZ_ARCBALL_TARGET_MODEL` preserves the current model-matrix arcball behavior;
2. `DVZ_ARCBALL_TARGET_CAMERA_ORBIT` changes the panel view/camera orbit instead of the model
   matrix;
3. camera-orbit mode is not a replacement for turntable, fly, or stable-up camera navigation;
4. turntable remains the preferred stable-up orbit controller.


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

Model-target arcball:

```text
M = arcball_model * visual_local_model
clip = P * V * M * position
```

Camera-orbit arcball:

```text
M = visual_local_model
V = arcball_camera_view * panel_camera_view
clip = P * V * M * position
```

Fixed overlays:

```text
M = visual_local_model
V/P = fixed overlay mapping
```


## Implementation Plan

1. Revert or supersede the CPU-vertex planet spin in `textured_planet.c`.
2. Add visual-local transform storage, set/clear API, dirtying, and common MVP lowering.
3. Ensure all visual families that consume the shared MVP path observe the transform.
4. Add tests for retained state, emitted non-identity model matrices, clear-to-identity behavior,
   and textured mesh transform support without `instance_transform`.
5. Add `DvzArcballTarget` to the arcball descriptor, defaulting to model target.
6. Implement camera-orbit arcball composition at the panel/controller transform stage.
7. Update `textured_planet.c` to use:
   - camera-orbit arcball for user navigation;
   - visual-local transform animation for planet self-spin;
   - static world-space star shell;
   - default Phong material without a standard-material switch.
8. Update transform/controller specs after implementation settles.


## Validation

Narrow validation before promotion:

```sh
direnv exec . just test scene
direnv exec . just example-c textured_planet 3
git diff --check
```

Focused tests should prove:

1. default arcball mode remains model-target;
2. camera-orbit arcball changes view/camera state, not the visual model matrix;
3. visual-local transform affects rendering through MVP, not attribute reupload;
4. stars remain static while planet self-spin affects only the planet visual;
5. picking/query paths use the same visual-local transform as the render path.
