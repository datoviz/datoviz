# Use Coordinate Systems

Map your data coordinates into panel space.

## Task Workflow

Decide which coordinate space your positions use, set the panel domain when positions are data
coordinates, attach visuals with the matching coordinate interpretation, then bind a controller
that operates on the same axes.

For normalized examples, positions often already live in `[-1, +1]` view coordinates. For plots,
maps, fields, and measurement-style overlays, prefer data coordinates plus an explicit panel
domain.

| Space | Use when | API hook |
| --- | --- | --- |
| `DVZ_COORD_DATA` | Positions are scientific or application data values. | Set panel domains and attach the visual with `coord_space = DVZ_COORD_DATA`. |
| `DVZ_COORD_VIEW` | Positions are already in panel view coordinates, usually `[-1, +1]`. | This is the default when `dvz_panel_add_visual()` receives `NULL`. |
| `DVZ_COORD_PANEL` | Positions are normalized to the panel itself for fixed overlays. | Attach with `coord_space = DVZ_COORD_PANEL`. |

## Data-Space Call Sequence

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);

DvzVisualAttachDesc attach = dvz_visual_attach_desc();
attach.coord_space = DVZ_COORD_DATA;
dvz_panel_add_visual(panel, visual, &attach);

DvzController* controller = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);
```

In this pattern, upload positions in your data units. The panel maps X and Y from the configured
domain into the visual panel range used for rendering. Panzoom changes the visible part of that
domain; it does not rewrite the uploaded source positions.

## View-Space Call Sequence

```c
// Positions already use the default view coordinate range.
dvz_visual_set_data(visual, "position", positions, count);
dvz_panel_add_visual(panel, visual, NULL);
```

Use this for examples or low-level visuals that intentionally work in normalized panel view space.
If the data has real units, avoid silently rescaling it into `[-1, +1]` unless that rescaling is part
of the example contract.

## 3D Scene Coordinates

For 3D scenes, keep object positions in the model/world coordinate system chosen by the example,
then use a camera and a 3D controller:

```c
DvzCameraDesc camera = dvz_camera_desc();
camera.eye[2] = 3.0f;
dvz_panel_set_camera(panel, &camera);

DvzController* controller = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

Use arcball, orbit, turntable, or fly controllers for 3D panels. Use panzoom for 2D panels. Mixing a
2D controller with 3D camera navigation usually means the panel and visual coordinate model is not
defined clearly enough.

## Choosing Domains

Set a domain when coordinates have external meaning, such as time, depth, distance, image sample
indices, atlas positions, or projected map coordinates. Set both X and Y for ordinary 2D navigation
so the visible rectangle is explicit.

Use `dvz_panel_visible_domain()` when you need the current data interval after controller changes.
Use `dvz_panel_data_to_visual_positions()` only when you intentionally need a CPU-side conversion
from panel data coordinates to visual coordinates; ordinary retained data-space attachments should
let the scene perform that mapping.


## Important Details

Panel domains describe the visible data range. Controllers modify the view over that domain. Visual
transforms are for object-level placement and should not replace a panel domain when the task is
ordinary data navigation.

Keep these boundaries clear:

- Domains describe the visible data interval for one panel dimension.
- Attachment coordinate space tells the panel how to interpret one visual's positions.
- Controllers update the view or camera state, not the source data.
- Visual transforms move or scale one visual relative to its attachment space.
- Texture coordinates and field sample coordinates are separate from panel data coordinates.

Nonlinear and geographic projections are not scene-managed in v0.4. Project those data on the CPU,
upload ordinary Cartesian positions or sampled fields, and keep projection metadata in labels,
legends, annotations, or application state.

Pointer input and queries start from framebuffer pixels, then pass through the target viewport and
panel transform. Do not treat CPU geometry coordinates as a substitute for rendered query behavior;
pick/probe results should follow the same transform, clipping, depth, and shader path as rendering.

## Common Mistakes

- Uploading pixel coordinates to a data-space panel.
- Uploading data coordinates but attaching the visual with the default `DVZ_COORD_VIEW` space.
- Setting only one axis domain and expecting aspect-preserving 2D navigation.
- Using a visual transform to compensate for a wrong data domain.
- Mixing 2D panzoom with 3D orbit/arcball camera control.
- Pre-projecting geographic or nonlinear data without recording the projected units.

## See Also

- [Use panzoom](use-panzoom.md)
- [Configure cameras](configure-cameras.md)
- [Transform visual data](transforms-and-scales.md)
- [Coordinate systems reference](../reference/coordinate-systems.md)

??? example "Related examples"

    - [Coordinate System](../examples/gallery/features/feature_coordinate_system.md) - Source: `examples/c/features/coordinate_system.c`
    - [Panel View 2D](../examples/gallery/features/feature_panel_view2d.md) - Source: `examples/c/features/panel_view2d.c`
    - [User Scale](../examples/gallery/features/feature_user_scale.md) - Source: `examples/c/features/user_scale.c`
