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

While the event loop is running, a screenshot of a canvas can be made as follows (typically [in an event handler](interact.md), for example when pressing a key):

=== "Python"
    ```python
    # TODO: not implemented yet
    ```

=== "C"
    ```c
    dvz_screenshot(canvas, "screenshot.png");
    ```

!!! bug
    Currently, the screenshot will fail if the event loop is not running. A screenshot cannot be done during the very first frame of the event loop, it needs to wait for the second frame.

Alternatively, if the environment variable `DVZ_AUTO_SCREENSHOT=screenshot.png` is set, a screenshot will be done after the first frame and the canvas will close immediately afterwards. This is useful when one needs to non-interactively run a Datoviz executable and automatically save a screenshot.



## Environment variables

Datoviz defines a few useful environment variables:

| Environment variable              | Description                                           |
|-----------------------------------|-------------------------------------------------------|
| `DVZ_AUTO_CLOSE=3`                | Automatically close the canvas after X seconds        |
| `DVZ_AUTO_SCREENSHOT=image.png`   | Automatically make a screenshot and close the canvas  |
| `DVZ_DEVICE=0`                    | Default GPU device number to select                   |
| `DVZ_DPI_FACTOR=1.5`              | Change the DPI scaling factor                         |
| `DVZ_EXAMPLE=app_blank`           | Compile only this example with `./manage.sh build`    |
| `DVZ_FPS=1`                       | Show the number of frames per second                  |
| `DVZ_INVERSE_MOUSE_WHEEL=1`       | Inverse the mouse wheel direction                     |
| `DVZ_LOG_LEVEL=0`                 | Logging level                                         |
| `DVZ_VSYNC=0`                     | Disable vertical sync                                 |

### Notes

* **Vertical synchronization** is activated by default. The refresh rate is typically limited to 60 FPS. Deactivating it (which is automatic when using `DVZ_FPS=1`) leads to the event loop running as fast as possible, which is useful for benchmarking. It may lead to 100% CPU utilization, whereas vertical synchronization is typically light on CPU cycles. Note also that user interaction seems laggy when vertical synchronization is active (the default). When it comes to GUI interaction (mouse movements, drag and drop, and so on), we're used to lags lower than 10 milliseconds.
* **Logging levels**: 0=trace, 1=debug, 2=info, 3=warning, 4=error
* **DPI scaling factor**: Datoviz natively supports DPI scaling for linewidths, font size, axes, etc. Since automatic cross-platform DPI detection does not seem reliable, Datoviz simply uses sensible defaults but provides an easy way for the user to increase or decrease the DPI via this environment variable. This is useful on high-DPI/Retina monitors.

### C API

The C API provides three functions to get the environment variables programmatically:

```c
// Return an environment variable as a string.
const char* value = getenv(name); // provided by the standard stdlib.h

// Convenience function to parse the environment variable into a boolean.
bool value = dvz_env_bool(name, default_bool_value);

// Convenience function to parse the environment variable into a double-precision floating-point number.
double value = dvz_env_double(name, default_double_value);
```
