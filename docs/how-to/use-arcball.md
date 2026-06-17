# Use Arcball

Arcball is now covered by the broader 3D controller guide.

## Task Workflow

Use arcball for object-centered inspection. Center and scale the object, attach the arcball
controller, then verify depth and lighting.

## Minimal Call Sequence

```c
DvzController* controller = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

## Canonical Examples

- Gallery: [Arcball Controller](../examples/gallery/features/feature_controller_arcball.md)
- Source: `examples/c/features/controller_arcball.c`
- Gallery: [Protein](../examples/gallery/showcases/protein_arcball_viewer.md)
- Source: `examples/c/showcases/protein.c`

## Important Details

Use arcball for inspecting an object around a target. Use orbit camera, turntable, or fly when those
interaction models better match the task.

## Common Mistakes

- Applying arcball to uncentered geometry and interpreting the result as a camera bug.
- Using arcball where 2D panzoom is the intended controller.
- Assuming the protein showcase is the minimal arcball example.

## See Also

- [Use 3D controllers](3d-navigation.md)
- [Configure cameras](configure-cameras.md)
- [Mesh with arcball navigation](mesh-arcball.md)
