# Handle Input Events

React to keyboard, mouse, and pointer input in native examples.

Use explicit input callbacks when an application needs custom shortcuts, selection modes, overlays,
diagnostics, or host integration. Use controllers instead when the input is ordinary navigation.

## Task Workflow

Use controllers for standard navigation first. Add explicit input callbacks when the application
needs custom selection, toggles, overlays, or host integration.

For host GUI controls, let the UI own application state such as toggles, sliders, and mode
switches, then update retained Datoviz visual data, visibility, or controller state from that
state.

## Minimal Workflow

1. Bind a controller first if the input is ordinary navigation.
2. Get the view input router with `dvz_view_input(view)`.
3. Subscribe to the routed event stream with `dvz_input_subscribe_event()`.
4. In the callback, inspect the event, update retained scene state, and request or render the next
   frame through the app or host loop.
5. Unsubscribe before the callback `user_data` is destroyed.

Keep callback work small; defer expensive updates to the next frame or a controlled update path.

```c
typedef struct
{
    bool show_overlay;
} AppState;

static void on_input(DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    (void)router;
    AppState* state = (AppState*)user_data;
    if (state == NULL || event == NULL)
        return;

    switch (event->type)
    {
    case DVZ_INPUT_EVENT_POINTER:
        /* Read event->content.pointer.pos, button, mods, and gesture type. */
        break;

    case DVZ_INPUT_EVENT_KEYBOARD:
        if (event->content.keyboard.type == DVZ_KEYBOARD_EVENT_PRESS &&
            event->content.keyboard.key == DVZ_KEY_O)
            state->show_overlay = !state->show_overlay;
        break;

    case DVZ_INPUT_EVENT_RESIZE:
        /* Read framebuffer, window, and content-scale dimensions. */
        break;

    default:
        break;
    }
}

DvzInputRouter* router = dvz_view_input(view);
DvzCallbackId callback_id = dvz_input_subscribe_event(router, on_input, &state);
dvz_input_unsubscribe(router, callback_id);
```

The related example uses this same route:
`examples/c/features/input_events.c`.


## Event Streams

| Stream | Subscribe with | Use for |
| --- | --- | --- |
| Routed input | `dvz_input_subscribe_event()` | Most application callbacks; pointer, keyboard, resize, scale, and high-level pointer gestures in one stream. |
| Raw pointer | `dvz_input_subscribe_pointer()` | Backend-normalized position, button, and wheel events before gesture interpretation. |
| Keyboard | `dvz_input_subscribe_keyboard()` | Keyboard-only consumers that do not need routed pointer, resize, or scale events. |
| Resize | `dvz_input_subscribe_resize()` | Resize-only consumers and late-bound layout state. |

Use `dvz_input_subscribe_event()` when you need click, double-click, drag-start, drag, or drag-stop
events. Those gesture-derived events are emitted on the routed input stream.


## Testing Callbacks

Synthetic event emission is useful for smoke tests and hosted integrations. After creating a view,
subscribe the callback and emit events through the view helper API:

```c
dvz_view_emit_resize(view, width, height, width, height, 1.0f, 1.0f);
dvz_view_emit_pointer(
    view, DVZ_POINTER_EVENT_PRESS, x, y, width, height, DVZ_POINTER_BUTTON_LEFT,
    DVZ_KEY_MODIFIER_NONE);
dvz_view_emit_wheel(view, x, y, width, height, 0.0f, +1.0f, DVZ_KEY_MODIFIER_NONE);
dvz_view_emit_key(view, DVZ_KEYBOARD_EVENT_PRESS, DVZ_KEY_A, DVZ_KEY_MODIFIER_NONE);
```

The synthetic path works with offscreen views, so it can run in automated checks without opening a
native GLFW window.


## Important Details

Input events are native-only in the current feature example. Browser interaction is handled by the
WebGPU route and should not be copied from GLFW callback code.

Callbacks run synchronously on the emitting thread. Keep callback state owned by the application,
and unsubscribe before destroying that state or the view.

## Common Mistakes

- Reimplementing pan/zoom in raw input callbacks.
- Mutating visual data from long-running callback work.
- Assuming GLFW key codes are portable to WebGPU.
- Letting GUI state and visual attributes drift apart.
- Forgetting to unsubscribe callbacks that capture application state.

## See Also

- [Use panzoom](use-panzoom.md)
- [Pick items](pick-items.md)
- [Embed in Qt](embed-in-qt.md)

??? example "Related examples"

    - [Input Events](../examples/gallery/features/feature_input_events.md) - Source: `examples/c/features/input_events.c`
    - [Picking](../examples/gallery/features/feature_picking.md) - Source: `examples/c/features/picking.c`
    - [GUI Controls](../examples/gallery/features/feature_gui_controls.md) - Source: `examples/c/features/gui_controls.c`
    - [Raw cimgui GUI](../examples/gallery/features/feature_gui_cimgui.md) - Source: `examples/c/features/gui_cimgui.c`
    - [GUI Viewport](../examples/gallery/features/feature_gui_viewport.md) - Source: `examples/c/features/gui_viewport.c`
