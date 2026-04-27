# External UI Integration

This document defines how external UI frameworks should interact with the future scene layer.

An external UI framework may be ImGui, a native widget toolkit, a web-side control surface, or any
other app-level interface that reads and mutates scene-owned semantic state.


## Normative Status

This document is normative for the boundary between scene semantics and external UI.

It should be read together with:

1. `SCENE_API_SKETCH.md` for the scene-owned state surface,
2. `CONTROLLERS.md` for scene-native interaction behavior,
3. `FRAME_LIFECYCLE.md` for per-frame ordering,
4. `RUNTIME_BOUNDARY.md` for what must remain below the scene semantic layer.


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
The following describes the concrete architecture as implemented in v0.3 and carried forward
into v0.4.


### Rendering Architecture

ImGui rendering uses a **separate Vulkan render pass** that executes after the scene render
and before the swapchain present.

Key properties of the ImGui render pass:

1. **`LOAD_OP_LOAD`** — the ImGui render pass loads the existing framebuffer content rather
   than clearing it. This means ImGui draws on top of the completed scene output without
   discarding it.
2. **Shared swapchain images** — ImGui framebuffers point to the same swapchain images as the
   scene render pass, so ImGui output lands on the same surface.
3. **`VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` as final layout** — the ImGui render pass performs
   the layout transition to present-ready, since it is the last pass before present.

The frame sequence is:

```text
1. scene FramePlan executes → scene command buffer recorded and submitted
2. ImGui NewFrame + app UI callbacks → ImGui draw lists built
3. ImGui Render + ImGui_ImplVulkan_RenderDrawData → ImGui command buffer recorded
4. both command buffers submitted together in one dvz_submit_send
5. swapchain present
```

ImGui is **always rendered on top** of the scene. There is no mechanism to interleave scene
and ImGui layers.

This architecture is **below DRP2**. ImGui rendering uses `ImGui_ImplVulkan` directly and
does not appear in the scene's `FramePlan` or DRP2 emission. The scene has no knowledge of
ImGui draw calls.


### Input Routing

`ImGui_ImplGlfw_NewFrame()` runs before scene controllers each frame, so ImGui consumes
keyboard and mouse events first.
Unconsumed events (i.e., events that do not interact with any ImGui window) are forwarded to
scene controllers.

When an ImGui window is focused or hovered, scene controllers should not receive those events.
This matches the standard ImGui `io.WantCaptureMouse` / `io.WantCaptureKeyboard` convention.


### Panel–ImGui Window Binding

A Datoviz panel can be bound to an ImGui window so that the panel viewport follows the
ImGui window's position and size as the user drags or resizes it.

```text
dvz_panel_gui(panel, "My Panel Title", flags)
```

At runtime:
1. an ImGui window with the given title is created each frame,
2. the scene detects ImGui window move and resize events via `dvz_gui_moving()` /
   `dvz_gui_resizing()`,
3. on change, `dvz_panel_resize()` is called to update the Datoviz panel viewport,
4. the scene rebuilds the `FramePlan` with the updated viewport on the next frame.

This is the **ImGui-driven layout mode** described in `PANEL_LAYOUT.md`.

Docking is supported when the scene is initialized with `DVZ_GUI_FLAGS_DOCKING`.
In docking mode, panels can be docked into ImGui dock spaces and moved collectively.

**Serialization**: ImGui automatically persists window positions and sizes to `imgui.ini`.
Datoviz does not need its own layout serialization for ImGui-driven panels — `imgui.ini`
is the save/restore mechanism.


### Rendering Panel Content Inside ImGui

A Datoviz panel rendered to an offscreen texture can be displayed inside an ImGui window
using `dvz_gui_image`:

```text
dvz_gui_image(tex, width, height)
```

This calls `ImGui_ImplVulkan_AddTexture` internally to bind the offscreen texture to an
ImGui image widget.
The result is a panel whose content is rendered by Datoviz but displayed and positioned by
ImGui.

This pattern is useful for:
1. tool panels showing a thumbnail or auxiliary view,
2. multi-window applications where panels live in dockable ImGui windows,
3. applications that mix ImGui-native UI with Datoviz-rendered content in one window.


### ImGui Font System

ImGui uses its own font system (`ImGui_ImplVulkan` font atlas), independent of Datoviz's
`DvzFont` / `DvzAtlas` glyph pipeline.
Datoviz bundles Roboto Regular and Bold as the default ImGui fonts.
These are loaded into ImGui's font atlas at startup and are used only for ImGui widget text,
not for `glyph` visual text rendering.

The two font systems are parallel and do not share resources.


### Offscreen And Headless

ImGui rendering is supported in offscreen mode via `dvz_gui_offscreen()`.
In headless runs (testing, video export), the ImGui render pass still executes but no window
is shown.
`ImGui_ImplGlfw` is replaced by a stub that does not require a display.
