# Use 3D Controllers

Navigate 3D panels with arcball, turntable, or fly controls.

![Arcball Controller](../assets/gallery/v0.4/features/feature_controller_arcball.webp)

3D navigation is the combination of a camera, a controller, and scene scale. The camera defines the
initial eye, target, up vector, projection, and clipping range. The controller turns mouse, wheel,
keyboard, and gesture input into changes to the panel's retained camera or view state.

Use the scene-owned controller constructors for ordinary v0.4 applications:

```c
DvzController* controller = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

Bind 3D controllers with `DVZ_DIM_MASK_XYZ`. Use `panzoom` only for 2D panels.

## Choose A Controller

Choose the controller by the user's mental model, not by the visual type alone.

| Controller | Best for | Navigation model | Interaction model |
| --- | --- | --- | --- |
| `dvz_arcball()` | Inspecting one centered object such as a mesh, molecule, sphere, volume, or compact point cloud. | Rotates the object/view around a virtual sphere, with optional pan and zoom around the same subject. | Left drag rotates; middle or right drag pans; wheel zooms; double-click resets. |
| `dvz_turntable()` | Upright scenes where roll would be disorienting, such as surfaces, terrain, laboratory coordinates, planets, or CAD-like models. | Orbits around a pivot with constrained yaw and pitch while preserving a world-up convention. | Left drag orbits; middle or right drag pans when enabled; wheel dollies; double-click resets. |
| `dvz_fly()` | Large scenes, walkthroughs, volume interiors, dense point clouds, and data where the camera should move through space. | First-person camera navigation, with pointer look/orbit and keyboard translation. | Left drag looks; right drag strafes/up-down, or orbits when a pivot is set; wheel moves forward/back; `WASD` or arrows move; `Space` moves up; `Ctrl` or `C` moves down; `Shift` moves faster; `R` resets. |

Use arcball as the default for a 3D object viewer. Use turntable when the scene has a meaningful
vertical axis. Use fly when translation through the scene is part of the task.

## Camera First

Set a camera before binding a controller when the initial viewpoint matters. A good camera makes
controller behavior predictable and keeps depth precision stable.

```c
DvzCameraDesc camera = dvz_camera_desc();

camera.view.eye[0] = 0.0f;
camera.view.eye[1] = 2.0f;
camera.view.eye[2] = 4.0f;

camera.view.target[0] = 0.0f;
camera.view.target[1] = 0.0f;
camera.view.target[2] = 0.0f;

camera.view.up[0] = 0.0f;
camera.view.up[1] = 1.0f;
camera.view.up[2] = 0.0f;

camera.projection.fov_y = 0.66f;
camera.projection.near_clip = 0.05f;
camera.projection.far_clip = 100.0f;

dvz_panel_set_camera_desc(panel, &camera);
```

Keep the camera convention consistent with labels, grids, lighting, picking, and any orientation
gizmo. For most examples in this documentation, `Y` is up, the camera looks toward the origin, and
the data is centered near the origin.

## Arcball Object Inspection

Arcball is best when the user wants to inspect an object from arbitrary directions. It allows roll,
so it is useful for molecules, anatomical meshes, isolated geometry, and compact point clouds where
the object is more important than a fixed horizon.

```c
// Add visuals first: mesh, sphere, volume, or a compact 3D point cloud.
// Then attach object-inspection navigation.
DvzController* controller = dvz_arcball(scene, NULL);
if (controller == NULL)
    return false;

if (dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) != 0)
    return false;
```

Use a centered object for the first version of an arcball view. If imported geometry is far from the
origin, either normalize the data or set a camera target that matches the object's center before
binding the controller.

Arcball is also the simplest controller to share across comparable panels:

```c
DvzController* shared = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(left_panel, shared, DVZ_DIM_MASK_XYZ);
dvz_panel_bind_controller(right_panel, shared, DVZ_DIM_MASK_XYZ);
```

Shared controllers link navigation state. Use separate controller handles when panels should move
independently.

## Turntable For Upright Scenes

Turntable navigation keeps an upright feel. It is usually the right choice for surfaces, coordinate
systems, instrument views, map-like 3D scenes, and any view where rolling the camera makes axes hard
to read.

```c
DvzTurntableDesc desc = dvz_turntable_desc();
desc.initial_view = camera.view;
desc.controller_flags = DVZ_TURNTABLE_FLAGS_CLAMP_DISTANCE;
desc.min_pitch = -0.72f;
desc.max_pitch = +0.72f;
desc.min_distance = 2.0f;
desc.max_distance = 8.0f;

DvzController* controller = dvz_turntable(scene, &desc);
if (controller == NULL)
    return false;

if (dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) != 0)
    return false;
```

The `initial_view` should match the panel camera. Clamp pitch and distance when the object has a
preferred viewing range, or when moving inside the object would be confusing.

## Fly For Large 3D Data

Fly navigation moves the camera through the world. It is more sensitive to scale than arcball or
turntable because speed is expressed in scene units.

```c
DvzFlyDesc desc = dvz_fly_desc();
desc.mode = DVZ_FLY_MODE_PLANE;
desc.initial_view = camera.view;
desc.speed = 0.70f;
desc.fast_multiplier = 4.0f;
desc.slow_multiplier = 0.25f;
desc.look_speed = 0.45f;
desc.wheel_speed = 0.60f;

DvzController* controller = dvz_fly(scene, &desc);
if (controller == NULL)
    return false;

if (dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ) != 0)
    return false;
```

Use `DVZ_FLY_MODE_PLANE` for scenes with a ground plane or vertical axis. Use the free mode when the
camera should move in all directions without preserving a ground-plane convention. Tune `speed`,
`look_speed`, and `wheel_speed` after choosing the camera and clipping range; one wheel tick or one
second of keyboard motion should move a useful distance, not skip across the dataset.

## Scene Scale And Clipping

Controller tuning starts with scene scale:

| Scene extent | Camera distance | Near/far clipping | Controller tuning |
| --- | --- | --- | --- |
| Compact object around `[-1, +1]` | Eye roughly 3 to 5 units from target. | `near_clip` around `0.01` to `0.05`; `far_clip` around `50` to `100`. | Defaults are usually close. |
| Medium scientific volume or mesh | Eye several object radii from target. | Keep `near_clip` as large as acceptable; set `far_clip` just beyond the data. | Increase fly speed and wheel speed in proportion to the data extent. |
| Large point cloud or spatial scene | Eye and speeds in the same unit system as the data. | Avoid a needlessly huge `far_clip / near_clip` ratio. | Prefer fly or turntable with explicit speed/distance limits. |

Do not hide scale problems in the controller. If a scene is imported in microns, meters, or voxel
indices, decide whether the displayed coordinates should stay in those units or be normalized for
inspection.

## Depth, Lighting, And Orientation

Navigation changes only view state. It does not fix render ordering, lighting, material, or depth
setup.

For opaque 3D visuals:

- Enable the visual or technique settings needed for depth testing.
- Use materials and normals for meshes where orientation should be readable.
- Add a reference grid, axes, or orientation gizmo when the scene has a stable world frame.
- Verify clipping before debugging controller motion.

For transparent 3D visuals, read the depth and blending documentation before assuming navigation is
the issue. Transparent geometry may need a dedicated transparency technique or draw-order policy.

## Multi-Panel Navigation

A controller is a retained scene object. Binding the same controller to several panels links the
state of those panels; creating several controllers gives each panel independent navigation.

```c
// Linked 3D inspection.
DvzController* linked = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(panel_a, linked, DVZ_DIM_MASK_XYZ);
dvz_panel_bind_controller(panel_b, linked, DVZ_DIM_MASK_XYZ);

// Independent 3D inspection.
DvzController* controller_a = dvz_arcball(scene, NULL);
DvzController* controller_b = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(panel_a, controller_a, DVZ_DIM_MASK_XYZ);
dvz_panel_bind_controller(panel_b, controller_b, DVZ_DIM_MASK_XYZ);
```

Use the same camera descriptor for linked panels unless the difference is intentional. If cameras
start from different targets or up vectors, shared interaction can feel inconsistent.

## WebGPU Status

The v0.4 WebGPU/WASM path supports promoted controller examples, but it is not full native parity.
Do not assume every native controller configuration, visual family, query target, or host input
path is available in the browser.

For public examples, use the manifest-backed gallery route status. For application code, keep the
scene contract shared and treat browser JavaScript as host glue, not a second implementation of the
navigation semantics.

## Common Mistakes

- Binding `dvz_panzoom()` to a 3D panel instead of a 3D controller.
- Binding a 3D controller with `DVZ_DIM_MASK_XY` instead of `DVZ_DIM_MASK_XYZ`.
- Setting the camera target far from the data center, then interpreting orbit motion as broken.
- Using a very small `near_clip` and very large `far_clip`, then seeing depth artifacts.
- Tuning fly speed before deciding the scene's displayed unit scale.
- Expecting turntable behavior from arcball; arcball can roll by design.
- Sharing one controller between panels that should navigate independently.
- Expecting every native 3D controller configuration to be live in WebGPU.

## Troubleshooting

| Symptom | Likely cause | Check |
| --- | --- | --- |
| Dragging appears to do nothing. | Controller was not bound, or was bound to the wrong dimensions. | Verify `dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ)` returns `0`. |
| Object disappears while zooming or flying. | Camera clipping range is too narrow or badly scaled. | Adjust `near_clip` and `far_clip` around the actual data extent. |
| Rotation feels off-center. | Camera target or object center does not match the intended pivot. | Center the data or set the camera target to the object center. |
| Surface turns upside down. | Arcball was used where a world-up constraint was needed. | Use `dvz_turntable()` for upright scenes. |
| Fly jumps too far. | Movement speed is too large for the data units. | Reduce `speed` and `wheel_speed`, or normalize the scene. |
| 3D shape looks flat or unordered. | Depth, normals, material, or transparency setup is missing. | Check visual depth settings and material/lighting configuration. |

## Validate The Result

For a native example, build and run the narrow controller example first:

```sh
just example-c features/controller_arcball
./build/examples/c/features/controller_arcball --live
```

Then compare behavior with:

```sh
just example-c features/controller_turntable
./build/examples/c/features/controller_turntable --live

just example-c features/controller_fly
./build/examples/c/features/controller_fly --live
```

Use `--png` for a non-interactive smoke run, then use `--live` to validate the actual input model.

## See Also

- [Configure cameras](configure-cameras.md)
- [Use coordinate systems](coordinate-systems.md)
- [Use lighting and materials](lighting-and-materials.md)
- [Control depth, blending, and transparency](depth-blending.md)
- [Controllers](../reference/controllers.md)

??? example "Related examples"

    - [Arcball Controller](../examples/gallery/features/feature_controller_arcball.md) - Source: `examples/c/features/controller_arcball.c`
    - [Fly Controller](../examples/gallery/features/feature_controller_fly.md) - Source: `examples/c/features/controller_fly.c`
    - [Turntable Controller](../examples/gallery/features/feature_controller_turntable.md) - Source: `examples/c/features/controller_turntable.c`
    - [Orientation Gizmo](../examples/gallery/features/feature_orientation_gizmo.md) - Source: `examples/c/features/orientation_gizmo.c`
    - [Protein](../examples/gallery/showcases/protein_arcball_viewer.md) - Source: `examples/c/showcases/protein.c`
