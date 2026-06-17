# Open an Interactive Window

Run a scene in a native GLFW window.

## Task Workflow

Build the scene first, create an app from that scene, attach a GLFW view to the figure, then run the
event loop. Use this path for desktop interaction and controller-driven examples.

## Minimal Call Sequence

```c
DvzApp* app = dvz_app(scene);
dvz_view_glfw(app, figure, width, height, "Datoviz");
dvz_app_run(app, 0);
dvz_app_destroy(app);
```

Pass `0` to `dvz_app_run()` to run until the window closes. Pass a positive frame count for smoke
tests or deterministic captures.

## Canonical Examples

- Gallery: [GLFW App](../examples/gallery/features/feature_app_glfw.md)
- Source: `examples/c/features/app_glfw.c`
- Gallery: [Panzoom](../examples/gallery/features/feature_panzoom.md)
- Source: `examples/c/features/panzoom.c`

## Important Details

Bind controllers before entering the run loop:

```c
DvzController* controller = dvz_panzoom(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);
```

The app/runtime is not a scene object. It should be destroyed explicitly with `dvz_app_destroy()`
before `dvz_scene_destroy()`.

## Common Mistakes

- Capturing a PNG from a GLFW view before the first frame has rendered.
- Running a native window path in headless CI instead of using `dvz_view_offscreen()`.
- Sharing platform window handles without using the external-surface integration path.

## See Also

- [Create a scene](create-a-scene.md)
- [Use panzoom](use-panzoom.md)
- [Handle input events](input-events.md)
