# Qt Hosting And PyQt FFI

Status: design note for the hosted Qt path, PyQt FFI convenience layer, and optional Qt host
bridge.

This note records the Qt-specific policy on top of
[HOSTED_BACKENDS.md](HOSTED_BACKENDS.md). The durable rule remains that Qt is an optional host
adapter over the generic hosted rendering contract, not a dependency or alternate rendering path in
`libdatoviz`.

Implementation handoff for the v0.4 PyQt bridge lives in
[QT_HOST_BRIDGE.md](QT_HOST_BRIDGE.md).


## Goals

1. Support Qt-owned and PyQt-owned event loops without handing scheduling to Datoviz.
2. Render into Qt-created Vulkan surfaces through the existing scene -> DRP2 -> vklite/canvas/app
   runtime path.
3. Keep Qt out of public Datoviz core headers and build requirements.
4. Give C/C++ adapters a precise struct-based API and FFI adapters a stable primitive-handle API.
5. Make lifecycle and ownership explicit enough that surface loss, resize, and widget destruction do
   not rely on undefined Vulkan ownership.


## Non-Goals

1. Do not add a Qt dependency to `libdatoviz`.
2. Do not make `QVulkanWindow` the primary integration target.
3. Do not let Qt own the Vulkan device, queues, swapchain, command buffers, or Datoviz frame
   resources.
4. Do not create a parallel renderer, presentation layer, frame stream, or Vulkan wrapper.
5. Do not recreate the v0.3 Python plotting API in Datoviz. PyQt hosting is a low-level integration
   surface; high-level Python application and plotting APIs remain GSP/VisPy2 scope.


## Existing Hosted Contract

The current C surface already contains the core pieces needed by host-owned UI toolkits:

1. `dvz_app_with_config()` accepts required Vulkan instance extensions before Datoviz creates the
   GPU context.
2. `dvz_app_vk_instance()` returns the borrowed `VkInstance` that a host can use to create or adopt a
   native Vulkan surface.
3. `DvzWindowExternalSurfaceInfo` describes a host-created `VkSurfaceKHR`, framebuffer extent,
   content scale, and surface ownership.
4. `dvz_view_external_surface()` creates a hosted `DvzView` above `DVZ_BACKEND_WRAP`.
5. `dvz_view_update_external_surface()` updates resize, scale, surface replacement, or temporary
   surface loss.
6. `dvz_view_release_external_surface()` releases Datoviz's surface-dependent present resources
   before the host destroys or recreates the surface.
7. `dvz_view_render_once()` and `dvz_app_render_once()` let the host drive rendering from its own
   event loop.
8. `dvz_view_emit_resize()`, `dvz_view_emit_pointer()`, `dvz_view_emit_wheel()`, and
   `dvz_view_emit_key()` inject normalized host events.
9. `dvz_view_set_request_frame_callback()` lets Datoviz ask a passive host to schedule a repaint.

Native C/C++ hosted adapters include `<datoviz/app_interop.h>` for the Vulkan-facing app/view
helpers and `<datoviz/window/backend.h>` for `DvzWindowExternalSurfaceInfo`. The default
`<datoviz/app.h>` header intentionally avoids exposing Vulkan SDK types.


## Native Qt Adapter

The in-tree native Qt examples live under `examples/qt/` and should remain optional build targets.
They validate the intended architecture without adding Qt to core Datoviz:

1. `hosted_qt_smoke.cpp` is the minimal host-owned Qt loop smoke.
2. `qt_hosting.cpp` embeds a hosted Datoviz Vulkan window inside a normal Qt Widgets layout
   and lets widget callbacks mutate retained scene data.
3. `hosted_qt_adapter.cpp` is the small adapter layer translating Qt window, surface, input, and
   update events into generic Datoviz hosted-view calls.

The preferred native path is `QWindow` plus `QSurface::VulkanSurface` plus a borrowed
`VkSurfaceKHR` obtained from `QVulkanInstance::surfaceForWindow()`. Qt adopts the Datoviz-created
`VkInstance` with `QVulkanInstance::setVkInstance()`. Datoviz still owns the GPU context, DRP2
runtime, canvas, swapchain wrapper, and scene emission.

`QVulkanWindow` is not the preferred first target because it manages a Vulkan device, queues,
command buffers, depth-stencil images, and swapchain resources. That overlaps with Datoviz's runtime
ownership and would create a second presentation path.


## PyQt Binding Problem

PyQt can in principle follow the same hosted contract as native Qt, but a direct `datoviz.raw`
binding has an awkward boundary:

1. Python needs the Datoviz-created `VkInstance` so Qt can adopt it.
2. Qt returns a `VkSurfaceKHR` for the `QWindow`.
3. The native C API expects those handles inside `DvzWindowExternalSurfaceInfo`.
4. `DvzWindowExternalSurfaceInfo` is Datoviz-owned, but its fields include Vulkan SDK types:
   `VkInstance`, `VkSurfaceKHR`, and `VkExtent2D`.

Generated `ctypes` layouts are appropriate for small, stable Datoviz-owned POD records whose ABI is
validated. They are less attractive for foreign SDK handles and host-window bridge structs because
they expose low-level Vulkan layout policy to Python examples and documentation. PyQt users should
not need to construct a Vulkan-flavored C struct by hand just to host a Datoviz view.

There is also a binding gap in the currently tested PyQt6 wheels: `QVulkanInstance` and
`QWindow.setVulkanInstance()` are present, but `QVulkanInstance::setVkInstance()` and
`QVulkanInstance::vkInstance()` are not exposed to Python. Qt's C++ API supports those methods, and
Datoviz needs `setVkInstance()` so Qt adopts the Datoviz-created Vulkan instance.

The v0.4 route is therefore a separate optional native Qt bridge loaded by `datoviz.qt`, not a Qt
dependency in `libdatoviz`. See [QT_HOST_BRIDGE.md](QT_HOST_BRIDGE.md).


## FFI Helper Decision

Prefer adding a pair of thin FFI convenience functions over forcing PyQt examples to instantiate
`DvzWindowExternalSurfaceInfo` directly:

```c
DVZ_EXPORT DvzView* dvz_ffi_view_external_surface(
    DvzApp* app, DvzFigure* figure, void* instance, uint64_t surface,
    uint32_t framebuffer_width, uint32_t framebuffer_height, float scale_x, float scale_y,
    bool owned_by_datoviz);

DVZ_EXPORT int dvz_ffi_view_update_external_surface(
    DvzView* view, void* instance, uint64_t surface, uint32_t framebuffer_width,
    uint32_t framebuffer_height, float scale_x, float scale_y, bool owned_by_datoviz);
```

These functions should be exact wrappers around the struct API. They should build a
`DvzWindowExternalSurfaceInfo` internally and call `dvz_view_external_surface()` or
`dvz_view_update_external_surface()`.

The `dvz_ffi_*` prefix is intentional:

1. it marks the functions as foreign-function-interface conveniences;
2. it avoids implying that they are simpler or preferred for native C callers;
3. it is not Python-specific, so the same functions can serve `ctypes`, CFFI, Rust, Zig, or other
   host-language adapters;
4. it keeps the struct-based API as the canonical C/C++ integration path.

C and C++ callers should prefer `DvzWindowExternalSurfaceInfo` unless they are implementing a
binding or host-language adapter. Public documentation should state this preference explicitly.


## Ownership And Lifecycle

Qt or PyQt owns:

1. the `QWindow` or widget container;
2. the native event loop;
3. expose, update, timer, resize, input, and close events;
4. the `VkSurfaceKHR` unless `owned_by_datoviz` is explicitly true;
5. repaint scheduling through `QWindow::requestUpdate()` or the PyQt equivalent.

Datoviz owns:

1. the `VkInstance` created after the host-provided WSI extensions are supplied;
2. GPU device selection and GPU context lifetime;
3. DRP2 runtime state;
4. canvas and present/swapchain resources built on top of the borrowed surface;
5. scene emission and per-frame rendering.

Before Qt destroys or recreates a surface, the adapter must call
`dvz_view_release_external_surface()`. This clears Datoviz's request-frame callback, marks the
surface unavailable, and runs one render-once cleanup pass so present resources release borrowed
surface-dependent objects. The host remains responsible for destroying a borrowed `VkSurfaceKHR`.

Resize or device-pixel-ratio changes should update both the external-surface tuple and the input
router dimensions. Surface loss should be represented by a NULL/zero instance and surface tuple
until the host provides a valid surface again.


## Event Forwarding

Qt event forwarding should remain a thin translation layer:

1. resize events call `dvz_view_emit_resize()`;
2. mouse move, press, and release events call `dvz_view_emit_pointer()`;
3. wheel events call `dvz_view_emit_wheel()` with normalized abstract wheel steps;
4. key press, release, and repeat events call `dvz_view_emit_key()`;
5. Datoviz request-frame callbacks schedule a Qt update, but do not render immediately.

Adapters may use `dvz_view_input()` directly when they need lower-level input-router access, but
the `dvz_view_emit_*()` API should be the documented default.


## Validation

Documentation-only changes need:

```sh
git diff --check
git status --short
```

Code changes to the hosted C API or Qt examples should use the narrowest relevant subset:

```sh
just build
./build/examples/qt/hosted_qt_smoke 120
./build/examples/qt/qt_hosting --smoke-ms 1000
python -m datoviz.qt
python examples/python/qt/hosted_pyqt.py --smoke-ms 1000
just ctypes
just ctypes-check
just ctypes-python-smoke
```

Graphics validation should also watch for Vulkan validation errors during surface release and
surface-loss cleanup. A warning that the canvas surface is unavailable during the cleanup pass is
expected; validation errors are not.


## Open Questions

1. Whether the Python extra should be named `datoviz[qt]`, a separate `datoviz-qt` package, or kept
   as a source-tree example for v0.4.
2. How much Linux platform policy to expose in public docs for Wayland versus XCB.
3. Whether CI can reliably run the Qt smoke in a Vulkan-capable environment.
4. Whether later FFI bindings should also expose the struct layout after the handle helper is proven.
