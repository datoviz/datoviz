# Controllers

Controllers are scene-side state machines. They translate input events into panel, camera,
selection, hover, or navigation state changes. They do not emit DRP2, record backend commands, or
own backend resources.

## Public Controller Families

| Controller | Primary use | Scope | Typical binding | WebGPU status |
| --- | --- | --- | --- | --- |
| Panzoom | 2D pan and wheel zoom. | Panel-local or shared. | `DVZ_DIM_MASK_XY`, or one axis with `DVZ_DIM_MASK_X` / `DVZ_DIM_MASK_Y`. | Live for promoted 2D routes. |
| Arcball | Object inspection around a pivot. | Panel-local or shared. | `DVZ_DIM_MASK_XYZ`. | Live for promoted 3D routes. |
| Fly | Free camera navigation. | Panel-local. | `DVZ_DIM_MASK_XYZ`. | Live where the example route is promoted. |
| Turntable | Constrained 3D rotation. | Panel-local. | `DVZ_DIM_MASK_XYZ`. | Live where the example route is promoted. |

Use controllers for standard navigation before adding raw input callbacks.

## Binding And Linking

Controllers are opaque handles that panels bind per dimension. Sharing one controller handle links
every bound part of that controller state; use separate controllers when panels should navigate
independently.

Examples:

```c
DvzController* panzoom = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(panel, panzoom, DVZ_DIM_MASK_XY);

DvzController* arcball = dvz_arcball(scene, NULL);
dvz_panel_bind_controller(panel3d, arcball, DVZ_DIM_MASK_XYZ);
```

When panels need independent controllers but selected state must stay synchronized, create a
scene-owned link with `dvz_controller_link()`. Links connect distinct controllers of the same
family, propagate a component mask such as X extent or rotation, and may be one-way or two-way.
Sharing one controller remains the simplest choice when all of its bound state should be common.

See [Link panels and controllers](../how-to/link-panels.md) for the selection rules and the
generated [`dvz_controller_link()` reference](c-api/scene.md#dvz_controller_link) for the exact
component and mode types.

## Event Flow

| Step | Behavior |
| --- | --- |
| Normalize input | Backend input becomes scene-level pointer, wheel, keyboard, resize, timer, or gesture events. |
| Route event | The scene chooses hovered/focused/captured panel and controller. |
| Mutate state | Controller updates panel domain, camera, hover, selection, or related scene state. |
| Mark dirty | Transform, axis/layout, visual style, or redraw dirtiness is recorded as narrowly as possible. |
| Render next frame | Validation, frame planning, DRP2 emission, and runtime execution observe the new state. |

## Invalidation

| Mutation | Typical consequence |
| --- | --- |
| Panzoom or camera navigation | Panel transform dirty; redraw. |
| Visible domain change | Axis/layout dirty where axes depend on the domain. |
| Hover or selection state change | Redraw, and visual-property dirtiness when styling depends on state. |
| Controller mode or query policy change | Frame-plan dirtiness only when participation or query topology changes. |

## Limitations

- Panzoom is a 2D controller; use 3D controllers for mesh, sphere, volume, and other 3D scenes.
- WebGPU support is example/subset-specific, not full native parity.
- Controllers mutate retained state; they are not a substitute for custom shader logic.
- Backend-specific GLFW or browser events should not become authoritative application state.

## See Also

- [Use panzoom](../how-to/use-panzoom.md)
- [Use 3D controllers](../how-to/3d-navigation.md)
- [Input events](../how-to/input-events.md)
- [Coordinate systems](coordinate-systems.md)
