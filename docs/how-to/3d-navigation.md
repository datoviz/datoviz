# Use 3D Controllers

Navigate 3D scenes with arcball, orbit camera, turntable, or fly controls.

## Task Workflow

Choose the controller by interaction style: arcball for object inspection, orbit camera for camera
around a target, turntable for constrained rotation, and fly for free navigation. Bind the
controller to the 3D panel before running the app.

| Controller | Best for | Navigation model | Interaction model |
| --- | --- | --- | --- |
| `dvz_arcball()` | Inspecting one centered object such as a mesh, molecule, sphere, or compact point cloud. | Rotates the object/view around a virtual sphere, with optional pan and zoom around the same subject. | Left drag rotates; middle or right drag pans; wheel zooms; double-click resets. |
| `dvz_turntable()` | Object inspection where roll would be disorienting, such as terrain, surfaces, and upright models. | Orbits around a target with constrained yaw and pitch, preserving an up-oriented feel. | Left drag orbits; middle or right drag pans; wheel dollies; double-click resets. |
| `dvz_orbit_camera()` | Scenes where camera position, target, and distance are the user-facing state. | Moves the camera around a pivot while keeping the target/pivot explicit. | Left drag orbits; middle or right drag pans; wheel changes camera distance; double-click resets. |
| `dvz_fly()` | Large 3D scenes, walkthroughs, volume exploration, and views where the camera moves through space. | First-person camera navigation, with pointer look/orbit and keyboard translation. | Left drag looks; right drag strafes/up-down, or orbits when a pivot is set; wheel moves forward/back; `WASD` or arrows move; `Space` moves up; `Ctrl` or `C` moves down; `Shift` moves faster; `R` resets. |

Arcball is the default choice for a 3D object viewer. Turntable is the conservative choice when
the scene has a natural vertical direction. Orbit camera is useful when application UI needs to
show or edit target, distance, or camera vectors. Fly is the broadest navigation model, but it also
requires a scene scale and speed that feel right for the data.

## Minimal Call Sequence

```c
DvzController* controller = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

Use the exact constructor and descriptor shown by the selected controller example when you need
controller-specific speed, target, camera, or pivot setup.


## Important Details

3D controllers work best with a clear target, domain, and camera convention. For object viewers,
normalize or center geometry before adding advanced navigation.

For mesh inspection, upload geometry and material attributes first, attach the 3D controller, then
verify depth and lighting. The protein viewer is a composed showcase, not the minimal mesh or
arcball recipe.

Fly navigation depends more strongly on scene scale than the inspection controllers. Set the
initial camera and movement speeds so one wheel tick or one second of keyboard movement covers a
useful distance in the dataset.

## Common Mistakes

- Binding a 2D panzoom controller to a 3D scene.
- Forgetting depth testing or material settings, then debugging the controller.
- Expecting every native 3D controller to have identical WebGPU status.

## See Also

- [Configure cameras](configure-cameras.md)
- [Use lighting and materials](lighting-and-materials.md)
- [Control depth, blending, and transparency](rendering-techniques.md)

??? example "Related examples"

    - [Arcball Controller](../examples/gallery/features/feature_controller_arcball.md) - Source: `examples/c/features/controller_arcball.c`
    - [Orbit Camera Controller](../examples/gallery/features/feature_controller_orbit_camera.md) - Source: `examples/c/features/controller_orbit_camera.c`
    - [Fly Controller](../examples/gallery/features/feature_controller_fly.md) - Source: `examples/c/features/controller_fly.c`
    - [Turntable Controller](../examples/gallery/features/feature_controller_turntable.md) - Source: `examples/c/features/controller_turntable.c`
    - [Protein](../examples/gallery/showcases/protein_arcball_viewer.md) - Source: `examples/c/showcases/protein.c`
