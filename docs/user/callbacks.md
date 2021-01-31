# Callbacks

The canvas provides an event system where the user can specify callback functions to be called at specific times during the time course of the canvas.

An event callback may be registered as a sync or async callback.

* **Sync callbacks** are called directly when an event is raised by the library.
* **Async callbacks** are called in a background thread managed by the library. The events are pushed to a thread-safe FIFO queue, and they are consumed by the background thread.

It is recommended to only use async callbacks when needed, as they come with some overhead. They are mostly useful with I/O-bound operations, for example, making a network request in response to a keyboard key press.


## Registering a callback function

The callback function receives two arguments: the canvas, and a special struct containing information about the event. The special field `u` is a union containing information specific to the event type.

=== "C"
    ```c
    void void callback(VklCanvas* canvas, VklEvent ev)
    {
        double time = ev.u.t.time;
        // ...
    }

    // ...

    vkl_event_callback(canvas, VKL_EVENT_TIMER, 1, VKL_EVENT_MODE_SYNC, callback, NULL);
    ```


## List of events

| Name | Description |
| ---- | ---- |
| `init` | called at the beginning of the first frame |
| `frame` | called at every frame |
| `refill` | called when the command buffers need to be recreated (e.g. window resize) |
| `resize` | called when the window is resized |
| `timer` | called in the main loop at regular time intervals |
| `mouse_button` | called when a mouse button is pressed or released |
| `mouse_move` | called when the mouse moves |
| `mouse_wheel` | called when the mouse wheel moves |
| `mouse_drag_begin` | called when a mouse drag operation begins |
| `mouse_drag_end` | called when a mouse drag operation ends |
| `mouse_click` | called after a single click |
| `mouse_double_click` | called after a double click |
| `key` | called when a key is pressed or released |

More events will be implemented/documented soon.
