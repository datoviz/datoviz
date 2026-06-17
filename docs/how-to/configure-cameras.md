# Configure Cameras

Choose the view model for 2D and 3D panels.

## Task Workflow

Use panel domains and panzoom for 2D data. Use a 3D controller and camera-oriented example for
object or volume scenes. Keep camera state separate from mesh data so navigation does not rewrite
geometry.

## Minimal Call Sequence

```c
DvzController* controller = dvz_orbit_camera(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

Use the constructor and camera descriptor shown by the selected controller example.


## Important Details

Camera conventions affect depth, lighting, and picking. Confirm the scene's axis convention before
adding orientation UI or scientific labels.

## Common Mistakes

- Using visual transforms as camera transforms.
- Combining 2D domain navigation and 3D camera navigation on the same panel.
- Forgetting to center or scale imported geometry before attaching a 3D controller.

## See Also

- [Use 3D controllers](3d-navigation.md)
- [Use coordinate systems](coordinate-systems.md)
- [Use lighting and materials](lighting-and-materials.md)

??? example "Related examples"

    - [Orbit Camera Controller](../examples/gallery/features/feature_controller_orbit_camera.md) - Source: `examples/c/features/controller_orbit_camera.c`
    - [Arcball Controller](../examples/gallery/features/feature_controller_arcball.md) - Source: `examples/c/features/controller_arcball.c`
    - [Orientation Gizmo](../examples/gallery/features/feature_orientation_gizmo.md) - Source: `examples/c/features/orientation_gizmo.c`
