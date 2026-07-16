# Embed in Qt

Integrate Datoviz with a Qt application that owns the window and event loop.

Qt hosting is a supported optional-provider path in v0.4. It is an advanced native integration to
configure, but it does not make Qt a dependency of the base Datoviz library. Build or supply the Qt
provider only when the host application needs it.

!!! info "At a glance"

    - **Status:** Supported optional native provider; not part of the base dependency set.
    - **Languages:** C++/Qt and Python/PyQt6 through the optional bridge.
    - **Prerequisites:** Matching Qt development/runtime libraries, Vulkan hosting support, and a built/discoverable bridge.
    - **Result:** Qt owns the window/event loop while Datoviz renders one hosted view and receives forwarded events.

## Task workflow

Let Qt own the application shell, menus, widgets, and native window. Let Datoviz render the figure
inside the hosted view. Use the external-surface or viewport integration path closest to the
platform you target.

## Minimal workflow

1. Let the host toolkit create or expose the native surface.
2. Create Datoviz with any required host Vulkan instance extensions.
3. Attach a Datoviz view to the external surface or hosted viewport path.
4. Let the host event loop drive resize, input, frame requests, and `dvz_view_render_once()`.

Use the GLFW external-surface example as the closest maintained native embedding reference.

This page describes an ownership protocol, not a copy-pasteable single function. Start from the
linked Qt example because instance-extension discovery, surface creation, event forwarding, and
teardown must agree with the host platform.


## Native Qt examples

The native Qt examples are built only when Qt development packages are available. Enable the
optional bridge explicitly when validating the provider:

```sh
DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build
./build/examples/qt/hosted_qt_smoke 120
./build/examples/qt/qt_hosting --smoke-ms 1000
```

`hosted_qt_smoke` is the minimal contract smoke. `qt_hosting` embeds the hosted Datoviz
window in a Qt Widgets layout and lets widget callbacks mutate retained scene data.


## Ownership contract

| Owner | Responsibilities |
| --- | --- |
| Qt | Native window, event loop, widget state, and repaint scheduling. |
| Datoviz | Graphics context, scene state, hosted view, and rendering. |

Qt adopts the Datoviz-created Vulkan instance with `QVulkanInstance::setVkInstance()`. Qt does not
own that instance; Datoviz destroys it when `dvz_app_destroy()` runs.

The Qt surface is borrowed by Datoviz. Qt remains responsible for creating and destroying the
surface; Datoviz renders into it while it is available.


## Event forwarding

Forward host events through the public hosted app API:

| Host event | Datoviz call |
| --- | --- |
| resize | `dvz_view_emit_resize()` |
| mouse move, press, release | `dvz_view_emit_pointer()` |
| wheel | `dvz_view_emit_wheel()` |
| key press, release, repeat | `dvz_view_emit_key()` |

Qt should schedule rendering with `QWindow::requestUpdate()`, then call `dvz_view_render_once()`
from the update path. Widget callbacks should mutate retained scene state, request a frame, and let
Datoviz render through the existing view.


## Surface teardown

Before Qt destroys or recreates the native surface, release Datoviz's presentation resources:

1. Clear the request-frame callback with `dvz_view_set_request_frame_callback(view, NULL, NULL)`.
2. Update the hosted view with a null surface tuple.
3. Call `dvz_view_render_once()` once so Datoviz observes the unavailable surface and releases the
   present swapchain.

The Qt adapter handles `QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed`. A warning that the
surface is unavailable during this cleanup pass is expected; graphics validation errors are not
expected.


## PyQt hosting

The Python hosted path uses the optional `datoviz_qtbridge` provider. The base Python wheel does not
load or link Qt by itself. Build the bridge with `DVZ_ENABLE_QT_BRIDGE=ON` or supply a compatible
bridge library separately. The loader checks the package directory, its `.libs` directory, common
source-checkout build directories, and the platform library search path. Set
`DATOVIZ_QTBRIDGE_LIBRARY` only when discovery needs an explicit bridge path.

For an installed package or a bridge discoverable through the platform library path, run:

```sh
python -m datoviz.qt
```

For an isolated provider smoke with PyQt6, use:

```sh
uv run --isolated --with PyQt6 python -m datoviz.qt
```

Keep failure diagnostics explicit: a missing bridge library, unsupported PyQt/PySide binding,
missing `QVulkanInstance` features, or Qt runtime mismatch should fail before rendering starts.

For a source-checkout bridge that is not otherwise discoverable, pass its exact path:

```sh
DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so \
python examples/python/qt/hosted_pyqt.py --smoke-ms 1000
```

The usual source-build filenames are:

| Platform | Bridge path or filename |
| --- | --- |
| Linux | `build/qtbridge/libdatoviz_qtbridge.so` |
| macOS | `build/qtbridge/libdatoviz_qtbridge.dylib` |
| Windows | `build/qtbridge/datoviz_qtbridge.dll` or `build/qtbridge/libdatoviz_qtbridge.dll` |

Prefer normal package or platform-library discovery and verify it with `python -m datoviz.qt`.
Use `DATOVIZ_QTBRIDGE_LIBRARY` only when the bridge is outside those discovery locations.

System PyQt packages may lack the required Vulkan instance API even when PyQt imports successfully.
Treat that as an environment limitation, not as a Datoviz rendering failure.


## Important details

Qt embedding is a host-integration task. Do not create a second rendering stack; adapt the existing
Datoviz hosted view path.

Qt and Datoviz must agree on Vulkan instance extensions before the Datoviz app is created. Query
Qt's required extensions first, then pass them into Datoviz app creation.

Destroy Datoviz presentation/app state while the borrowed Qt surface and adopted instance context
are still valid. Then allow Qt to finish destroying its surface/window objects.

## Common mistakes

- Letting both Qt and Datoviz own the same native graphics handle.
- Handling resize in the UI but not notifying the Datoviz view.
- Copying Datoviz implementation code instead of using public integration surfaces.
- Destroying or recreating the Qt surface without giving Datoviz a cleanup render pass.
- Rendering directly from arbitrary widget callbacks instead of scheduling a frame.
- Treating PyQt import success as proof that the required Vulkan hosting API is available.

## See also

- [Open an interactive window](create-a-window.md)
- [Handle input events](input-events.md)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Complete and related examples"

    - Canonical complete example: [Qt Hosting](../examples/gallery/advanced/advanced_qt_hosting.md) - Source: `examples/qt/qt_hosting.cpp`
    - [External Surface GLFW](../examples/gallery/advanced/advanced_external_surface_glfw.md) - Source: `examples/c/advanced/external_surface_glfw.c`
    - [GUI Viewport](../examples/gallery/features/features_gui_viewport.md) - Source: `examples/c/features/gui_viewport.c`
