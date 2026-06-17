# Embed in Qt

Integrate Datoviz with a host UI toolkit without creating a parallel renderer.

## Task Workflow

Keep Datoviz responsible for GPU rendering and let the host toolkit own the application shell,
menus, and widgets. Use the external-surface or viewport integration path closest to the platform
you target.

## Minimal Workflow

1. Let the host toolkit create or expose the native surface.
2. Create Datoviz with any required host Vulkan instance extensions.
3. Attach a Datoviz view to the external surface or hosted viewport path.
4. Let the host event loop drive resize, input, frame requests, and `dvz_view_render_once()`.

Use the GLFW external-surface example as the closest maintained native embedding reference.


## Important Details

Qt embedding is a host-integration task. Do not create a second Vulkan wrapper or presentation
stack; adapt the existing runtime/view boundary.

## Common Mistakes

- Letting both Qt and Datoviz own the same native graphics handle.
- Handling resize in the UI but not notifying the Datoviz view.
- Copying internal runtime code instead of using public integration surfaces.

## See Also

- [Open an interactive window](create-a-window.md)
- [Handle input events](input-events.md)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Related examples"

    - [External Surface GLFW](../examples/gallery/advanced/advanced_external_surface_glfw.md) - Source: `examples/c/advanced/external_surface_glfw.c`
    - [GUI Viewport](../examples/gallery/features/feature_gui_viewport.md) - Source: `examples/c/features/gui_viewport.c`
