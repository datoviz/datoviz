# Controllers

## List of controllers


When creating a new panel, one needs to specify a **Controller**. This object defines how the user interacts with the panel.

The controllers currently implemented are:

* **static**: no interactivity,
* **panzoom**: pan and zoom with the mouse,
* **axes**: axes with ticks, tick labels, grid, and interactivity with pan and zoom,
* **arcbcall**: static 3D camera, model rotation with the mouse,
* **camera**: first-person 3D camera.

More controllers will be implemented in the future. The C interface used to create custom controllers will be refined too.

### Panzoom

The **panzoom controller** provides mouse interaction patterns for panning and zooming:

* **Mouse dragging with left button**: pan
* **Mouse dragging with right button**: zoom in x and y axis independently
* **Mouse wheel**: zoom in and out in both axes simultaneously
* **Double-click with left button**: reset to initial view

### Axes 2D

The **axes 2D controller** displays ticks, tick labels, grid and provides panzoom interaction.

### Arcball

The arcball controller is used to rotate a 3D object in all directions using the mouse. It is implemented with quaternions.

### First-person camera

Left-dragging controls the camera, the arrow keys control the position, the Z is controlled by the mouse wheel.





## List of event types

| Name | Description |
| ---- | ---- |
| `init` | called at the beginning of the first frame |
| `frame` | called at every frame |
| `refill` | called when the command buffers need to be recreated (e.g. window resize) |
| `resize` | called when the window is resized |
| `timer` | called in the main loop at regular time intervals |
| `mouse_press` | called when a mouse button is pressed |
| `mouse_release` | called when a mouse button is released |
| `mouse_move` | called when the mouse moves |
| `mouse_wheel` | called when the mouse wheel moves |
| `mouse_drag_begin` | called when a mouse drag operation begins |
| `mouse_drag_end` | called when a mouse drag operation ends |
| `mouse_click` | called after a single click |
| `mouse_double_click` | called after a double click |
| `key_press` | called when a key is pressed |
| `key_release` | called when a key is released |
