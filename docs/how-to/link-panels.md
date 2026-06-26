# Link Panels and Controllers

Make multiple panels pan, zoom, or probe together.

![Linked Panels](../assets/gallery/v0.4/features/feature_panel_linked.webp)

## Task Workflow

Create the panels first. Bind the same controller object to every panel that should share view
state. Keep separate controllers for panels that should move independently.

| Goal | Pattern |
| --- | --- |
| Two panels always share the same pan/zoom state. | Bind the same controller handle to both panels. |
| Panels share only one axis. | Bind the same panzoom controller with `DVZ_DIM_MASK_X` or `DVZ_DIM_MASK_Y`. |
| Panels need independent state plus selected synchronization. | Use separate controllers and `dvz_controller_link()`. |
| Axes, colorbars, visuals, or probe readouts should match. | Share the corresponding scale/data/readout state explicitly; controller sharing does not copy them. |

## Minimal Call Sequence

```c
DvzController* shared = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(panel_a, shared, DVZ_DIM_MASK_XY);
dvz_panel_bind_controller(panel_b, shared, DVZ_DIM_MASK_XY);
```

Use a narrower dimension mask to link only one axis.

```c
DvzController* shared_x = dvz_panzoom(scene, NULL);
DvzController* y_a = dvz_panzoom(scene, NULL);
DvzController* y_b = dvz_panzoom(scene, NULL);

dvz_panel_bind_controller(panel_a, shared_x, DVZ_DIM_MASK_X);
dvz_panel_bind_controller(panel_a, y_a, DVZ_DIM_MASK_Y);
dvz_panel_bind_controller(panel_b, shared_x, DVZ_DIM_MASK_X);
dvz_panel_bind_controller(panel_b, y_b, DVZ_DIM_MASK_Y);
```

For linked but distinct controllers, use a two-way controller link when either panel should drive
the shared state:

```c
DvzController* x_a = dvz_panzoom(scene, NULL);
DvzController* x_b = dvz_panzoom(scene, NULL);

dvz_controller_link(scene, x_a, x_b, DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_TWO_WAY);
```

## Important Details

Controller sharing links interaction state. It does not copy visuals, axes, colorbars, or data
between panels.

Sharing the same controller is the simplest and strictest link: every bound part of that controller
state is common. Use it for panels that should move as one view.

`dvz_controller_link()` is more selective. It supports one-way and two-way links between distinct
controllers of the same family. Panzoom links support pan, zoom, X extent, and Y extent components.
Arcball links support rotation, pan, and zoom components. For two-way links, the actively
interacting endpoint drives the passive endpoint; when neither endpoint is active, the declared
source initializes the target.

Linked controller state does not make domains or units compatible. Set each panel's domain,
view/aspect policy, axes, and units deliberately before linking. For shared X workflows with
different Y ranges, bind or link X state only and keep each Y controller separate.

Linked probe workflows have more state than navigation. The panels may share a controller, but the
application still needs one consistent sampled field, color scale, query mapping, and readout
format.

## Common Mistakes

- Creating two identical controllers instead of sharing one controller pointer.
- Sharing a full `DVZ_DIM_MASK_XY` controller when only X should be linked.
- Expecting `dvz_controller_link()` to work between different controller families.
- Linking panels with simultaneous active interactions and expecting both gestures to merge.
- Linking panels with incompatible domains.
- Forgetting to keep axes, colorbars, units, and probe readouts synchronized with the linked view.
- Treating showcase workflows as the minimal linked-panel API.

## See Also

- [Create multiple panels](create-multiple-panels.md)
- [Use panzoom](use-panzoom.md)
- [Controllers reference](../reference/controllers.md)
- [Add axes](axes.md)
- [Add colorbars, scale bars, and legends](adornments.md)
- [Probe image or field values](probe-fields.md)

??? example "Related examples"

    - [Linked Panels](../examples/gallery/features/feature_panel_linked.md) - Source: `examples/c/features/panel_linked.c`
    - [Linked Panels With Axes](../examples/gallery/showcases/linked_panels_axes_panzoom.md) - Source: `examples/c/showcases/panel_linked_axes.c`
    - [Linked Probe With Colorbar](../examples/gallery/showcases/linked_panels_probe_colorbar.md) - Source: `examples/c/showcases/linked_probe_colorbar.c`
