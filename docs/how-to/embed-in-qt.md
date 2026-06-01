# Embed In Qt

Status: advanced/experimental.

Datoviz can be hosted inside a Qt application without making Qt part of `libdatoviz`. Qt owns the
window and event loop. Datoviz owns the rendering stack and renders into a Qt-created Vulkan
surface when Qt schedules an update.

The durable design note is
[`spec/scene/integration/QT_HOSTING.md`](../../spec/scene/integration/QT_HOSTING.md). The runnable
native examples live under `examples/qt/`.


## Build And Run

The Qt examples are optional. They are built only when Qt6 development packages are available:

```sh
just build
./build/examples/qt/hosted_qt_smoke 120
./build/examples/qt/hosted_qt_widgets
```

`hosted_qt_smoke` is the minimal contract smoke. `hosted_qt_widgets` embeds the hosted Datoviz
window in a Qt Widgets layout and lets widget callbacks mutate retained scene data.

The minimal PyQt source-tree example uses the raw `ctypes` FFI helpers and requires a PyQt6 build
with Qt Vulkan support:

```sh
PYTHONPATH=. python examples/python/qt/hosted_pyqt.py
```


## Ownership Model

Qt owns:

1. the `QWindow` or widget container;
2. the native event loop;
3. expose, update, timer, resize, input, and close events;
4. the `VkSurfaceKHR` returned by `QVulkanInstance::surfaceForWindow()`;
5. repaint scheduling, normally through `QWindow::requestUpdate()`.

Datoviz owns:

1. the `VkInstance` created after Qt-required instance extensions are supplied;
2. the GPU context, DRP2 runtime, canvas, and swapchain wrapper;
3. scene emission and rendering;
4. one-frame rendering through `dvz_view_render_once()`.

Qt adopts the Datoviz-created Vulkan instance with `QVulkanInstance::setVkInstance()`. Qt does not
own that instance; Datoviz destroys it when `dvz_app_destroy()` runs.


## Native Qt Flow

The native adapter follows this sequence:

1. query Qt/platform Vulkan instance extensions;
2. pass those extensions to `dvz_app_with_config()`;
3. get the Datoviz `VkInstance` with `dvz_app_vk_instance()`;
4. let Qt adopt that instance with `QVulkanInstance::setVkInstance()`;
5. create a Vulkan-capable `QWindow`;
6. get the window surface with `QVulkanInstance::surfaceForWindow()`;
7. create a Datoviz hosted view with `dvz_view_external_surface()`;
8. call `dvz_view_render_once()` from Qt's update path.

The adapter forwards host events through:

```c
dvz_view_emit_resize()
dvz_view_emit_pointer()
dvz_view_emit_wheel()
dvz_view_emit_key()
```

Datoviz request-frame callbacks should schedule a Qt update. They should not render immediately.


## Widget Embedding

For Qt Widgets, the current example uses `QWidget::createWindowContainer()` around the hosted
`QWindow`. The surrounding widgets remain ordinary Qt controls. Their callbacks mutate retained
Datoviz scene data, then request another Datoviz frame.

Use `examples/qt/hosted_qt_widgets.cpp` as the reference for this layout.


## Surface Cleanup

Before Qt destroys or recreates the native surface, release Datoviz's surface-dependent present
resources:

```c
dvz_view_release_external_surface(view);
```

The release call clears the request-frame callback, marks the surface unavailable, and runs one
render-once cleanup pass. The host remains responsible for destroying the borrowed `VkSurfaceKHR`.

A warning that the canvas surface is unavailable during this cleanup pass is expected. Vulkan
validation errors are not expected.


## PyQt Status

PyQt uses the same hosted contract as the native adapter. The raw `ctypes` boundary should use the
FFI convenience helpers instead of constructing `DvzWindowExternalSurfaceInfo` in Python:

```python
view = dvz.dvz_view_external_surface_ffi(
    app,
    figure,
    ctypes.c_void_p(instance),
    surface,
    framebuffer_width,
    framebuffer_height,
    scale,
    scale,
    False,
)

dvz.dvz_view_update_external_surface_ffi(
    view,
    ctypes.c_void_p(instance),
    surface,
    framebuffer_width,
    framebuffer_height,
    scale,
    scale,
    False,
)
```

These helpers accept primitive handle values and internally build the native
`DvzWindowExternalSurfaceInfo` record. C and C++ callers should continue to prefer the struct-based
`dvz_view_external_surface()` and `dvz_view_update_external_surface()` APIs.

Use `examples/python/qt/hosted_pyqt.py` as the minimal PyQt reference. It keeps Qt in charge of the
event loop, asks Qt for the native `VkSurfaceKHR` with `QVulkanInstance.surfaceForWindow()`, renders
from the Qt update path with `dvz_view_render_once()`, and calls
`dvz_view_release_external_surface()` before the Qt surface is destroyed.


## Limitations

1. The in-tree examples currently target Qt6.
2. A Vulkan-capable platform and Qt Vulkan support are required.
3. The Qt examples are optional build targets and may be skipped when Qt development packages are
   unavailable.
4. `QVulkanWindow` is not the recommended path because it overlaps with Datoviz ownership of the
   Vulkan device, queues, swapchain, and command buffers.
