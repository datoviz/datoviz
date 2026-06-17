# Use 3D Controllers

Navigate 3D scenes with arcball, orbit camera, turntable, or fly controls.

## Task Workflow

Choose the controller by interaction style: arcball for object inspection, orbit camera for camera
around a target, turntable for constrained rotation, and fly for free navigation. Bind the
controller to the 3D panel before running the app.

## Minimal Call Sequence

```c
DvzController* controller = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

Use the exact constructor shown by the selected controller example.

## Canonical Examples

- Gallery: [Arcball Controller](../examples/gallery/features/feature_controller_arcball.md)
- Source: `examples/c/features/controller_arcball.c`
- Gallery: [Orbit Camera Controller](../examples/gallery/features/feature_controller_orbit_camera.md)
- Source: `examples/c/features/controller_orbit_camera.c`
- Gallery: [Fly Controller](../examples/gallery/features/feature_controller_fly.md)
- Source: `examples/c/features/controller_fly.c`
- Gallery: [Turntable Controller](../examples/gallery/features/feature_controller_turntable.md)
- Source: `examples/c/features/controller_turntable.c`
- Gallery: [Protein](../examples/gallery/showcases/protein_arcball_viewer.md)
- Source: `examples/c/showcases/protein.c`

## Important Details

3D controllers work best with a clear target, domain, and camera convention. For object viewers,
normalize or center geometry before adding advanced navigation.

For mesh inspection, upload geometry and material attributes first, attach the 3D controller, then
verify depth and lighting. The protein viewer is a composed showcase, not the minimal mesh or
arcball recipe.

## Common Mistakes

- Binding a 2D panzoom controller to a 3D scene.
- Forgetting depth testing or material settings, then debugging the controller.
- Expecting every native 3D controller to have identical WebGPU status.

## See Also

- [Configure cameras](configure-cameras.md)
- [Use lighting and materials](lighting-and-materials.md)
- [Control depth, blending, and transparency](rendering-techniques.md)
