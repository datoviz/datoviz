# Datoviz Qt hosted examples

This directory contains optional Qt examples that exercise the external host-window contract without adding Qt to `libdatoviz`.

The examples are built only when Qt development packages are found:

```bash
just build
./build/examples/qt/hosted_qt_smoke 120
./build/examples/qt/qt_hosting
./build/examples/qt/qt_hosting --png
```

`hosted_qt_smoke` is the minimal contract smoke. `qt_hosting` embeds the hosted Vulkan window in a normal Qt Widgets layout and lets widget callbacks mutate retained scene data. The `--png` mode deterministically composes a 1024x720 Datoviz offscreen readback with a 256x720 Qt-rendered controls pane, avoiding window-manager and compositor screenshot behavior. It uses `DVZ_CAPTURE_DIR` and `DVZ_CAPTURE_BASENAME` when set.

## Ownership contract

Qt owns the native window and event loop:

- Qt creates the `QWindow`.
- Qt sets `QSurface::VulkanSurface`.
- Qt creates or returns the `VkSurfaceKHR` with `QVulkanInstance::surfaceForWindow()`.
- Qt forwards resize, pointer, wheel, and key events to Datoviz.
- Qt schedules repaint with `QWindow::requestUpdate()`.

Datoviz owns the rendering stack:

- Datoviz creates the `VkInstance` after the Qt-required instance extensions are supplied.
- Datoviz owns the GPU context, DRP2 runtime, canvas, swapchain wrapper, and scene emission.
- Datoviz renders one frame when Qt calls `dvz_view_render_once()`.

Qt adopts the Datoviz-created instance with `QVulkanInstance::setVkInstance()`. Qt does not own that instance; Datoviz destroys it when `dvz_app_destroy()` runs.

## Surface lifecycle

The Qt surface is borrowed by Datoviz. The adapter passes `owned_by_datoviz = false` in `DvzWindowExternalSurfaceInfo`, so Datoviz must not destroy the `VkSurfaceKHR`.

Before Qt destroys or recreates the native surface, the adapter must release Datoviz's swapchain:

1. Clear the request-frame callback with `dvz_view_set_request_frame_callback(win, NULL, NULL)`.
2. Update the hosted window with a NULL surface tuple.
3. Call `dvz_view_render_once()` once so Datoviz observes the unavailable surface and cleans up the present swapchain.

`DvzQtHostedWindow` does this in `release_surface()` and also handles Qt `QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed`.

The cleanup pass may log that the canvas surface is unavailable. That warning is expected in these examples; Vulkan validation errors during that path are not expected.

## Event forwarding

The adapter forwards Qt events through the hosted app API:

- resize: `dvz_view_emit_resize()`
- mouse move/press/release: `dvz_view_emit_pointer()`
- wheel: `dvz_view_emit_wheel()`
- key press/release/repeat: `dvz_view_emit_key()`

Wheel events are normalized to abstract wheel steps. Qt `angleDelta()` is divided by `120.0`; when only `pixelDelta()` is available, that is also divided by `120.0` before any example-level sensitivity is applied.
