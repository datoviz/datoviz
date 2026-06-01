# Embed In Qt

Status: advanced/experimental.

Datoviz can be hosted inside a Qt application without making Qt part of `libdatoviz`. Qt owns the
window and event loop. Datoviz owns the rendering stack and renders into a Qt-created Vulkan
surface when Qt schedules an update.

The durable design note is
[`spec/scene/integration/QT_HOSTING.md`](../../spec/scene/integration/QT_HOSTING.md). The runnable
native examples live under `examples/qt/`.


## Build And Run

The native C++ Qt examples are optional. They are built only when Qt6 development packages are
available:

```sh
just build
./build/examples/qt/hosted_qt_smoke 120
./build/examples/qt/hosted_qt_widgets
```

`hosted_qt_smoke` is the minimal contract smoke. `hosted_qt_widgets` embeds the hosted Datoviz
window in a Qt Widgets layout and lets widget callbacks mutate retained scene data.

The Python Qt adapter is pure Python and does not require Qt development headers, Qt development
libraries, or Qt-aware Datoviz compilation. Users install PyQt6 with their Python, Conda, or system
package manager; that PyQt6 package supplies or depends on the needed Qt runtime. The installed
PyQt6 package must expose Qt Vulkan support, including `QVulkanInstance`.

The PyQt source-tree example uses `datoviz.qt.DatovizWidget`:

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

Use `examples/qt/hosted_qt_widgets.cpp` as the native C++ reference and
`examples/python/qt/hosted_pyqt.py` as the PyQt reference.


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


## PyQt Flow

PyQt users should build the Datoviz scene normally, bind controllers to panels normally, and then
give the scene and figure to `DatovizWidget`:

```python
from datoviz.qt import DatovizWidget
import datoviz.raw as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

controller = dvz.dvz_panzoom(scene, None)
dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

widget = DatovizWidget(scene, figure)
```

`DatovizWidget` owns the Qt hosting plumbing: Vulkan extension discovery, `QVulkanInstance`
adoption, Qt surface wrapping, Qt event forwarding, Datoviz request-frame scheduling, and surface
cleanup. The hosted view automatically connects the view input router to the figure's current
panels, so panel-bound controllers receive Qt mouse, wheel, and key input without Qt-specific
controller code.

The low-level raw `ctypes` FFI helpers remain available for binding authors:
`dvz_view_external_surface_ffi()` and `dvz_view_update_external_surface_ffi()` accept primitive
handle values and internally build the native `DvzWindowExternalSurfaceInfo` record. C and C++
callers should continue to prefer the struct-based `dvz_view_external_surface()` and
`dvz_view_update_external_surface()` APIs.


## Limitations

1. The in-tree examples currently target Qt6.
2. A Vulkan-capable platform and Qt Vulkan support are required.
3. The native C++ Qt examples are optional build targets and may be skipped when Qt development
   packages are unavailable. The Python adapter does not need those development packages.
4. `QVulkanWindow` is not the recommended path because it overlaps with Datoviz ownership of the
   Vulkan device, queues, swapchain, and command buffers.
