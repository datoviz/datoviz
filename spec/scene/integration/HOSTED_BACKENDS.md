# Hosted Backends And External Event Loops

This document records the intended direction for Qt, Python console, IPython, Jupyter, SDL, Tk, and
other host-owned integration paths.

The main rule is that Datoviz should provide a backend-agnostic hosted rendering contract. Qt should
be the first serious consumer of that contract, not a special case that pulls Qt into the core
library.


## Status

This document is an architecture note for the next app/window/runtime refactor pass.

The current codebase already has the low-level ingredient needed for native host surfaces:

1. `DVZ_BACKEND_WRAP` is implemented in the window module,
2. `DvzWindowExternalSurfaceInfo` carries a borrowed or owned `VkSurfaceKHR`,
3. `dvz_window_wrap_set_required_extensions()` lets a host provide required Vulkan instance
   extensions before instance creation,
4. `dvz_window_wrap_attach_surface()`, `dvz_window_wrap_update_surface()`, and
   `dvz_window_wrap_detach_surface()` bind, resize, replace, lose, or detach the surface,
5. the completed external-surface phase records Qt/PyQt as the first expected consumer without
   adding a Qt dependency to Datoviz core.

The remaining work is above that boundary: an app/runtime API that lets host frameworks own the
event loop and call Datoviz rendering at the right time.


## Why Qt Should Stay Out Of Core

Qt integration should be optional integration code outside `libdatoviz`.

Outside core means outside the base shared library target, not necessarily outside the repository.
Possible locations include `extras/qt/`, `integrations/qt/`, a companion `datoviz-qt` package, or
the Python extra `datoviz[qt]`.

Reasons:

1. `libdatoviz` is a C library; Qt is a C++ framework with its own ABI, build system hooks, and
   version matrix.
2. Qt requires additional CMake/moc/plugin/deployment machinery that should not affect headless,
   GLFW, server, WebGPU, or basic Python users.
3. Qt owns its event loop and often its top-level windows; that ownership should remain a host
   concern instead of being embedded into scene semantics.
4. Datoviz already has `DVZ_BACKEND_WRAP`, so a host can provide Vulkan surfaces without Qt being
   linked by Datoviz.
5. Keeping the Qt adapter thin makes the same contract usable by SDL, wx, Tk, native application
   shells, Python consoles, and notebook frontends.

Core may still expose generic hooks needed by Qt. It should not expose `QWidget`, `QWindow`,
`QVulkanWindow`, `QObject`, or Qt event types in public Datoviz headers.


## Hosted Rendering Contract

The current `DvzApp` API is convenient for Datoviz-owned offscreen and GLFW loops, but it combines
too many responsibilities for external hosts:

1. GPU context creation,
2. required WSI extension selection,
3. window creation,
4. canvas creation,
5. scene to DRP2 runtime setup,
6. event polling,
7. frame-loop ownership.

Power users need a lower-level API where Datoviz can be stepped by an external loop.

The target concepts are:

1. `DvzRenderHost` or equivalent: owns GPU context, DRP2 runtime, and backend capabilities.
2. `DvzRenderTarget`: describes where frames go: offscreen image, Datoviz/GLFW window, external
   Vulkan surface, live-image sink, future WebGPU/browser target, or shared-texture target.
3. `DvzView`: binds a figure to a render target and owns per-view frame state.
4. `dvz_view_render_once(view)`: render exactly one frame if the target is ready.
5. `dvz_view_resize(view, framebuffer_w, framebuffer_h, logical_w, logical_h, scale_x, scale_y)`:
   update dimensions and device-pixel-ratio state.
6. `dvz_view_input_router(view)`: expose the router that host adapters use for normalized input.
7. a request-frame callback: Datoviz or scene mutations can ask the host to schedule a future frame,
   while the host still decides when that frame happens.

This split keeps the scene layer independent from window systems and lets the app/runtime layer be
hosted by many event-loop models.


## Event Loop Ownership

Datoviz should support both modes:

1. **Datoviz-owned loop**: examples, simple C apps, and GLFW can keep using a convenience loop such
   as `dvz_app_run()`.
2. **Host-owned loop**: Qt, SDL, Tk, IPython, Jupyter, and custom engines call Datoviz from their own
   loop through render-once, resize, input, and request-frame hooks.

The host-owned loop contract should not require polling through Datoviz. A host can poll itself and
then call the Datoviz APIs that correspond to what happened.

Typical host-owned loop:

1. host receives an expose, timer, animation, input, or data-change event,
2. host updates Datoviz size/surface/input state,
3. host calls a Datoviz render-once function when the target is drawable,
4. Datoviz emits scene work through the current scene -> DRP2 -> runtime path,
5. Datoviz returns without blocking the host loop indefinitely.


## Qt Direction

Qt should be an optional adapter over the hosted contract.

Preferred first native path:

1. the Qt adapter creates or receives a Vulkan-capable `QWindow`,
2. the adapter provides the platform WSI instance extensions before Datoviz creates its GPU
   context, or Qt adopts the Datoviz `VkInstance` when Qt is responsible for surface creation,
3. Qt creates or returns a `VkSurfaceKHR` for the window,
4. the adapter creates a Datoviz external-surface render target using `DVZ_BACKEND_WRAP`,
5. Qt owns expose/update/timer events and calls Datoviz render-once,
6. Qt resize, device-pixel-ratio, and surface-loss events call the Datoviz resize/update/detach
   hooks,
7. Qt mouse, wheel, keyboard, focus, and modifier events are translated into the Datoviz input
   router.

Avoid making `QVulkanWindow` the first core target. `QVulkanWindow` manages a Vulkan device, queues,
command buffers, depth-stencil images, and swapchain resources, which overlaps with the current
Datoviz canvas/vklite ownership model. A simpler `QWindow` plus borrowed `VkSurfaceKHR` path matches
`DVZ_BACKEND_WRAP` more directly.

The Qt adapter can still live in-tree as an optional target. It should dynamically link against the
system Qt installation or the Python binding runtime rather than vendoring Qt.


## Python Console And IPython

Terminal Python and terminal IPython should be treated as host-loop integrations, not as separate
rendering backends.

For stock Python:

1. a convenience helper may create a backend adapter and pump it from timers or explicit calls,
2. blocking calls should remain opt-in,
3. advanced users should be able to call render-once manually.

For IPython terminal:

1. IPython already has GUI event-loop integration through input hooks,
2. `%gui qt` should allow Qt-owned Datoviz windows to remain responsive while the prompt waits,
3. Datoviz should not implement an IPython-specific renderer; it should provide a Qt/SDL/etc.
   adapter that IPython can keep alive through the corresponding GUI integration,
4. a custom Datoviz input hook is only needed if Datoviz owns a nonstandard event loop, which this
   design tries to avoid.

In practice, the best IPython terminal path is probably:

1. `datoviz[qt]` creates a Qt adapter,
2. IPython `%gui qt` runs the Qt event loop while waiting for input,
3. Datoviz renders when Qt schedules updates.


## Jupyter Direction

Jupyter is different from terminal IPython because the visible UI is usually a browser frontend.

Recommended staged paths:

1. **Offscreen image display**: render offscreen, copy RGBA/PNG, display in the notebook. This is the
   simplest path and useful for deterministic previews.
2. **Widget image stream**: use a custom widget or comm channel to push frames and receive pointer
   events. This supports interaction without native window embedding.
3. **Browser/WebGPU path**: replay a narrow DRP2 subset in the browser. This should build on the
   WebGPU feasibility lane and keep scene semantics shared.
4. **Shared GPU texture**: possible later for local desktop stacks, but platform-specific and not a
   first portable notebook target.

Native Qt/Vulkan embedding is not the natural default inside JupyterLab or notebook frontends. The
browser-facing path should not depend on Qt.


## SDL, Tk, wx, And Custom Engines

These integrations should use the same hosted contract:

1. provide required native surface extensions before GPU context creation when presenting through
   Vulkan,
2. provide or update the native surface handle when available,
3. map host resize/DPI events to Datoviz size and scale updates,
4. map host input events to the Datoviz input router,
5. call render-once from the host loop,
6. respond to Datoviz request-frame notifications by scheduling a host update.

Some toolkits may not expose a robust cross-platform Vulkan child-surface path. Those adapters can
still use offscreen RGBA/image presentation first.


## API Implications

The next implementation pass should prefer generic APIs over Qt-specific APIs.

High-value additions:

1. an app or render-host creation config that accepts backend-required instance extensions before
   GPU context creation,
2. a public external-surface app/view constructor above `DVZ_BACKEND_WRAP`,
3. a render-once API for host-owned loops,
4. resize/scale/surface-loss update APIs that do not require Datoviz event polling,
5. request-frame notification hooks,
6. Python binding coverage for the same generic APIs,
7. examples proving one host-owned loop without baking that host into core.

Avoid:

1. `dvz_app_window_qt()` as the only path,
2. Qt symbols in core public headers,
3. a second renderer path parallel to scene -> frame plan -> DRP2 -> runtime,
4. backend-native handles leaking into scene APIs,
5. a notebook path that depends on native desktop windows.


## Suggested Implementation Order

1. Add a hosted render/view API over the current `DvzAppWindow` draw path without changing scene
   semantics.
2. Add a C example that manually steps a Datoviz view from a simple custom loop.
3. Add a wrap-target example using an externally supplied surface if a portable test fixture is
   practical.
4. Add an optional Qt adapter as the first real hosted backend.
5. Add Python convenience wrappers for offscreen image display and hosted Qt.
6. Add IPython terminal guidance around `%gui qt` and explicit render-once.
7. Add a Jupyter widget/offscreen display path.
8. Use lessons from Qt and Jupyter to refine the WebGPU/browser lane.


## Boundary Summary

1. Scene owns visualization semantics.
2. DRP2 carries backend-agnostic rendering commands.
3. Runtime/canvas executes rendering against concrete targets.
4. Host adapters own event loops, native widgets, and toolkit-specific input.
5. `libdatoviz` should stay free of heavy toolkit dependencies.
6. Optional adapters should be small, dynamically linked, and replaceable.
