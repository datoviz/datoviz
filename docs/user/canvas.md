# The Canvas

## Creating a canvas

* Background color

## Creating multiple canvases


## Making screenshots




## Environment variables

Visky defines a few useful environment variables:

| Environment variable              | Description                                           |
|-----------------------------------|-------------------------------------------------------|
| `VKY_AUTO_CLOSE=3`                | Automatically close the canvas after X seconds        |
| `VKY_AUTO_SCREENSHOT=image.png`   | Automatically make a screenshot and close the canvas  |
| `VKY_DPI_FACTOR=1.5`              | Change the DPI scaling factor                         |
| `VKY_EXAMPLE=app_blank`           | Compile only this example with `./manage.sh build`    |
| `VKY_FPS=1`                       | Show the number of frames per second                  |
| `VKY_INVERSE_MOUSE_WHEEL=1`       | Inverse the mouse wheel direction                     |
| `VKY_LOG_LEVEL=0`                 | Logging level                                         |
| `VKY_VSYNC=0`                     | Disable vertical sync                                 |

### Notes

* **Vertical synchronization** is activated by default. The refresh rate is typically limited to 60 FPS. Deactivating it (which is automatic when using `VKY_FPS=1`) leads to the event loop running as fast as possible, which is useful for benchmarking. It may lead to 100% CPU utilization, whereas vertical synchronization is typically light on CPU cycles. Note also that user interaction seems laggy when vertical synchronization is active (the default). When it comes to GUI interaction (mouse movements, drag and drop, and so on), we're used to lags lower than 10 milliseconds.
* **Logging levels**: 0=trace, 1=debug, 2=info, 3=warning, 4=error
* **DPI scaling factor**: Visky natively supports DPI scaling for linewidths, font size, axes, etc. Since automatic cross-platform DPI detection does not seem reliable, Visky simply uses sensible defaults but provides an easy way for the user to increase or decrease the DPI via this environment variable. This is useful on high-DPI/Retina monitors.

### C API

The C API provides three functions to get the environment variables programmatically:

```c
// Return an environment variable as a string.
const char* value = getenv(name); // provided by the standard stdlib.h

// Convenience function to parse the environment variable into a boolean.
bool value = vky_check_env(name, default_bool_value);

// Convenience function to parse the environment variable into a double-precision floating-point number.
double value = vky_get_env(name, default_double_value);
```
