# External UI Integration

This document defines how external UI frameworks should interact with the future scene layer.

An external UI framework may be ImGui, a native widget toolkit, a web-side control surface, or any
other app-level interface that reads and mutates scene-owned semantic state.


## Normative Status

This document is normative for the boundary between scene semantics and external UI.

It should be read together with:

1. `api/API_DESIGN.md` for the scene-owned state surface,
2. `interaction/CONTROLLERS.md` for scene-native interaction behavior,
3. `pipeline/FRAME_LIFECYCLE.md` for per-frame ordering,
4. `core/RUNTIME_BOUNDARY.md` for what must remain below the scene semantic layer.


## Core Rule

External UI is a client of scene state, not part of the scene object model.

That means:

1. tree views, sliders, buttons, menus, inspectors, and similar widgets are not scene primitives,
2. these widgets may read and mutate scene-owned semantic state,
3. the scene remains the owner of the resulting meaning such as selection, visibility, opacity,
   active slice, filter choice, probe state, or panel configuration.


## What External UI May Do

An external UI layer may:

1. inspect scene objects and scene-owned semantic state,
2. mutate scene-owned state through setters, descriptors, or app-level commands,
3. request redraw after mutating scene-visible state,
4. observe scene-produced picking, probe, validation, and diagnostic results,
5. display app-level controls that coordinate several scene objects at once.


## What External UI Must Not Be

External UI should not be modeled as:

1. a visual family,
2. an annotation family,
3. a backend-specific scene escape hatch,
4. a second planner parallel to scene.

If an app uses ImGui, the preferred interpretation is:

1. ImGui is app-owned native UI,
2. scene remains the semantic producer of visualization state and `FramePlan`,
3. the runtime remains the execution service for scene work.


## Relationship To Controllers

Scene controllers and external UI may coexist, but they play different roles.

The preferred split is:

1. scene controllers own panel-native interaction such as camera navigation, picking-driven
   selection, hover, and linked-panel behaviors,
2. external UI owns widget interaction such as tree toggles, filter selectors, numeric inspectors,
   and tool panels,
3. both mutate the same scene-owned semantic state when they affect the same concept.

For example:

1. clicking a region in a panel may update selected-region state through a scene controller,
2. clicking a region entry in an ImGui tree may update that same selected-region state through
   app-level UI code.

The state should remain single-sourced at the scene level.


## Controller Inspector Widgets

Controller inspector widgets are a recommended external-UI pattern.

Examples include:

1. a panzoom widget exposing pan, zoom, visible domain, and reset controls;
2. an arcball widget exposing rotation, zoom, pan center, and reset controls;
3. a turntable widget exposing yaw, pitch, distance, pivot, and clamp controls;
4. a fly-camera widget exposing position, yaw, pitch, roll, movement speed, and pivot controls;
5. a camera widget exposing eye, target, up vector, FOV, near plane, and far plane.

These widgets are not scene primitives. They are app-owned inspectors that read and mutate
scene-owned controller state through public controller APIs.

The preferred implementation path is:

1. external UI reads a controller state snapshot;
2. external UI displays scalar/vector controls;
3. user edits are applied through controller setters or state-update APIs;
4. the controller marks the same invalidation scopes as an equivalent input gesture;
5. the app requests redraw.

The widget must not own camera math, write panel matrices directly, or bypass controller
invalidation. A controller inspector is equivalent to a typed editing surface for the controller,
not a second controller implementation.


## Input Routing

The spec should allow an app-level event policy in which external UI receives input before scene
controllers when the app chooses that behavior.

The preferred native-app pattern is:

1. raw runtime input arrives,
2. the external UI layer consumes the event first if appropriate,
3. unconsumed or forwarded input is translated into scene-level events,
4. scene controllers process those events and mutate scene state.

This keeps widget focus, typing, dragging, and menu interaction out of scene-native controller
policy.


## Rendering Relationship

External UI rendering is outside the scene plan.

The scene spec should assume:

1. scene builds one scene-level `FramePlan`,
2. runtime executes that plan,
3. an app may optionally render an external UI overlay after scene execution and before present,
4. this overlay rendering does not redefine scene semantics or bypass scene validation and planning.

The exact overlay implementation may be native and backend-specific.
That implementation detail belongs below the scene semantic layer.


## DRP2 Boundary

External UI should not be required to appear as scene-emitted DRP2.

The current preferred direction is:

1. DRP2 carries scene-planned visualization work,
2. app-owned UI overlays may use a separate native rendering path when the runtime supports it,
3. future generic retained UI systems, if desired later, should be specified separately rather than
   smuggled into scene through an ImGui-shaped shortcut.


## Examples

Good fits for external UI:

1. an ImGui tree controlling atlas-region visibility and opacity,
2. a filter combo box selecting the active slice-processing mode,
3. a tool panel showing the latest picked world coordinates and sampled value,
4. an inspector editing panel layout or camera presets.

Good fits for scene-native semantics instead:

1. crosshairs,
2. probe labels anchored inside a panel,
3. colorbars and legends,
4. pickable region highlights,
5. camera navigation.


## Rule Summary

1. external UI is app-owned and widget-oriented,
2. scene owns semantic visualization state,
3. scene controllers own panel-native interaction,
4. runtime executes scene work,
5. optional external UI overlay rendering may happen after scene execution and before present.


---

## Dear ImGui Integration

Dear ImGui is bundled with Datoviz and is the primary supported external UI framework.


### v0.3 Architecture (Reference)

In v0.3, ImGui is integrated as follows:

1. **Separate Vulkan render pass** with `LOAD_OP_LOAD` — preserves scene output, ImGui draws
   on top without clearing. Framebuffers point to the same swapchain images as the scene pass.
2. **Two command buffers, one submit** — scene CB records first, ImGui CB records after,
   both submitted together. ImGui is always on top.
3. **Hard-coded to Vulkan** — uses `ImGui_ImplVulkan` directly, bypassing DRP2.
4. **Hard-coupled to GLFW** — uses `ImGui_ImplGlfw` for input.
5. **Implicit ordering** — the loop submits the ImGui CB after the scene CB with no explicit
   contract in the FramePlan.
6. **Offscreen texture binding** — `dvz_gui_image` calls `ImGui_ImplVulkan_AddTexture`
   directly with a Vulkan image view.
7. **Panel binding** — `dvz_panel_gui(panel, title)` links a panel to an ImGui window;
   the scene polls `dvz_gui_moving()` / `dvz_gui_resizing()` each frame and calls
   `dvz_panel_resize()` to follow the ImGui window.


### v0.4 Target Architecture

The v0.3 architecture works well but has three problems for v0.4:

1. **DRP2 bypass is implicit** — the FramePlan has no knowledge of the ImGui overlay.
2. **Single backend** — `ImGui_ImplVulkan` is hard-coded; WebGPU and offscreen targets
   are not naturally supported.
3. **Vulkan-specific texture binding** — `dvz_gui_image` leaks `VkImageView` into scene code.

The v0.4 improvements are:

#### 1. Explicit External Overlay Slot In The Frame Lifecycle

The FramePlan carries an explicit **external overlay slot** as the last execution step,
after scene work and before present.
The runtime fills this slot with the ImGui render pass at the right point in the frame.
This makes the ordering contract explicit rather than implicit in the loop.
See `pipeline/FRAME_LIFECYCLE.md` step 10.

#### 2. Runtime-Selected ImGui Backend

The scene and DRP2 remain backend-agnostic.
The runtime selects and initializes the correct ImGui backend pair at startup:

| Target | ImGui graphics backend | ImGui windowing backend |
|---|---|---|
| Desktop Vulkan | `imgui_impl_vulkan` | `imgui_impl_glfw` |
| Desktop WebGPU (Dawn) | `imgui_impl_wgpu` | `imgui_impl_glfw` |
| Browser (Emscripten + WebGPU) | `imgui_impl_wgpu` | `imgui_impl_glfw` (Emscripten GLFW) |
| Offscreen / headless | either, no window | stub or none |

`imgui_impl_wgpu` is Dear ImGui's official WebGPU backend (added 2022-2023).
It supports Dawn (native) and browser WebGPU via Emscripten.
The scene layer never references which backend is active.

#### 3. Backend-Agnostic Texture Binding

`dvz_gui_image(tex, w, h)` is a scene-level call that takes a logical scene texture handle,
not a Vulkan image view.
The runtime resolves the handle to the backend-native texture descriptor
(`VkImageView` for Vulkan, `WGPUTextureView` for WebGPU) before passing it to ImGui.
Scene code never calls `ImGui_ImplVulkan_AddTexture` or any backend-specific function directly.


### Rendering Architecture (Both Versions)

The following properties are correct in both v0.3 and v0.4:

1. ImGui renders **after** the scene and **before** present.
2. ImGui uses a **separate render pass** with `LOAD_OP_LOAD` — scene output is preserved.
3. ImGui is **always on top** — there is no mechanism to interleave scene and ImGui layers.
4. ImGui rendering is **outside DRP2** — it uses a native backend path not visible to the
   scene plan.


### Input Routing

ImGui input polling (`ImGui_ImplGlfw_NewFrame`) runs before scene controllers each frame.
ImGui consumes input first; unconsumed events are forwarded to scene controllers.

Standard ImGui convention applies: `io.WantCaptureMouse` / `io.WantCaptureKeyboard` indicate
whether ImGui has claimed the current event.
Scene controllers should not process events claimed by ImGui.


### Panel–ImGui Window Binding

A Datoviz panel can be bound to an ImGui window so that the panel viewport follows the
ImGui window as the user drags or resizes it:

```text
dvz_panel_gui(panel, "Window Title", flags)
```

Each frame the scene reads the ImGui window's current rect and updates the panel viewport.
See `core/PANEL_LAYOUT.md` for the ImGui-driven layout mode.

**Serialization**: ImGui persists window positions and sizes to `imgui.ini` automatically.
Datoviz does not need its own layout serialization for ImGui-driven panels.

Docking is enabled with `DVZ_GUI_FLAGS_DOCKING`.


### Rendering Panel Content Inside ImGui

An offscreen Datoviz panel can be displayed inside an ImGui window:

```text
dvz_gui_image(tex, width, height)
```

The runtime resolves the logical texture handle to the backend-native descriptor and passes
it to ImGui. In v0.4 this call is backend-agnostic from the scene side.


### ImGui Font System

ImGui uses its own font atlas, independent of `DvzFont` / `DvzAtlas`.
Datoviz bundles Roboto Regular and Bold as the default ImGui fonts.
These are used only for ImGui widget text, not for `glyph` visual rendering.
The two font systems are parallel and do not share resources.
