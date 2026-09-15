# GUI Viewports

This note describes the active v0.4 Dear ImGui integration shape.


## Terms

`DvzGui` is the Dear ImGui overlay attached to a visible GLFW `DvzAppWindow`.

`DvzGuiViewport` is an ImGui-hosted Datoviz viewport. It displays the latest image rendered by an
offscreen Datoviz app-window inside an ImGui window. It is not a scene `DvzPanel`.

`DvzPanel` remains the scene layout primitive inside a `DvzFigure`. A figure rendered in a GUI
viewport may contain one or many scene panels.


## Creation

The preferred public path is:

```c
DvzGuiViewport* viewport = dvz_gui_viewport(gui, figure, NULL);
```

This creates an owned offscreen `DvzAppWindow` for `figure`, connects its live-image sink to ImGui,
and displays it with:

```c
dvz_gui_viewport_window(viewport, "Datoviz viewport", NULL, 0);
```

For an application layout with a resizable sidebar and visualization area, dock the controls first and the viewport into the remaining center:

```c
dvz_gui_dock_window_once(gui, "Controls", DVZ_GUI_DOCK_SLOT_LEFT, 320.0f);
dvz_gui_dock_window_once(gui, "Datoviz viewport", DVZ_GUI_DOCK_SLOT_CENTER, 0.0f);
```

The resulting windows are sibling docks. Resizing the sidebar therefore resizes the hosted Datoviz figure instead of covering it.

`dvz_gui_viewport_from_window()` is the advanced path for integrations that already own an
offscreen source app-window.


## Raw ImGui C API

Datoviz also exposes the generated cimgui binding through `datoviz/imgui.h`. This is the raw
version-coupled API with upstream `ig*` names, such as `igBegin()`, `igTextUnformatted()`, and
`igButton()`. It is intended for advanced code that needs Dear ImGui coverage beyond the curated
`dvz_gui_*` helpers.

Raw `ig*` calls are valid inside a Datoviz GUI callback because Datoviz sets the current ImGui
context before invoking user GUI code. The raw layer should not own Datoviz concepts such as
`DvzGuiViewport`; those remain in `datoviz/gui.h`.

Use `datoviz/gui.h` for the stable Datoviz-facing layer: `DvzGui`, `DvzGuiViewport`, fonts,
viewport images, color editors, range controls, section headers, and compact convenience widgets.
Use `datoviz/imgui.h` when code needs direct Dear ImGui coverage and accepts the upstream/cimgui
naming and version coupling.


## Device and user scale

Datoviz applies one scale contract to scene screen-space quantities and the attached Dear ImGui context. App font sizes and curated GUI dimensions are authored in Datoviz logical pixels. Device scale converts those logical pixels to physical framebuffer pixels, while `dvz_view_set_user_scale()` applies an additional presentation or accessibility scale to both the scene and GUI content.

Dear ImGui uses native backend coordinates, which are not necessarily Datoviz logical coordinates. Datoviz derives this conversion from the current logical, native-window, and framebuffer extents. Fonts are rasterized at device resolution, and font metrics, padding, indentation, spacing, scrollbars, and other style dimensions follow the same effective scale.

Moving a visible view between monitors may change its device scale. Datoviz rebuilds the ImGui font atlas when the device or backend framebuffer scale changes. Changing only the view user scale updates GUI presentation without changing requested dock or viewport layout dimensions.

GUI style and vertex colors use display-encoded sRGB RGB values with linear alpha, consistently
with `DvzColor`. When the overlay target is an sRGB Vulkan attachment, the GUI renderer converts
packed vertex RGB to linear before blending and attachment encoding. Texture sampling keeps its
normal image-view color-space behavior. This avoids the washed-out, double-encoded appearance that
would result from sending Dear ImGui's display colors directly to an sRGB target.

The C examples make the split explicit:

| Example | API layer |
| ------- | --------- |
| `visuals/point` | Datoviz `dvz_gui_*` helpers in a retained-visual stress workbench |
| `techniques/gui_viewport` | Datoviz render target embedded in a dockable GUI viewport |
| `gui_viewport_glfw` | `DvzGuiViewport`, for a dockable Datoviz render target inside ImGui |

Reusable example control groups that combine these widgets live in
`examples/c/example_gui_controls.{h,c}`. Keep those helpers example-facing until a control block
maps cleanly to a stable Datoviz scene concept rather than to one demo's state struct.


## Input

Mouse input can be forwarded from the ImGui image item to the source input router. This is enabled
by default through `DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT`.

Scene controllers should be attached through the viewport input router:

```c
dvz_panel_set_panzoom(panel, dvz_gui_viewport_input(viewport), 0);
```

The viewport forwards pointer press, move, release, and wheel events in source-window logical coordinates, including the current keyboard modifier mask. During a button drag, forwarding continues even if the pointer leaves the ImGui item, and out-of-bounds drag coordinates are kept so controllers receive the full drag delta. Conversion is per-axis and remains correct when native-window and framebuffer scale differ.

While the viewport image is hovered, it owns horizontal and vertical wheel input. The enclosing ImGui window therefore does not scroll in response to the same wheel event.

Clicking a viewport gives it keyboard focus for Datoviz input routing. GLFW key press, release, and
repeat events are forwarded to that viewport only while no regular ImGui widget wants keyboard
capture. Clicking another ImGui item clears the focused viewport.


## Visibility

Hidden or collapsed ImGui viewport windows stop rendering their source figure after the first
source image is available. This avoids spending GPU time on hidden dock tabs. Set
`DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN` when continuous background rendering is required.

Viewport resize requests are debounced by `DvzGuiViewportConfig.resize_delay_frames`. Until the new logical and framebuffer extent is committed, Datoviz displays the last complete source image instead of exposing a partially resized render target.


## Retained tree layout

Retained trees use a compact font-relative layout by default, which is suitable for deep hierarchies. Use `dvz_gui_tree_layout()` and `dvz_gui_tree_set_layout()` to override indentation, row padding, item spacing, and label gaps in em units. These values automatically follow the GUI device and user scale.


## Ownership

`dvz_gui_viewport()` owns the source app-window render policy and disables rendering when the
viewport is destroyed. The app still owns the underlying app-window storage and releases it during
`dvz_app_destroy()`.

`dvz_gui_viewport_from_window()` borrows the source app-window. The caller keeps ownership of its
lifetime and render policy.
