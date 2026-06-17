# Handle Input Events

React to keyboard, mouse, and pointer input in native examples.

## Task Workflow

Use controllers for standard navigation first. Add explicit input callbacks when the application
needs custom selection, toggles, overlays, or host integration.

For host GUI controls, let the UI own application state such as toggles, sliders, and mode
switches, then update retained Datoviz visual data, visibility, or controller state from that
state.

## Minimal Workflow

1. Bind a controller first if the input is ordinary navigation.
2. Register the native input callback path shown in `examples/c/features/input_events.c` only for
   custom gestures, shortcuts, or host integration.
3. In the callback, inspect the event, update retained scene state, and request/render the next
   frame through the app or host loop.

Keep callback work small; defer expensive updates to the next frame or a controlled update path.


## Important Details

Input events are native-only in the current feature example. Browser interaction is handled by the
WebGPU route and should not be copied from GLFW callback code.

## Common Mistakes

- Reimplementing pan/zoom in raw input callbacks.
- Mutating visual data from long-running callback work.
- Assuming GLFW key codes are portable to WebGPU.
- Letting GUI state and visual attributes drift apart.

## See Also

- [Use panzoom](use-panzoom.md)
- [Pick items](pick-and-probe.md)
- [Embed in Qt](embed-in-qt.md)

??? example "Related examples"

    - [Input Events](../examples/gallery/features/feature_input_events.md) - Source: `examples/c/features/input_events.c`
    - [Picking](../examples/gallery/features/feature_picking.md) - Source: `examples/c/features/picking.c`
    - [GUI Controls](../examples/gallery/features/feature_gui_controls.md) - Source: `examples/c/features/gui_controls.c`
    - [Raw cimgui GUI](../examples/gallery/features/feature_gui_cimgui.md) - Source: `examples/c/features/gui_cimgui.c`
    - [GUI Viewport](../examples/gallery/features/feature_gui_viewport.md) - Source: `examples/c/features/gui_viewport.c`
