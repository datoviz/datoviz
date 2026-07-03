# Configure Cameras

Set the view and projection conventions for 3D panels.

## Task Workflow

Use panel domains and panzoom for 2D data. For 3D data, configure a panel camera or use a 3D
controller that updates camera/view state. Keep camera state separate from mesh data so navigation
does not rewrite geometry.

Camera setup answers different questions than controller selection:

| Question | Page |
| --- | --- |
| Which interaction model should the user get: arcball, turntable, orbit, or fly? | [Use 3D controllers](3d-navigation.md) |
| Where is the eye, what is it looking at, what is up, and how is depth projected? | This page |

A camera descriptor defines the initial view and projection:

| Field | Meaning |
| --- | --- |
| `eye` | Camera position in scene coordinates. |
| `target` | Point the camera looks at. |
| `up` | Direction treated as vertical for the view. Use a consistent world-up convention across the scene. |
| `type` | Perspective or orthographic projection. |
| `fov_y` | Vertical field of view for perspective cameras, in radians. |
| `ortho_height` | Visible height for orthographic cameras. |
| `near`, `far` | Clipping planes. Choose values that enclose the data without making the depth range needlessly large. |

## Minimal Call Sequence

```c
DvzCameraDesc camera = dvz_camera_desc();
camera.eye[0] = 0.0f;
camera.eye[1] = 1.5f;
camera.eye[2] = 4.0f;
camera.target[0] = 0.0f;
camera.target[1] = 0.0f;
camera.target[2] = 0.0f;
camera.up[0] = 0.0f;
camera.up[1] = 1.0f;
camera.up[2] = 0.0f;
camera.near = 0.01f;
camera.far = 100.0f;

dvz_panel_set_camera_desc(panel, &camera);
```

Attach a controller after setting the camera when the example needs interactive navigation. Orbit,
turntable, and fly descriptors commonly reuse the same target, eye, or up convention. Arcball is
primarily object-inspection state, but it still depends on a clear view convention for predictable
drag axes and lighting.


## Important Details

Camera conventions affect depth, lighting, picking, labels, and orientation UI. Confirm the scene's
axis convention before adding a grid, orientation gizmo, scientific labels, or a controller with a
world-up direction.

Center or normalize imported geometry before choosing camera distances. If the data is very large
or very small, adjust controller speeds and clipping planes to match the same scale.

Use perspective projection for most spatial inspection. Use orthographic projection when apparent
size should not depend on distance, for example CAD-like inspection, aligned volumes, or physical
scale comparisons.

## Common Mistakes

- Using visual transforms as camera transforms.
- Combining 2D domain navigation and 3D camera navigation on the same panel.
- Choosing `near` and `far` values that clip the data or waste depth precision.
- Forgetting to center or scale imported geometry before attaching a 3D controller.
- Letting the controller imply an axis convention that disagrees with labels, lighting, or the
  orientation gizmo.

## See Also

- [Use 3D controllers](3d-navigation.md)
- [Use coordinate systems](coordinate-systems.md)
- [Use lighting and materials](lighting-and-materials.md)

??? example "Related examples"

    - [Arcball Controller](../examples/gallery/features/feature_controller_arcball.md) - Source: `examples/c/features/controller_arcball.c`
    - [Orientation Gizmo](../examples/gallery/features/feature_orientation_gizmo.md) - Source: `examples/c/features/orientation_gizmo.c`
