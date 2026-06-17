# Input Events

How to respond to keyboard, pointer, and resize events in a datoviz window.

## Overview

Datoviz routes input events through a per-view `DvzInputRouter`. Register a callback with
`dvz_input_subscribe_event` to receive a unified `DvzInputEvent` stream that includes pointer
gestures (click, drag, wheel), keyboard press/release/repeat, and window resize notifications.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdio.h>
    #include "datoviz/app.h"
    #include "datoviz/input.h"
    #include "datoviz/scene.h"

    /* unified event callback — pointer, keyboard, and resize arrive here */
    static void on_event(DvzInputRouter* router, const DvzInputEvent* ev, void* user_data)
    {
        (void)router;
        (void)user_data;
        if (ev->type == DVZ_INPUT_EVENT_POINTER)
        {
            const DvzPointerEvent* p = &ev->content.pointer;
            if (p->type == DVZ_POINTER_EVENT_CLICK)
                printf("click at (%.0f, %.0f)\n", p->pos[0], p->pos[1]);
        }
        else if (ev->type == DVZ_INPUT_EVENT_KEYBOARD)
        {
            const DvzKeyboardEvent* k = &ev->content.keyboard;
            if (k->type == DVZ_KEYBOARD_EVENT_PRESS)
                printf("key pressed: %d\n", (int)k->key);
        }
        else if (ev->type == DVZ_INPUT_EVENT_RESIZE)
        {
            const DvzInputResizeEvent* r = &ev->content.resize;
            printf("resize: %ux%u\n", r->window_width, r->window_height);
        }
    }

    int main(void)
    {
        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 640, 480, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* a simple point visual so the window has content */
        DvzVisual* visual = dvz_point(scene, 0);
        vec3 pos[1] = {{0.0f, 0.0f, 0.0f}};
        DvzColor color[1] = {{255, 200, 100, 255}};
        float size[1] = {20.0f};
        dvz_visual_set_data(visual, "position", pos, 1);
        dvz_visual_set_data(visual, "color", color, 1);
        dvz_visual_set_data(visual, "diameter", size, 1);
        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        /* open a window */
        DvzView* view = dvz_view_glfw(app, figure, 640, 480, "Input events");

        /* subscribe to all events on this view */
        DvzInputRouter* router = dvz_view_input(view);
        dvz_input_subscribe_event(router, on_event, NULL);

        dvz_app_run(app, 0);
        dvz_input_unsubscribe_event(router, on_event, NULL);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

After creating the scene, figure, panel, and visual in the usual way, call `dvz_app` and
`dvz_view_glfw` to obtain a `DvzView`. The view owns the input router for that window.

Retrieve the router with `dvz_view_input(view)`, then register your callback with
`dvz_input_subscribe_event(router, callback, user_data)`. The callback receives a `DvzInputEvent`
whose `type` field identifies the category: `DVZ_INPUT_EVENT_POINTER`,
`DVZ_INPUT_EVENT_KEYBOARD`, or `DVZ_INPUT_EVENT_RESIZE`. Access the event details through the
corresponding union field (`ev->content.pointer`, `ev->content.keyboard`, or
`ev->content.resize`).

Run the app with `dvz_app_run(app, 0)`. Before destroying the app, unsubscribe with
`dvz_input_unsubscribe_event` to avoid dangling references.

## Common patterns / Variants

**Check for modifier keys** — every pointer and keyboard event carries a `mods` bitmask:

```c
if (k->type == DVZ_KEYBOARD_EVENT_PRESS &&
    k->key == DVZ_KEY_Z &&
    (k->mods & DVZ_KEY_MODIFIER_CONTROL) != 0)
{
    /* Ctrl+Z pressed */
}
```

**Wheel scrolling** — pointer events of type `DVZ_POINTER_EVENT_WHEEL` expose the scroll
direction through `ev->content.pointer.content.w.dir[0]` (horizontal) and `.dir[1]` (vertical):

```c
if (p->type == DVZ_POINTER_EVENT_WHEEL)
    printf("scroll dy=%.2f\n", p->content.w.dir[1]);
```

**Drag gesture** — the router synthesizes drag events from raw move+press sequences. Listen for
`DVZ_POINTER_EVENT_DRAG_START`, `DVZ_POINTER_EVENT_DRAG`, and `DVZ_POINTER_EVENT_DRAG_STOP` to
track the full drag lifecycle.

## See also

- [Use panzoom](use-panzoom.md) — built-in 2D navigation controller
- [3D navigation](3d-navigation.md) — arcball, turntable, and fly controllers
- [Create a scene](create-a-scene.md) — scene/figure/panel setup
