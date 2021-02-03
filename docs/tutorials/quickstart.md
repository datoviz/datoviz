# Python tutorial

Canvas
Visual
Data
Another panel with diff controller and visual
Callback
GUI








## First plot with the Python API

!!! note
    The Python bindings are still at a much earlier stage of development than the C API.

The following code example shows how to make a simple scatter plot in Python:

<!-- CODE_PYTHON bindings/cython/examples/test.py -->

<!-- IMAGE ../images/python_example.png -->


## First plot with the C API

<!-- CODE_C examples/standalone_scene.c -->

<!-- IMAGE ../images/c_example.png -->












# The Canvas

Datoviz provides three similar, but different abstractions:

* the **scene**,
* the **canvas**,
* the **window**.

The **scene** provides a relatively high-level plotting interface that allows to arrange panels (subplots) in a grid, define controllers, and add visuals to the panels.

The **canvas** is lower-level object that allows to use Vulkan directly via vklite. While the scene deals with *visual* elements, the canvas deals with *Vulkan* objects.

The **window** is an abstraction provided by the backend windowing library, glfw at the moment. It is a bare window that doesn't allow for any kind rendering, unless manually creating a swapchain and so on by using Vulkan or vklite directly.

**Most users will only need with the scene.** Advanced users will use the canvas to create custom applications, interactive animations, or even small video games. Finally, the window is only used internally and will probably never be used directly.

On this page, we'll focus on the canvas.

## Creating a canvas

The following code snippet displays a blank window with a black background.

=== "Python"
    ```python
    # Required imports.
    from datoviz import canvas, run

    # Create the canvas.
    c = canvas()

    # Run the event loop.
    run()
    ```

=== "C"
    ```c
    #include <datoviz/datoviz.h>

    int main()
    {
        // Create a singleton application with a GLFW backend.
        DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);

        // Use the first detected GPU. The last argument is the GPU index.
        DvzGpu* gpu = dvz_gpu(app, 0);

        // Create a new canvas with the size specified. The last argument is for optional flags.
        DvzCanvas* canvas = dvz_canvas(gpu, 1280, 1024, 0);

        // Run the main rendering loop.
        dvz_app_run(app, 0);

        // We need to clean up all objects handled by Datoviz at the end.
        dvz_app_destroy(app);

        return 0;
    }
    ```

The rendering loop is an infinite loop that continuously refreshes the canvas until the Escape key is pressed, at which point the canvas is closed. The canvas destruction logic (freeing the memory on the host and on the GPU) is called automatically as soon as the canvas is closed.

Multiple canvases can be created. The application stops as soon as there is no remaining open canvas.


## Changing the background color

The C API provides a function to change the background color of a canvas.

=== "C"
    ```c
    dvz_canvas_clear_color(canvas, 1, 0, 0); // red background
    ```


## Making screenshots

Here is how to create a screenshot of the first frame, *before* rendering the rendering loop.

=== "Python"
    ```python
    run(screenshot="screenshot.png")
    ```

=== "C"
    ```c
    dvz_screenshot_file(canvas, "screenshot.png");
    ```

Creating a screenshot *during* the lifetime of a canvas will be implemented soon. Similarly for the ability to create multiple screenshots efficiently, or even recording a video (screencast).



## Environment variables

!!! warning
    This section may be partly out of date.

Datoviz defines a few useful environment variables:

| Environment variable              | Description                                           |
|-----------------------------------|-------------------------------------------------------|
| `DVZ_DPI_FACTOR=1.5`              | Change the DPI scaling factor                         |
| `DVZ_FPS=1`                       | Show the number of frames per second                  |
| `DVZ_INVERSE_MOUSE_WHEEL=1`       | Inverse the mouse wheel direction                     |
| `DVZ_LOG_LEVEL=0`                 | Logging level                                         |
| `DVZ_VSYNC=0`                     | Disable vertical sync                                 |

### Notes

* **Vertical synchronization** is activated by default. The refresh rate is typically limited to 60 FPS. Deactivating it (which is automatic when using `DVZ_FPS=1`) leads to the event loop running as fast as possible, which is useful for benchmarking. It may lead to high CPU and GPU utilization, whereas vertical synchronization is typically light on CPU cycles. Note also that user interaction seems laggy when vertical synchronization is active (the default). When it comes to GUI interaction (mouse movements, drag and drop, and so on), we're used to lags lower than 10 milliseconds, which a frame rate of 60 FPS cannot achieve.
* **Logging levels**: 0=trace, 1=debug, 2=info, 3=warning, 4=error
* **DPI scaling factor**: Datoviz natively supports DPI scaling for linewidths, font size, axes, etc. Since automatic cross-platform DPI detection does not seem reliable, Datoviz simply uses sensible defaults but provides an easy way for the user to increase or decrease the DPI via this environment variable. This is useful on high-DPI/Retina monitors.











# The scene

The **scene** provides facilities to create **panels** (subplots) within a canvas, add visual elements to the panels in various coordinate systems, and define **controllers** (how to interact with the visuals in the panels).


## Coordinate system

Datoviz uses the standard OpenGL 3D coordinate system:

![Datoviz coordinate system](../images/cds.svg)
*Datoviz coordinate system*

Note that this is different from the Vulkan coordinate system, where y and z go in the opposite direction. The other difference is that in Datoviz, all axes range in the interval `[-1, +1]`. In the original Vulkan coordinate system, `z` goes from 0 to 1 instead.

This convention makes it possible to use existing camera matrix routines implemented in the cglm library. The GPU code of all included shaders include the final OpenGL->Vulkan transformation right before the vertex shader output.

Other conventions for `x, y, z` axes will be supported in the future.


## Data transforms

Position props are specified in the original data coordinate system corresponding to the scientific data to be visualized. Yet, Datoviz requires vertex positions to be in normalized coordinates (between -1 and 1) when sent to the GPU. Since the GPU only deals with single-precision floating point numbers, doing data normalization on the GPU would result in significant loss of precision and would harm performance.

Therefore, Datoviz provides a system to make transformations on the CPU **in double precision** before uploading the data to the GPU. By default, the data is linearly transformed to fit the [-1, +1] cube. Other types of transformations will soon be implemented (polar coordinates, geographic coordinate systems, and so on).


## Controllers

When creating a new panel, one needs to specify a **Controller**. This object defines how the user interacts with the panel.

=== "Python"
    ```python
    panel = c.panel(row=0, col=0, controller='axes')
    ```
=== "C"
    ```c
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);
    ```

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


## Subplots

Panels are organized within a **grid** layout. Each panel is indexed by its row and column. By default, there is a single panel spanning the entire canvas.

By default, a regular grid is created. You can customize the size of each colum, row, and make panels span multiple grid cells. Only the C interface is implemented at the moment.

=== "C"
    ```c
    dvz_panel_size(panel, DVZ_GRID_HORIZONTAL, 0.5); // proportion of the width
    dvz_panel_span(panel, DVZ_GRID_HORIZONTAL, 2); // the panel spans 2 horizontal cells
    ```






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
    void void callback(DvzCanvas* canvas, DvzEvent ev)
    {
        double time = ev.u.t.time;
        // ...
    }

    // ...

    dvz_event_callback(canvas, DVZ_EVENT_TIMER, 1, DVZ_EVENT_MODE_SYNC, callback, NULL);
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













## Next steps

The simple examples above showed:

* how to create an application,
* how to create a canvas,
* how to create a panel with an axes controller,
* how to add a visual,
* how to set visual data,
* how to run the application.

These steps already cover a large number of simple plotting applications. The strength of the library is to provide a unified interface to all kinds of visual elements. Next steps will show you:

* how to use colormaps,
* the list of currently supported visuals, with their associated user-settable properties,
* the list of currently supported controller types,
* how to create subplots.

Further notes:

* The examples shown here use the so-called **scene API**. Datoviz provides other, lower-level APIs for those with more advanced needs (see the expert manual). In particular, it is possible to use a "raw" canvas and have full control on what is displayed (for example, simple animations or video games).
* Although the library should cover most use-cases with the included visuals, you can also write your own visuals. Writing custom shaders is not straightforward, however, and this topic is covered in the expert manual.








Datoviz integrates the [Dear ImGUI C++ library](https://github.com/ocornut/imgui) which allows one to create GUIs directly in a Datoviz canvas, without resorting to GUI backends such as Qt or wx. This reduces the number of required dependencies and allows for easy GUI integration.

!!! note
    Datoviz integrates Dear ImGUI via a git submodule ([fork](https://github.com/datoviz/imgui) in the Datoviz GitHub organization). There's a custom branch based on the `docking` upstream branch, which an additional [patch](https://github.com/martty/imgui/commit/f1f948bea715754ad5e83d4dd9f928aecb4ed1d3) applied to it in order to support creating GUIs with integrated Datoviz canvases.
